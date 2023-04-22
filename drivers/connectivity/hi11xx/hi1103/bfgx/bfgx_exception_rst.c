

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include <linux/delay.h>
#include <linux/rtc.h>
#include <linux/tty.h>

#include "plat_type.h"
#include "plat_debug.h"
#include "hw_bfg_ps.h"
#include "plat_pm.h"
#include "plat_pm_wlan.h"
#include "bfgx_exception_rst.h"
#include "plat_firmware.h"
#include "plat_uart.h"

#include "oal_sdio.h"
#include "oal_sdio_host_if.h"
#include "oal_hcc_host_if.h"
#include "oal_ext_if.h"
#include <linux/ktime.h>
#include "chr_errno.h"
#include "oam_rdr.h"

/*****************************************************************************
  3 全局变量定义
*****************************************************************************/
struct st_exception_info *g_pst_exception_info_etc = NULL;
struct sdio_dump_bcpu_buff st_bcpu_dump_buff_etc = {NULL, 0, 0};
oal_netbuf_stru*       st_bcpu_dump_netbuf_etc = NULL;

#if (_PRE_PRODUCT_ID != _PRE_PRODUCT_ID_HI1103_HOST)
uint8 *g_bfgx_mem_file_name_etc[BFGX_MEM_DUMP_BLOCK_COUNT] =
                            {
                                "plat_nfc_stack",
                                "fgb_stack",
                                "bt_extra_mem",
                            };

struct st_exception_mem_info g_pst_bfgx_mem_dump_etc[BFGX_MEM_DUMP_BLOCK_COUNT] = {{0},{0},{0}};
#else
uint8 *g_bfgx_mem_file_name_etc[BFGX_MEM_DUMP_BLOCK_COUNT] =
                            {
                                "bcpu_itcm_mem",
                                "bcpu_dtcm_mem",
                            };

struct st_exception_mem_info g_pst_bfgx_mem_dump_etc[BFGX_MEM_DUMP_BLOCK_COUNT] = {{0},{0}};
#endif
uint32 g_recvd_block_count_etc = 0;

#define WIFI_PUB_REG_BLOCKS      (12)
#define WIFI_PRIV_REG_BLOCKS     (9)
#define WIFI_MEM_BLOCKS          (3)

#define WIFI_PUB_GLB_CTL_LEN              (4*1024)
#define WIFI_PUB_PMU_CMU_CTL_LEN          (4*1024)
#define WIFI_PUB_RF_WB_CTL_LEN            (2*1024)
#define WIFI_PUB_RF_WB_TRX_REG_LEN        (2*1024)
#define WIFI_PUB_RF_WB_PLL_REG_LEN        (2*1024)
#define WIFI_PUB_RF_FG_CTL_LEN            (2*1024)
#define WIFI_PUB_RF_FG_TRX_REG_LEN        (2*1024)
#define WIFI_PUB_RF_FG_PLL_REG_LEN        (2*1024)
#define WIFI_PUB_COEX_CTL_LEN             (1*1024)
#define WIFI_PUB_DIAG_CTL_LEN             (1*1024)
#define WIFI_PUB_COM_CTL_LEN              (2*1024)
#define WIFI_PUB_AON_GPIO_RTC_LEN         (16*1024)

#define WIFI_PUB_REG_TOTAL_LEN            (WIFI_PUB_GLB_CTL_LEN \
                                           + WIFI_PUB_PMU_CMU_CTL_LEN \
                                           + WIFI_PUB_RF_WB_CTL_LEN \
                                           + WIFI_PUB_RF_WB_TRX_REG_LEN \
                                           + WIFI_PUB_RF_WB_PLL_REG_LEN \
                                           + WIFI_PUB_RF_FG_CTL_LEN \
                                           + WIFI_PUB_RF_FG_TRX_REG_LEN \
                                           + WIFI_PUB_RF_FG_PLL_REG_LEN \
                                           + WIFI_PUB_COEX_CTL_LEN \
                                           + WIFI_PUB_DIAG_CTL_LEN \
                                           + WIFI_PUB_COM_CTL_LEN \
                                           + WIFI_PUB_AON_GPIO_RTC_LEN)

#define WIFI_PRIV_W_CTL_LEN               (1*1024)
#define WIFI_PRIV_W_WDT_LEN               (1*1024)
#define WIFI_PRIV_W_TIMER_LEN             (1*1024)
#define WIFI_PRIV_W_CPU_CTL_LEN           (1*1024)
#define WIFI_PRIV_W_PHY_BANK1_LEN         (1*1024)
#define WIFI_PRIV_W_PHY_BANK2_LEN         (1*1024)
#define WIFI_PRIV_W_PHY_BANK3_LEN         (1*1024)
#define WIFI_PRIV_W_PHY_BANK4_LEN         (1*1024)
#define WIFI_PRIV_W_MAC_BANK_LEN          (2*1024 + 512)
#define WIFI_PRIV_W_DMA_LEN               (1*1024)

#define WIFI_PRIV_REG_TOTAL_LEN           (WIFI_PRIV_W_CTL_LEN \
                                           + WIFI_PRIV_W_WDT_LEN \
                                           + WIFI_PRIV_W_TIMER_LEN \
                                           + WIFI_PRIV_W_CPU_CTL_LEN \
                                           + WIFI_PRIV_W_PHY_BANK1_LEN \
                                           + WIFI_PRIV_W_PHY_BANK2_LEN \
                                           + WIFI_PRIV_W_PHY_BANK3_LEN \
                                           + WIFI_PRIV_W_PHY_BANK4_LEN \
                                           + WIFI_PRIV_W_MAC_BANK_LEN \
                                           + WIFI_PRIV_W_DMA_LEN)

#define WIFI_MEM_W_TCM_WRAM_LEN           (512*1024)
#define WIFI_MEM_W_PKT_SHARE_RAM_LEN      (256*1024)
#define WIFI_MEM_B_SHARE_RAM_LEN          (64*1024)

#define WIFI_MEM_TOTAL_LEN                (WIFI_MEM_W_TCM_WRAM_LEN \
                                           + WIFI_MEM_W_PKT_SHARE_RAM_LEN \
                                           + WIFI_MEM_B_SHARE_RAM_LEN)

#define BFGX_PUB_REG_NUM                  (5)
#define BFGX_PRIV_REG_NUM                 (2)
#define BFGX_SHARE_RAM_NUM                (2)

#define BFGX_GLB_PMU_CMU_CTL_ADDR                       (0x50000000)
#define BFGX_RF_WB_CTL_ADDR                             (0x5000C000)
#define BFGX_RF_FG_CTL_ADDR                             (0x5000E000)
#define BFGX_COEX_DIAG_COM_CTL_ADDR                     (0x50010000)
#define BFGX_AON_WB_GPIO_RTC_ADDR                       (0x50006000)
#define BFGX_PATCH_PLAT_NFC_BFGNI_SHARE_RAM_ADDR        (0x300E4000)
#define BFGX_W_SHARE_RAM_ADDR                           (0x30178000)
#define BFGX_GNSS_SUB_ADDR                              (0x38000000)
#define BFGX_B_CTL_WDT_TIMER_UART_ADDR                  (0x40000000)
#define BFGX_IR_SUB_ADDR                                (0x40007000)
#define BFGX_B_DMA_CFG_ADDR                             (0x40010000)
#define BFGX_BT_FM_SUB_ADDR                             (0x41040000)
#define BFGX_NFC_APB_ADDR                               (0x60000000)

#define BFGX_GLB_PMU_CMU_CTL_LEN                        (1024*12)
#define BFGX_RF_WB_CTL_LEN                              (1024*8)
#define BFGX_RF_FG_CTL_LEN                              (1024*8)
#define BFGX_COEX_DIAG_COM_CTL_LEN                      (1024*20)
#define BFGX_AON_WB_GPIO_RTC_LEN                        (1024*16)
#define BFGX_PATCH_PLAT_NFC_BFGNI_SHARE_RAM_LEN         (1024*592)
#define BFGX_W_SHARE_RAM_LEN                            (1024*32)
#define BFGX_GNSS_SUB_LEN                               (1024*128)
#define BFGX_B_CTL_WDT_TIMER_UART_LEN                   (1024*20)
#define BFGX_IR_SUB_LEN                                 (1024*4)
#define BFGX_B_DMA_CFG_LEN                              (1024*4)
#define BFGX_BT_FM_SUB_LEN                              (1024*140)
#define BFGX_NFC_APB_LEN                                (1024*20)

#define GLB_PMU_CMU_CTL_FILE_NAME                       "glb_pmu_cmu_ctl"
#define RF_WB_CTL_FILE_NAME                             "rf_wb_ctl"
#define RF_FG_CTL_FILE_NAME                             "rf_fg_ctl"
#define COEX_DIAG_COM_CTL_FILE_NAME                     "coex_diag_com_ctl"
#define AON_WB_GPIO_RTC_FILE_NAME                       "aon_wb_gpio_rtc"
#define PATCH_PLAT_NFC_BFGNI_SHARE_RAM_FILE_NAME        "patch_plat_nfc_bfgni_share_ram"
#define W_SHARE_RAM_FILE_NAME                           "w_share_ram"
#define GNSS_SUB_FILE_NAME                              "gnss_sub"
#define B_CTL_WDT_TIMER_UART_FILE_NAME                  "b_ctl_wdt_timer_uart"
#define IR_SUB_FILE_NAME                                "ir_sub"
#define B_DMA_CFG_FILE_NAME                             "b_dma_cfg"
#define BT_FM_SUB_FILE_NAME                             "bt_fm_sub"
#define NFC_APB_FILE_NAME                               "nfc_apb"

struct st_uart_dump_wifi_mem_info g_ast_wifi_pub_reg_info_etc[WIFI_PUB_REG_BLOCKS] =
{
    {"glb_ctl",       WIFI_PUB_GLB_CTL_LEN},
    {"pmu_cmu_ctl",   WIFI_PUB_PMU_CMU_CTL_LEN},
    {"rf_wb_ctl",     WIFI_PUB_RF_WB_CTL_LEN},
    {"rf_wb_trx_reg", WIFI_PUB_RF_WB_TRX_REG_LEN},
    {"rf_wb_pll_reg", WIFI_PUB_RF_WB_PLL_REG_LEN},
    {"rf_fg_ctl",     WIFI_PUB_RF_FG_CTL_LEN},
    {"rf_fg_trx_reg", WIFI_PUB_RF_FG_TRX_REG_LEN},
    {"rf_fg_pll_reg", WIFI_PUB_RF_FG_PLL_REG_LEN},
    {"coex_ctl",      WIFI_PUB_COEX_CTL_LEN},
    {"diag_ctl",      WIFI_PUB_DIAG_CTL_LEN},
    {"com_ctl",       WIFI_PUB_COM_CTL_LEN},
    {"aon_gpio_rtc",  WIFI_PUB_AON_GPIO_RTC_LEN},
};

struct st_uart_dump_wifi_mem_info g_ast_wifi_priv_reg_info_etc[WIFI_PRIV_REG_BLOCKS] =
{
    {"w_ctl",         WIFI_PRIV_W_CTL_LEN},
    {"w_wdt",         WIFI_PRIV_W_WDT_LEN},
    {"w_timer",       WIFI_PRIV_W_TIMER_LEN},
    {"w_cpu_ctl",     WIFI_PRIV_W_CPU_CTL_LEN},
    {"w_phy_bank1",   WIFI_PRIV_W_PHY_BANK1_LEN},
    {"w_phy_bank2",   WIFI_PRIV_W_PHY_BANK2_LEN},
    {"w_phy_bank3",   WIFI_PRIV_W_PHY_BANK3_LEN},
    {"w_phy_bank4",   WIFI_PRIV_W_PHY_BANK4_LEN},
    {"w_mac_bank",    WIFI_PRIV_W_MAC_BANK_LEN},
    /*{"w_dma",         WIFI_PRIV_W_DMA_LEN},*/
};

struct st_uart_dump_wifi_mem_info g_ast_wifi_mem_info_etc[WIFI_MEM_BLOCKS] =
{
    {"w_tcm_wram",      WIFI_MEM_W_TCM_WRAM_LEN},
    {"w_pkt_share_ram", WIFI_MEM_W_PKT_SHARE_RAM_LEN},
    {"b_share_ram",     WIFI_MEM_B_SHARE_RAM_LEN},
};

struct st_uart_dump_wifi_info g_uart_read_wifi_mem_info_etc[UART_WIFI_MEM_DUMP_BOTTOM] =
{
    {SYS_CFG_READ_WLAN_PUB_REG,  WIFI_PUB_REG_TOTAL_LEN,  WIFI_PUB_REG_BLOCKS,  g_ast_wifi_pub_reg_info_etc},
    {SYS_CFG_READ_WLAN_PRIV_REG, WIFI_PRIV_REG_TOTAL_LEN, WIFI_PRIV_REG_BLOCKS, g_ast_wifi_priv_reg_info_etc},
    {SYS_CFG_READ_WLAN_MEM,      WIFI_MEM_TOTAL_LEN,      WIFI_MEM_BLOCKS,      g_ast_wifi_mem_info_etc},
};

struct st_exception_mem_info g_pst_uart_wifi_mem_dump_etc[UART_WIFI_MEM_DUMP_BOTTOM] = {{0},{0},{0}};
uint32 g_recvd_wifi_block_index_etc = UART_WIFI_MEM_DUMP_BOTTOM;

struct st_bfgx_reset_cmd g_ast_bfgx_reset_msg_etc[BFGX_BUTT] =
{
    {BT_RESET_CMD_LEN,   {0x04,0xff,0x01,0xc7}},
    {FM_RESET_CMD_LEN,   {0xfb}},
    {GNSS_RESET_CMD_LEN, {0x8, 0x0 ,0x0, 0x0, 0xa1, 0xb4, 0xc7, 0x51, GNSS_SEPER_TAG_LAST}},
    {IR_RESET_CMD_LEN,   {0}},
    {NFC_RESET_CMD_LEN,  {0}},
};

exception_bcpu_dump_msg g_sdio_read_bcpu_pub_reg_info_etc[BFGX_PUB_REG_NUM] =
{
    {GLB_PMU_CMU_CTL_FILE_NAME,  BFGX_GLB_PMU_CMU_CTL_ADDR,   ALIGN_2_BYTE, BFGX_GLB_PMU_CMU_CTL_LEN},
    {RF_WB_CTL_FILE_NAME,        BFGX_RF_WB_CTL_ADDR,         ALIGN_2_BYTE, BFGX_RF_WB_CTL_LEN},
    {RF_FG_CTL_FILE_NAME,        BFGX_RF_FG_CTL_ADDR,         ALIGN_2_BYTE, BFGX_RF_FG_CTL_LEN},
    {COEX_DIAG_COM_CTL_FILE_NAME,BFGX_COEX_DIAG_COM_CTL_ADDR, ALIGN_2_BYTE, BFGX_COEX_DIAG_COM_CTL_LEN},
    {AON_WB_GPIO_RTC_FILE_NAME  ,BFGX_AON_WB_GPIO_RTC_ADDR,   ALIGN_2_BYTE, BFGX_AON_WB_GPIO_RTC_LEN},
};
exception_bcpu_dump_msg g_sdio_read_bcpu_mem_info_etc[BFGX_SHARE_RAM_NUM] =
{
    {PATCH_PLAT_NFC_BFGNI_SHARE_RAM_FILE_NAME, BFGX_PATCH_PLAT_NFC_BFGNI_SHARE_RAM_ADDR,
                                                                        ALIGN_1_BYTE, BFGX_PATCH_PLAT_NFC_BFGNI_SHARE_RAM_LEN},
    {W_SHARE_RAM_FILE_NAME,                    BFGX_W_SHARE_RAM_ADDR,   ALIGN_1_BYTE, BFGX_W_SHARE_RAM_LEN},
};
exception_bcpu_dump_msg g_sdio_read_bcpu_priv_reg_info_etc[BFGX_PRIV_REG_NUM] =
{
	/*私有寄存器只能拷贝2个*/
    //{GNSS_SUB_FILE_NAME,             BFGX_GNSS_SUB_ADDR,             ALIGN_2_BYTE, BFGX_GNSS_SUB_LEN},
    {B_CTL_WDT_TIMER_UART_FILE_NAME, BFGX_B_CTL_WDT_TIMER_UART_ADDR, ALIGN_2_BYTE, BFGX_B_CTL_WDT_TIMER_UART_LEN},
    {IR_SUB_FILE_NAME,               BFGX_IR_SUB_ADDR,               ALIGN_2_BYTE, BFGX_IR_SUB_LEN},
    //{B_DMA_CFG_FILE_NAME,            BFGX_B_DMA_CFG_ADDR,            ALIGN_2_BYTE, BFGX_B_DMA_CFG_LEN},
    //{BT_FM_SUB_FILE_NAME,            BFGX_BT_FM_SUB_ADDR,            ALIGN_2_BYTE, BFGX_BT_FM_SUB_LEN},
    //{NFC_APB_FILE_NAME,              BFGX_NFC_APB_ADDR,              ALIGN_2_BYTE, BFGX_NFC_APB_LEN},
};

exception_bcpu_dump_msg g_sdio_read_all_etc[BFGX_PUB_REG_NUM + BFGX_SHARE_RAM_NUM + BFGX_PRIV_REG_NUM] =
{
    {GLB_PMU_CMU_CTL_FILE_NAME,  BFGX_GLB_PMU_CMU_CTL_ADDR,   ALIGN_2_BYTE, BFGX_GLB_PMU_CMU_CTL_LEN},
    {RF_WB_CTL_FILE_NAME,        BFGX_RF_WB_CTL_ADDR,         ALIGN_2_BYTE, BFGX_RF_WB_CTL_LEN},
    {RF_FG_CTL_FILE_NAME,        BFGX_RF_FG_CTL_ADDR,         ALIGN_2_BYTE, BFGX_RF_FG_CTL_LEN},
    {COEX_DIAG_COM_CTL_FILE_NAME,BFGX_COEX_DIAG_COM_CTL_ADDR, ALIGN_2_BYTE, BFGX_COEX_DIAG_COM_CTL_LEN},
    {AON_WB_GPIO_RTC_FILE_NAME  ,BFGX_AON_WB_GPIO_RTC_ADDR,   ALIGN_2_BYTE, BFGX_AON_WB_GPIO_RTC_LEN},
    {PATCH_PLAT_NFC_BFGNI_SHARE_RAM_FILE_NAME, BFGX_PATCH_PLAT_NFC_BFGNI_SHARE_RAM_ADDR,
                                                                        ALIGN_1_BYTE, BFGX_PATCH_PLAT_NFC_BFGNI_SHARE_RAM_LEN},
    {W_SHARE_RAM_FILE_NAME,                    BFGX_W_SHARE_RAM_ADDR,   ALIGN_1_BYTE, BFGX_W_SHARE_RAM_LEN},
    //{GNSS_SUB_FILE_NAME,             BFGX_GNSS_SUB_ADDR,             ALIGN_2_BYTE, BFGX_GNSS_SUB_LEN},
    {B_CTL_WDT_TIMER_UART_FILE_NAME, BFGX_B_CTL_WDT_TIMER_UART_ADDR, ALIGN_2_BYTE, BFGX_B_CTL_WDT_TIMER_UART_LEN},
    {IR_SUB_FILE_NAME,               BFGX_IR_SUB_ADDR,               ALIGN_2_BYTE, BFGX_IR_SUB_LEN},
    //{BT_FM_SUB_FILE_NAME,            BFGX_BT_FM_SUB_ADDR,            ALIGN_2_BYTE, BFGX_BT_FM_SUB_LEN},
    //{NFC_APB_FILE_NAME,              BFGX_NFC_APB_ADDR,              ALIGN_2_BYTE, BFGX_NFC_APB_LEN},
    //{B_DMA_CFG_FILE_NAME,            BFGX_B_DMA_CFG_ADDR,            ALIGN_2_BYTE, BFGX_B_DMA_CFG_LEN},
};

uint8  g_plat_beatTimer_timeOut_reset_cfg_etc = 0;
extern struct oal_sdio* oal_alloc_sdio_stru_etc(oal_void);
extern int32 ssi_write16_etc(uint16 addr, uint16 value);
uint8 gst_excp_test_cfg[EXCP_TEST_CFG_BOTT]={DFR_TEST_DISABLE,DFR_TEST_DISABLE,DFR_TEST_DISABLE,DFR_TEST_DISABLE};
#ifdef HI110X_HAL_MEMDUMP_ENABLE
memdump_info_t bcpu_memdump_cfg_etc;
memdump_info_t wcpu_memdump_cfg_etc;
#endif
/*****************************************************************************
  2 函数实现
*****************************************************************************/
void  bfgx_beat_timer_expire_etc(uint64 data);
int32 get_exception_info_reference_etc(struct st_exception_info **exception_data);
int32 plat_exception_handler_etc(uint32 subsys_type, uint32 thread_type, uint32 exception_type);
void  plat_exception_reset_work_etc(struct work_struct *work);
int32 wifi_exception_handler_etc(void);
int32 wifi_subsystem_reset_etc(void);
int32 wifi_system_reset_etc(void);
int32 wifi_status_recovery_etc(void);
int32 wifi_exception_mem_dump_etc(struct st_wifi_dump_mem_info *pst_mem_dump_info, uint32 count, oal_int32 excep_type);
int32 bfgx_exception_handler_etc(void);
int32 bfgx_subthread_reset_etc(void);
int32 bfgx_subsystem_reset_etc(void);
int32 bfgx_system_reset_etc(void);
int32 bfgx_recv_dev_mem_etc(uint8 *buf_ptr, uint16 count);
int32 bfgx_store_stack_mem_to_file_etc(void);
void  bfgx_dump_stack_etc(void);
int32 bfgx_status_recovery_etc(void);
int32 plat_bfgx_exception_rst_register_etc(struct ps_plat_s *data);
int32 plat_exception_reset_init_etc(void);
int32 plat_exception_reset_exit_etc(void);


void plat_dfr_cfg_set_etc(uint64 arg)
{
    struct st_exception_info *pst_exception_data = NULL;

    get_exception_info_reference_etc(&pst_exception_data);
    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("get exception info reference is error\n");
        return;
    }

    pst_exception_data->exception_reset_enable = arg ? (PLAT_EXCEPTION_ENABLE) : (PLAT_EXCEPTION_DISABLE);

    PS_PRINT_INFO("plat dfr cfg set value = %ld\n", arg);
}


void plat_beatTimer_timeOut_reset_cfg_set_etc(uint64 arg)
{
    g_plat_beatTimer_timeOut_reset_cfg_etc = arg ? (PLAT_EXCEPTION_ENABLE) : (PLAT_EXCEPTION_DISABLE);
    PS_PRINT_INFO("plat beat timer timeOut reset cfg set value = %ld\n", arg);
}

/*****************************************************************************
 * Prototype    : mod_beat_timer_etc
 * Description  :
 * input        : uint8
 * output       : no
 * Calls        :
 * Called By    :
 *
 *   History        :
 *   1.Date         : 2013/05/09
 *     Author       : wx145522
 *     Modification : Created function
*****************************************************************************/
int32 mod_beat_timer_etc(uint8 on)
{
    struct st_exception_info *pst_exception_data = NULL;

    get_exception_info_reference_etc(&pst_exception_data);
    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("get exception info reference is error\n");
        return -EXCEPTION_FAIL;
    }

    if (on)
    {
        PS_PRINT_INFO("reset beat timer\n");
        mod_timer(&pst_exception_data->bfgx_beat_timer, jiffies + BFGX_BEAT_TIME * HZ);
        atomic_set(&pst_exception_data->bfgx_beat_flag, BFGX_NOT_RECV_BEAT_INFO);
    }
    else
    {
        PS_PRINT_INFO("delete beat timer\n");
        del_timer_sync(&pst_exception_data->bfgx_beat_timer);
    }

    return 0;
}


void bfgx_beat_timer_expire_etc(uint64 data)
{
    struct ps_core_s *ps_core_d = NULL;
    struct st_exception_info *pst_exception_data = NULL;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35))
    struct tty_buffer *thead;
    struct tty_bufhead *buf = NULL;
#endif

    get_exception_info_reference_etc(&pst_exception_data);
    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("get exception info reference is error\n");
        return;
    }

    if (unlikely(NULL == pst_exception_data->ps_plat_d))
    {
        PS_PRINT_ERR("pst_exception_data->ps_plat_d is NULL\n");
        return;
    }

    ps_core_d = pst_exception_data->ps_plat_d->core_data;

    /*bfgx睡眠时，没有心跳消息上报*/
    if (BFGX_SLEEP == ps_core_d->ps_pm->bfgx_dev_state_get())
    {
        PS_PRINT_INFO("bfgx has sleep!\n");
        return;
    }

    if (BFGX_NOT_RECV_BEAT_INFO == atomic_read(&pst_exception_data->bfgx_beat_flag))
    {
        if (OAL_IS_ERR_OR_NULL(ps_core_d->tty))
        {
            PS_PRINT_ERR("tty is NULL\n");
            return;
        }

        PS_PRINT_ERR("###########host can not recvive bfgx beat info@@@@@@@@@@@@@@!\n");
        ps_uart_state_pre_etc(ps_core_d->tty);
        ps_uart_state_dump_etc(ps_core_d->tty);

        if(NULL == ps_core_d->tty->port)
        {
            PS_PRINT_ERR("tty->port is null, bfgx download patch maybe failed!\n");
            return;
        }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35))
        buf = &(ps_core_d->tty->port->buf);
        thead = buf->head;
        while (thead != NULL)
        {
            PS_PRINT_INFO("tty rx buf:used=0x%x,size=0x%x,commit=0x%x,read=0x%x\n", \
                           thead->used, thead->size, thead->commit, thead->read);
            thead = thead->next;
        }
#endif

        DECLARE_DFT_TRACE_KEY_INFO("bfgx beat timeout", OAL_DFT_TRACE_EXCEP);

        if(g_ssi_dump_en)
        {
            ssi_dump_device_regs(SSI_MODULE_MASK_COMM | SSI_MODULE_MASK_BCTRL |SSI_MODULE_MASK_WCTRL);
        }

        if (PLAT_EXCEPTION_ENABLE == g_plat_beatTimer_timeOut_reset_cfg_etc)
        {
            PS_PRINT_ERR("bfgx beat timer bring to reset work!\n");
            plat_exception_handler_etc(SUBSYS_BFGX, THREAD_IDLE, BFGX_BEATHEART_TIMEOUT);
            CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_GNSS, CHR_LAYER_DRV, CHR_GNSS_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_BEAT_TIMEOUT);
            return;
        }
    }

    atomic_set(&pst_exception_data->bfgx_beat_flag, BFGX_NOT_RECV_BEAT_INFO);

    PS_PRINT_INFO("reset beat timer\n");
    mod_timer(&pst_exception_data->bfgx_beat_timer, jiffies + BFGX_BEAT_TIME * HZ);

    bfg_check_timer_work();

    return;
}


int32 get_exception_info_reference_etc(struct st_exception_info **exception_data)
{
    if (NULL == exception_data)
    {
        PS_PRINT_ERR("%s parm exception_data is NULL\n", __func__);
        return -EXCEPTION_FAIL;
    }

    if (NULL == g_pst_exception_info_etc)
    {
        *exception_data = NULL;
        PS_PRINT_ERR("%s g_pst_exception_info_etc is NULL\n", __func__);
        return -EXCEPTION_FAIL;
    }

    *exception_data = g_pst_exception_info_etc;

    return EXCEPTION_SUCCESS;
}

int32 uart_reset_wcpu_etc(void)
{
    uint64 timeleft;
    struct ps_core_s *ps_core_d = NULL;
    struct st_exception_info *pst_exception_data = NULL;

    get_exception_info_reference_etc(&pst_exception_data);
    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("get exception info reference is error\n");
        return -EXCEPTION_FAIL;
    }

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is err\n");
        return -EXCEPTION_FAIL;
    }

    if (pst_exception_data->subsys_type >= SUBSYS_BOTTOM)
    {
        PS_PRINT_ERR("subsys [%d] is error!\n", pst_exception_data->subsys_type);
        return -EXCEPTION_FAIL;
    }

    if (EXCEPTION_SUCCESS != prepare_to_visit_node_etc(ps_core_d))
    {
        PS_PRINT_ERR("prepare work FAIL\n");
        return -EXCEPTION_FAIL;
    }

    INIT_COMPLETION(ps_core_d->wait_wifi_opened);
    ps_uart_state_pre_etc(ps_core_d->tty);

    if (SUBSYS_WIFI == pst_exception_data->subsys_type)
    {
        PS_PRINT_INFO("uart reset WCPU\n");
        ps_tx_sys_cmd_etc(ps_core_d, SYS_MSG, SYS_CFG_EXCEP_RESET_WCPU);
    }
    else
    {
        PS_PRINT_INFO("uart open WCPU\n");
        ps_tx_sys_cmd_etc(ps_core_d, SYS_MSG, SYS_CFG_OPEN_WIFI);
    }

    timeleft = wait_for_completion_timeout(&ps_core_d->wait_wifi_opened, msecs_to_jiffies(WAIT_WIFI_OPEN_TIME));
    if (!timeleft)
    {
        ps_uart_state_dump_etc(ps_core_d->tty);
        PS_PRINT_ERR("wait wifi open ack timeout\n");
        post_to_visit_node_etc(ps_core_d);
        CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_OPEN_WCPU);
        return -EXCEPTION_FAIL;
    }

    post_to_visit_node_etc(ps_core_d);

    return EXCEPTION_SUCCESS;
}
void plat_dfr_sysrst_type_cnt_inc(enum DFR_RST_SYSTEM_TYPE_E rst_type,enum SUBSYSTEM_ENUM subs)
{
    struct st_exception_info *pst_exception_data = NULL;
    if ((rst_type >= DFR_SYSTEM_RST_TYPE_BOTT) || (subs >= SUBSYS_BOTTOM))
    {
        goto exit;
    }
    get_exception_info_reference_etc(&pst_exception_data);
    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("get exception info reference is error\n");
        goto exit;
    }
    if (pst_exception_data->subsys_type == subs)
    {
        pst_exception_data->etype_info[pst_exception_data->excetion_type].rst_type_cnt[rst_type]++;
    }
    return;
exit:
    PS_PRINT_ERR("dfr rst type cnt inc fail\n");
}
excp_info_str_t  excp_info_str_tab[]= {
    {.id=BFGX_BEATHEART_TIMEOUT, .name="bfgx_beatheart_timeout" },
    {.id=BFGX_LASTWORD_PANIC         , .name="bfgx_lastword_panic" },
    {.id=BFGX_TIMER_TIMEOUT     , .name="bfgx_timer_timeout" },
    {.id=BFGX_ARP_TIMEOUT       , .name="bfgx_arp_timeout" },
    {.id=BFGX_POWERON_FAIL     , .name="bfgx_poweron_fail" },
    {.id=BFGX_WAKEUP_FAIL       , .name="bfgx_wakeup_fail" },
    {.id=WIFI_WATCHDOG_TIMEOUT  , .name="wifi_watchdog_timeout" },
    {.id=WIFI_POWERON_FAIL     , .name="wifi_poweron_fail" },
    {.id=WIFI_WAKEUP_FAIL       , .name="wifi_wakeup_fail" },
    {.id=WIFI_DEVICE_PANIC      , .name="wifi_device_panic" },
    {.id=WIFI_TRANS_FAIL        , .name="wifi_trans_fail" },
    {.id=SDIO_DUMPBCPU_FAIL , .name="sdio_dumpbcpu_fail" },
};
char* excp_info_str_get(int32 id)
{
    int32 i = 0;
    for(i = 0; i < sizeof(excp_info_str_tab)/sizeof(excp_info_str_t); i++) {
        if (id == excp_info_str_tab[i].id) {
            return excp_info_str_tab[i].name;
        }
    }
    return NULL;
}
int32 plat_get_dfr_sinfo(char* buf,int32 index)
{
    struct st_exception_info *pst_exception_data = NULL;
    int i = index;
    int etype;
    int ret;
    get_exception_info_reference_etc(&pst_exception_data);
    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("get exception info reference is error\n");
        return -EXCEPTION_FAIL;
    }
    ret = snprintf(buf + i, PAGE_SIZE - i, "==========dfr info:=========\n");
    if (0 >= ret)
    {
        return i;
    }
    i += ret;

    ret = snprintf(buf + i, PAGE_SIZE - i, "total cnt:%-10d\n", plat_get_excp_total_cnt());
    if (0 >= ret)
    {
        return i;
    }
    i += ret;

    for(etype = 0;etype < EXCEPTION_TYPE_BOTTOM ;etype++)
    {
        ret = snprintf(buf + i, PAGE_SIZE - i, "id:%-30scnt:%-10dfail:%-10dsingle_sysrst:%-10dall_sysrst:%-10dmaxtime:%lldms\n", excp_info_str_get(etype),
                                                                                            pst_exception_data->etype_info[etype].excp_cnt,
                                                                                            pst_exception_data->etype_info[etype].fail_cnt,
                                                                                            pst_exception_data->etype_info[etype].rst_type_cnt[DFR_SINGLE_SYS_RST],
                                                                                            pst_exception_data->etype_info[etype].rst_type_cnt[DFR_ALL_SYS_RST],
                                                                                            pst_exception_data->etype_info[etype].maxtime);
        if (0 >= ret)
        {
            return i;
        }
        i += ret;
    }
    return i;
}
void plat_print_dfr_info(void)
{
    struct st_exception_info *pst_exception_data = NULL;
    int etype;
    get_exception_info_reference_etc(&pst_exception_data);
    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("get exception info reference is error\n");
        return;
    }
    PS_PRINT_INFO("========== dfr info:+++++++++++++++++++++++++++++++\n");
    PS_PRINT_INFO("total cnt:%-10d\n" ,plat_get_excp_total_cnt());
    for(etype = 0;etype < EXCEPTION_TYPE_BOTTOM ;etype++)
    {
        PS_PRINT_INFO("type:%-30scnt:%-10dfail:%-10dsingle_sysrst:%-10dall_sysrst:%-10dmaxtime:%lldms\n", excp_info_str_get(etype),
                                                                                                            pst_exception_data->etype_info[etype].excp_cnt,
                                                                                                            pst_exception_data->etype_info[etype].fail_cnt,
                                                                                                            pst_exception_data->etype_info[etype].rst_type_cnt[DFR_SINGLE_SYS_RST],
                                                                                                            pst_exception_data->etype_info[etype].rst_type_cnt[DFR_ALL_SYS_RST],
                                                                                                            pst_exception_data->etype_info[etype].maxtime);
    }
    PS_PRINT_INFO("========== dfr info:-----------------------------------\n");

}

int32 plat_get_excp_total_cnt()
{
    struct st_exception_info *pst_exception_data = NULL;
    int i;
    int total_cnt = 0;
    get_exception_info_reference_etc(&pst_exception_data);
    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("get exception info reference is error\n");
        return -EXCEPTION_FAIL;
    }
    for(i = 0;i < EXCEPTION_TYPE_BOTTOM ;i++)
    {
        total_cnt += pst_exception_data->etype_info[i].excp_cnt;
    }
    return total_cnt;
}

int32 plat_power_fail_exception_info_set_etc(uint32 subsys_type, uint32 thread_type, uint32 exception_type)
{
    struct st_exception_info *pst_exception_data = NULL;
    uint64 flag;

    if (subsys_type >= SUBSYS_BOTTOM)
    {
        PS_PRINT_ERR("para subsys_type %u is error!\n", subsys_type);
        return -EXCEPTION_FAIL;
    }

    if (((subsys_type == SUBSYS_WIFI) && (thread_type >= WIFI_THREAD_BOTTOM))
     || ((subsys_type == SUBSYS_BFGX) && (thread_type >= BFGX_THREAD_BOTTOM)))
    {
        PS_PRINT_ERR("para thread_type %u is error! subsys_type is %u\n", thread_type, subsys_type);
        return -EXCEPTION_FAIL;
    }

    if (exception_type >= EXCEPTION_TYPE_BOTTOM)
    {
        PS_PRINT_ERR("para exception_type %u is error!\n", exception_type);
        return -EXCEPTION_FAIL;
    }

    get_exception_info_reference_etc(&pst_exception_data);
    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("get exception info reference is error\n");
        return -EXCEPTION_FAIL;
    }

    if (PLAT_EXCEPTION_ENABLE != pst_exception_data->exception_reset_enable)
    {
        PS_PRINT_WARNING("palt exception reset not enable!");
        return -EXCEPTION_FAIL;
    }

    spin_lock_irqsave(&pst_exception_data->exception_spin_lock, flag);

    if (PLAT_EXCEPTION_RESET_IDLE == atomic_read(&pst_exception_data->is_reseting_device))
    {
        pst_exception_data->subsys_type   = subsys_type;
        pst_exception_data->thread_type   = thread_type;
        pst_exception_data->excetion_type = exception_type;

        /*当前异常没有处理完成之前，不允许处理新的异常*/
        atomic_set(&pst_exception_data->is_reseting_device, PLAT_EXCEPTION_RESET_BUSY);
    }
    else
    {
        PS_PRINT_INFO("plat is processing exception! subsys=%d, exception type=%d\n",
                               pst_exception_data->subsys_type, pst_exception_data->excetion_type);
        spin_unlock_irqrestore(&pst_exception_data->exception_spin_lock, flag);
        return -EXCEPTION_FAIL;
    }

    /*增加统计信息*/
    pst_exception_data->etype_info[exception_type].excp_cnt += 1;

    spin_unlock_irqrestore(&pst_exception_data->exception_spin_lock, flag);

    return EXCEPTION_SUCCESS;
}


void plat_power_fail_process_done_etc(void)
{
    struct st_exception_info *pst_exception_data = NULL;

    get_exception_info_reference_etc(&pst_exception_data);
    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("get exception info reference is error\n");
        return;
    }

    atomic_set(&pst_exception_data->is_reseting_device, PLAT_EXCEPTION_RESET_IDLE);

    PS_PRINT_SUC("bfgx open fail process done\n");

    return;
}


int32 plat_exception_handler_etc(uint32 subsys_type, uint32 thread_type, uint32 exception_type)
{
    struct st_exception_info *pst_exception_data = NULL;
    uint64 flag;
    int32 timeout;
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();

    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -EXCEPTION_FAIL;
    }

    if (subsys_type >= SUBSYS_BOTTOM)
    {
        PS_PRINT_ERR("para subsys_type %u is error!\n", subsys_type);
        return -EXCEPTION_FAIL;
    }

    if (((subsys_type == SUBSYS_WIFI) && (thread_type >= WIFI_THREAD_BOTTOM))
     || ((subsys_type == SUBSYS_BFGX) && (thread_type >= BFGX_THREAD_BOTTOM)))
    {
        PS_PRINT_ERR("para thread_type %u is error! subsys_type is %u\n", thread_type, subsys_type);
        return -EXCEPTION_FAIL;
    }

    if (exception_type >= EXCEPTION_TYPE_BOTTOM)
    {
        PS_PRINT_ERR("para exception_type %u is error!\n", exception_type);
        return -EXCEPTION_FAIL;
    }

    get_exception_info_reference_etc(&pst_exception_data);
    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("get exception info reference is error\n");
        return -EXCEPTION_FAIL;
    }

    if (PLAT_EXCEPTION_ENABLE != pst_exception_data->exception_reset_enable)
    {
        PS_PRINT_INFO("plat exception reset not enable!");
        return EXCEPTION_SUCCESS;
    }

    /*这里只能用spin lock，因为该函数会被心跳超时函数调用，心跳超时函数属于软中断，不允许睡眠*/
    spin_lock_irqsave(&pst_exception_data->exception_spin_lock, flag);
    if (PLAT_EXCEPTION_RESET_IDLE == atomic_read(&pst_exception_data->is_reseting_device))
    {
        pst_exception_data->subsys_type   = subsys_type;
        pst_exception_data->thread_type   = thread_type;
        pst_exception_data->excetion_type = exception_type;

        /*当前异常没有处理完成之前，不允许处理新的异常*/
        atomic_set(&pst_exception_data->is_reseting_device, PLAT_EXCEPTION_RESET_BUSY);
    }
    else
    {
        PS_PRINT_INFO("plat is processing exception! subsys=%d, exception type=%d\n",
                               pst_exception_data->subsys_type, pst_exception_data->excetion_type);
        spin_unlock_irqrestore(&pst_exception_data->exception_spin_lock, flag);
        return EXCEPTION_SUCCESS;
    }
    spin_unlock_irqrestore(&pst_exception_data->exception_spin_lock, flag);

#ifdef _PRE_CONFIG_GPIO_TO_SSI_DEBUG
    /*等待SSI操作完成，防止NOC*/
    timeout = wait_for_ssi_idle_timeout(HI110X_DFR_WAIT_SSI_IDLE_MS);
    if(timeout <= 0) {
        PS_PRINT_ERR("[HI110X_DFR]wait for ssi idle failed\n");
        atomic_set(&pst_exception_data->is_reseting_device, PLAT_EXCEPTION_RESET_IDLE);
        return EXCEPTION_FAIL;
    }
    PS_PRINT_INFO("[HI110X_DFR]wait for ssi idle cost time:%dms\n",HI110X_DFR_WAIT_SSI_IDLE_MS - timeout);
#endif

    if (subsys_type == SUBSYS_BFGX)    {
         cancel_work_sync(&pm_data->send_allow_sleep_work);
         del_timer_sync(&pm_data->bfg_timer);
         pm_data->bfg_timer_mod_cnt = 0;
         pm_data->bfg_timer_mod_cnt_pre = 0;

        /*timer time out 中断调用时，不能再这里删除，死锁*/
        if (exception_type != BFGX_BEATHEART_TIMEOUT)
        {
                pm_data->ps_pm_interface->operate_beat_timer(BEAT_TIMER_DELETE);
        }
    }

    pst_exception_data->etype_info[pst_exception_data->excetion_type].stime=ktime_get();
    PS_PRINT_WARNING("[HI110X_DFR]plat start doing exception! subsys=%d, exception type=%d\n",
                               pst_exception_data->subsys_type, pst_exception_data->excetion_type);
    plat_bbox_msg_hander(subsys_type, exception_type);

    oal_wake_lock_timeout(&pst_exception_data->plat_exception_rst_wlock, 10*1000);/*处理异常，持锁10秒*/
    /*触发异常处理worker*/
    queue_work(pst_exception_data->plat_exception_rst_workqueue, &pst_exception_data->plat_exception_rst_work);

    return EXCEPTION_SUCCESS;
}

EXPORT_SYMBOL(plat_exception_handler_etc);


void plat_exception_reset_work_etc(struct work_struct *work)
{
    int32  ret = -EXCEPTION_FAIL;
    struct st_exception_info *pst_exception_data = NULL;
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();
    ktime_t trans_time;
    unsigned long long total_time;

    if (NULL == pm_data)
    {
        PS_PRINT_ERR("[HI110X_DFR]pm_data is NULL!\n");
        return;
    }

    get_exception_info_reference_etc(&pst_exception_data);
    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("[HI110X_DFR]get exception info reference is error\n");
        return;
    }

    PS_PRINT_WARNING("[HI110X_DFR] enter plat_exception_reset_work_etc\n");

    mutex_lock(&pm_data->host_mutex);

    if (SUBSYS_WIFI == pst_exception_data->subsys_type)
    {
        if (wlan_is_shutdown_etc())
        {
            PS_PRINT_WARNING("[HI110X_DFR]wifi is closed,stop wifi dfr\n");
            goto exit_exception;
        }
        ret = wifi_exception_handler_etc();
    }
    else
    {
        if (bfgx_is_shutdown_etc())
        {
            PS_PRINT_WARNING("[HI110X_DFR]bfgx is closed,stop bfgx dfr\n");
            goto exit_exception;
        }
        ret = bfgx_exception_handler_etc();
    }

    pst_exception_data->etype_info[pst_exception_data->excetion_type].excp_cnt +=1;

    if (ret != EXCEPTION_SUCCESS)
    {
        PS_PRINT_ERR("[HI110X_DFR]plat execption recovery fail! subsys_type = [%u], exception_type = [%u]\n",
                                       pst_exception_data->subsys_type, pst_exception_data->excetion_type);
        pst_exception_data->etype_info[pst_exception_data->excetion_type].fail_cnt++;
    }
    else
    {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35))
        trans_time = ktime_sub(ktime_get(), pst_exception_data->etype_info[pst_exception_data->excetion_type].stime);
        total_time = (unsigned long long)ktime_to_ms(trans_time);
        if (pst_exception_data->etype_info[pst_exception_data->excetion_type].maxtime < total_time) {
            pst_exception_data->etype_info[pst_exception_data->excetion_type].maxtime  = total_time;
            PS_PRINT_WARNING("[HI110X_DFR]time update:%llu,exception_type:%d \n",total_time,pst_exception_data->excetion_type);
        }
#endif
        PS_PRINT_WARNING("[HI110X_DFR]plat execption recovery success, current time [%llu]ms, max time [%llu]ms\n",total_time, pst_exception_data->etype_info[pst_exception_data->excetion_type].maxtime );
    }
    /*每次dfr完成，显示历史dfr处理结果*/
    plat_print_dfr_info();

exit_exception:
    atomic_set(&pst_exception_data->is_reseting_device, PLAT_EXCEPTION_RESET_IDLE);
    mutex_unlock(&pm_data->host_mutex);
    return;
}

int32 is_subsystem_rst_enable(void)
{
    struct st_exception_info *pst_exception_data = NULL;
    get_exception_info_reference_etc(&pst_exception_data);
    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("get exception info reference is error\n");
        return DFR_TEST_DISABLE;
    }
    PS_PRINT_INFO("#########is_subsystem_rst_enable:%d\n",pst_exception_data->subsystem_rst_en);
    return pst_exception_data->subsystem_rst_en;
}


int32 wifi_exception_handler_etc(void)
{
    int32 ret = -EXCEPTION_FAIL;
    uint32 exception_type;
    struct st_exception_info *pst_exception_data = NULL;

    get_exception_info_reference_etc(&pst_exception_data);
    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("get exception info reference is error\n");
        return -EXCEPTION_FAIL;
    }

    exception_type = pst_exception_data->excetion_type;

    /*如果bfgx已经打开，执行wifi子系统复位流程，否则执行wifi全系统复位流程*/
    if (!bfgx_is_shutdown_etc())
    {
        PS_PRINT_INFO("bfgx is opened, start wifi subsystem reset!\n");
        bfgx_print_subsys_state();
        if (!is_subsystem_rst_enable())
        {
            ret = wifi_subsystem_reset_etc();
            if (EXCEPTION_SUCCESS != ret)
            {
                PS_PRINT_ERR("wifi subsystem reset failed, start wifi system reset!\n");
                ret = wifi_system_reset_etc();
            }
        }
        else
        {
            PS_PRINT_ERR("in debug mode,skip subsystem rst,do wifi system reset!\n");
            ret = wifi_system_reset_etc();
        }
    }
    else
    {
        PS_PRINT_INFO("bfgx is not opened, start wifi system reset!\n");
        ret = wifi_system_reset_etc();
    }

    if (ret != EXCEPTION_SUCCESS)
    {
        PS_PRINT_ERR("wifi execption recovery fail!\n");
        return ret;
    }

    if (NULL == pst_exception_data->wifi_callback->wifi_recovery_complete)
    {
        PS_PRINT_ERR("wifi recovery complete callback not regist\n");
        return -EXCEPTION_FAIL;
    }

    pst_exception_data->wifi_callback->wifi_recovery_complete();

    PS_PRINT_INFO("wifi recovery complete\n");

    return EXCEPTION_SUCCESS;
}

int32 wifi_subsystem_reset_etc(void)
{
    oal_int32 ret;
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();
    int32 l_subchip_type = get_hi110x_subchip_type();

    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -EXCEPTION_FAIL;
    }

    switch (l_subchip_type)
    {
        case BOARD_VERSION_HI1102:
            if (EXCEPTION_SUCCESS != uart_reset_wcpu_etc())
            {
                PS_PRINT_ERR("uart reset wcpu failed\n");
                return -EXCEPTION_FAIL;
            }
            break;
        case BOARD_VERSION_HI1103:
            if (EXCEPTION_SUCCESS != hi1103_wifi_subsys_reset())
            {
                PS_PRINT_ERR("wifi reset failed\n");
                return -EXCEPTION_FAIL;
            }
            break;
        default:
            PS_PRINT_ERR("wifi subsys reset error! subchip=%d\n", l_subchip_type);
            return  -EXCEPTION_FAIL;
    }

    hcc_bus_power_action(hcc_get_current_110x_bus(), HCC_BUS_POWER_PATCH_LOAD_PREPARE);

    if (EXCEPTION_SUCCESS != firmware_download_function_etc(WIFI_CFG))
    {
        PS_PRINT_ERR("wifi firmware download failed\n");
        return -EXCEPTION_FAIL;
    }

    ret = hcc_bus_power_action(hcc_get_current_110x_bus(), HCC_BUS_POWER_PATCH_LAUCH);
    if (0 != ret)
    {
        PS_PRINT_ERR("HCC_BUS_POWER_PATCH_LAUCH failed, ret=%d", ret);
        CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_WCPU_BOOTUP);

        return -EXCEPTION_FAIL;
    }

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    //下发定制化参数到device去
    hwifi_hcc_customize_h2d_data_cfg();
#endif

    plat_dfr_sysrst_type_cnt_inc(DFR_SINGLE_SYS_RST, SUBSYS_WIFI);

    CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_WIFI_SUBSYS_DFR_SUCC);

    return EXCEPTION_SUCCESS;
}


int32 wifi_system_reset_etc(void)
{
    int32 ret;
    uint64 timeleft;
    struct ps_core_s *ps_core_d = NULL;
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return BFGX_POWER_FAILED;
    }

    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -EXCEPTION_FAIL;
    }

    /*重新上电，firmware重新加载*/
    hcc_bus_power_action(hcc_get_current_110x_bus(), HCC_BUS_POWER_PATCH_LOAD_PREPARE);

    if (!bfgx_is_shutdown_etc())
    {
        bfgx_print_subsys_state();
        del_timer_sync(&ps_core_d->ps_pm->pm_priv_data->bfg_timer);
        pm_data->bfg_timer_mod_cnt = 0;
        pm_data->bfg_timer_mod_cnt_pre = 0;

        ps_core_d->ps_pm->operate_beat_timer(BEAT_TIMER_DELETE);
        if (0 != release_tty_drv_etc(ps_core_d->pm_data))
        {
            PS_PRINT_WARNING("wifi_system_reset_etc, release_tty_drv_etc fail, line = %d\n",__LINE__);
        }
    }

    PS_PRINT_INFO("wifi system reset, board power on\n");
    ret = board_power_reset(WLAN_POWER);
    if(ret)
    {
        PS_PRINT_ERR("board_power_reset wlan failed=%d\n", ret);
        return -EXCEPTION_FAIL;
    }

    if (EXCEPTION_SUCCESS != firmware_download_function_etc(BFGX_AND_WIFI_CFG))
    {
        PS_PRINT_ERR("hi110x system power reset failed!\n");
        return -EXCEPTION_FAIL;
    }

    ret = hcc_bus_power_action(hcc_get_current_110x_bus(), HCC_BUS_POWER_PATCH_LAUCH);
    if (0 != ret)
    {
        PS_PRINT_ERR("HCC_BUS_POWER_PATCH_LAUCH failed ret=%d !!!!!!", ret);
        CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_WCPU_BOOTUP);

        return -EXCEPTION_FAIL;
    }

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
        //下发定制化参数到device去
    hwifi_hcc_customize_h2d_data_cfg();
#endif
    if (!bfgx_is_shutdown_etc())
    {
        INIT_COMPLETION(pm_data->dev_bootok_ack_comp);
        atomic_set(&pm_data->bfg_needwait_devboot_flag, NEED_SET_FLAG);

        if (0 !=open_tty_drv_etc(ps_core_d->pm_data))
        {
            PS_PRINT_ERR("open_tty_drv_etc failed\n");
            return -EXCEPTION_FAIL;
        }

        if (EXCEPTION_SUCCESS != wlan_pm_open_bcpu_etc())
        {
            PS_PRINT_ERR("wifi reset bcpu fail\n");
            if (0 != release_tty_drv_etc(ps_core_d->pm_data))
            {
                PS_PRINT_WARNING("wifi_system_reset_etc, release_tty_drv_etc fail, line = %d\n", __LINE__);
            }
            atomic_set(&pm_data->bfg_needwait_devboot_flag, NONEED_SET_FLAG);
            CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_OPEN_BCPU);

            return -EXCEPTION_FAIL;
        }

        timeleft = wait_for_completion_timeout(&pm_data->dev_bootok_ack_comp, msecs_to_jiffies(WAIT_BFGX_BOOTOK_TIME));
        if (!timeleft)
        {
            PS_PRINT_ERR("wait bfgx boot ok timeout\n");
            atomic_set(&pm_data->bfg_needwait_devboot_flag, NONEED_SET_FLAG);
            CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_BCPU_BOOTUP);

            return  -FAILURE;
        }

        atomic_set(&pm_data->bfg_needwait_devboot_flag, NONEED_SET_FLAG);

        bfgx_pm_feature_set_etc();

        /*恢复bfgx业务状态*/
        if (EXCEPTION_SUCCESS != bfgx_status_recovery_etc())
        {
            PS_PRINT_ERR("bfgx status revocery failed!\n");
            CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_BFGX_RECOVERY);

            return -EXCEPTION_FAIL;
        }

        plat_dfr_sysrst_type_cnt_inc(DFR_ALL_SYS_RST, SUBSYS_WIFI);
    }
    else
    {
        plat_dfr_sysrst_type_cnt_inc(DFR_SINGLE_SYS_RST, SUBSYS_WIFI);
    }

    CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_WIFI_SYSTEM_DFR_SUCC);

    return EXCEPTION_SUCCESS;
}


int32 wifi_status_recovery_etc(void)
{
    struct st_exception_info *pst_exception_data = NULL;

    get_exception_info_reference_etc(&pst_exception_data);
    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("get exception info reference is error\n");
        return -EXCEPTION_FAIL;
    }

	if (EXCEPTION_SUCCESS != wifi_subsystem_reset_etc())
	{
        PS_PRINT_ERR("wifi subsystem reset failed\n");
        return -EXCEPTION_FAIL;
	}

	if (NULL == pst_exception_data->wifi_callback->wifi_recovery_complete)
	{
        PS_PRINT_ERR("wifi recovery complete callback not regist\n");
        return -EXCEPTION_FAIL;
	}

	pst_exception_data->wifi_callback->wifi_recovery_complete();

    PS_PRINT_INFO("wifi recovery complete\n");

	return EXCEPTION_SUCCESS;
}


int32 wifi_open_bcpu_set_etc(uint8 enable)
{
    struct st_exception_info *pst_exception_data = NULL;
    uint64 timeleft;
    struct ps_core_s *ps_core_d = NULL;
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();

    if (NULL == pm_data)
    {
       PS_PRINT_ERR("pm_data is NULL!\n");
        return -EXCEPTION_FAIL;
    }

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is err\n");
        return -EXCEPTION_FAIL;
    }

    get_exception_info_reference_etc(&pst_exception_data);
    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("get exception info reference is error\n");
        return -EXCEPTION_FAIL;
    }

    if (!enable)
    {
        PS_PRINT_INFO("wifi_open_bcpu_enable flag disable\n");
        pst_exception_data->wifi_open_bcpu_enable = false;
        return EXCEPTION_SUCCESS;
    }

    PS_PRINT_INFO("wifi_open_bcpu_enable flag enable\n");
    pst_exception_data->wifi_open_bcpu_enable = true;

    mutex_lock(&pm_data->host_mutex);

    if (SUCCESS != release_tty_drv_etc(ps_core_d->pm_data))
    {
        PS_PRINT_ERR("close tty is err!");
        mutex_unlock(&pm_data->host_mutex);
        return -EXCEPTION_FAIL;
    }

    if (SUCCESS != open_tty_drv_etc(ps_core_d->pm_data))
    {
        PS_PRINT_ERR("open tty is err!\n");
        mutex_unlock(&pm_data->host_mutex);
        return -EXCEPTION_FAIL;
    }

    if(OAL_SUCC != wlan_pm_open_bcpu_etc())
    {
        mutex_unlock(&pm_data->host_mutex);

        CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_GNSS, CHR_LAYER_DRV, CHR_GNSS_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_OPEN_BCPU);

        return  -EXCEPTION_FAIL;
    }

    timeleft = wait_for_completion_timeout(&pm_data->dev_bootok_ack_comp, msecs_to_jiffies(WAIT_BFGX_BOOTOK_TIME));
    if (!timeleft)
    {
        PS_PRINT_ERR("wait BFGX boot ok timeout\n");
        mutex_unlock(&pm_data->host_mutex);
        return  -EXCEPTION_FAIL;
    }

    mutex_unlock(&pm_data->host_mutex);

    return EXCEPTION_SUCCESS;
}

EXPORT_SYMBOL(wifi_open_bcpu_set_etc);

int32 wifi_device_reset(void)
{
    uint64 timeleft;
    struct ps_core_s *ps_core_d = NULL;
    int32  l_subchip_type = get_hi110x_subchip_type();

    ps_get_core_reference_etc(&ps_core_d);
    if (NULL == ps_core_d)
    {
        PS_PRINT_ERR("ps_core_d is NULL\n");
        return -EXCEPTION_FAIL;
    }

    /*1102通过BCPU复位WCPU*/
    if (BOARD_VERSION_HI1102 == l_subchip_type)
    {
        PS_PRINT_INFO("reset wifi device by BCPU\n");
        /*If sdio transfer failed, reset wcpu first*/
        if (!bfgx_is_shutdown_etc())
        {
            if (prepare_to_visit_node_etc(ps_core_d) < 0)
            {
                PS_PRINT_ERR("prepare work FAIL\n");
                return -EXCEPTION_FAIL;
            }

            /*bcpu is power on, reset wcpu by bcpu*/
            INIT_COMPLETION(ps_core_d->wait_wifi_opened);
            ps_uart_state_pre_etc(ps_core_d->tty);
            ps_tx_sys_cmd_etc(ps_core_d, SYS_MSG, SYS_CFG_DUMP_RESET_WCPU);
            timeleft = wait_for_completion_timeout(&ps_core_d->wait_wifi_opened, msecs_to_jiffies(WAIT_WIFI_OPEN_TIME));
            if (!timeleft)
            {
                ps_uart_state_dump_etc(ps_core_d->tty);
                PS_PRINT_ERR("wait wifi open ack timeout\n");
                post_to_visit_node_etc(ps_core_d);
                return -EXCEPTION_FAIL;
            }
            else
            {
                PS_PRINT_INFO("reset wifi by uart sucuess\n");
                post_to_visit_node_etc(ps_core_d);
                return EXCEPTION_SUCCESS;
            }
        }
        else
        {
            PS_PRINT_INFO("bfgx did't opened, repower wifi directly\n");
            return -EXCEPTION_FAIL;
        }
    }
    else /*1103 通过w_en复位*/
    {
        PS_PRINT_INFO("reset wifi device by w_en gpio\n");
        hi1103_wifi_disable();
        if(hi1103_wifi_enable())
        {
            PS_PRINT_ERR("hi1103 wifi enable failed\n");
            return -EXCEPTION_FAIL;
        }
        return EXCEPTION_SUCCESS;
    }
}


int32 wifi_exception_mem_dump_etc(struct st_wifi_dump_mem_info *pst_mem_dump_info, uint32 count, oal_int32 excep_type)
{
    int32 ret;
    int32 reset_flag = 0;
    hcc_bus_dev* pst_bus_dev;
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();

    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -EXCEPTION_FAIL;
    }

    if(WIFI_DEVICE_PANIC != excep_type && WIFI_TRANS_FAIL != excep_type)
    {
        PS_PRINT_ERR("unsupport exception type :%d\n", excep_type);
        return -EXCEPTION_FAIL;
    }

    mutex_lock(&pm_data->host_mutex);

    if(wlan_is_shutdown_etc())
    {
        PS_PRINT_ERR("[E]dfr ignored, wifi shutdown before memdump\n");
        mutex_unlock(&pm_data->host_mutex);
        return -EXCEPTION_FAIL;
    }

retry:
    if(WIFI_DEVICE_PANIC != excep_type)
    {
        if (EXCEPTION_SUCCESS != wifi_device_reset())
        {
            PS_PRINT_ERR("wifi device reset fail, exception type :%d\n", excep_type);
            mutex_unlock(&pm_data->host_mutex);
            return -EXCEPTION_FAIL;
        }
        reset_flag = 1;
        hcc_change_state_exception_etc();
    }

    pst_bus_dev = hcc_get_bus_dev(HCC_CHIP_110X_DEV);

    if(NULL != pst_bus_dev)
    {
        ret = hcc_bus_reinit(pst_bus_dev->cur_bus);
    }
    else
    {
        ret = -OAL_EIO;
    }

    if (OAL_SUCC != ret)
    {
        PS_PRINT_ERR("wifi mem dump:current bus reinit failed, ret=[%d]\n", ret);
        if(!reset_flag)
        {
            PS_PRINT_ERR("reinit failed, try to reset wifi\n");
            excep_type = WIFI_TRANS_FAIL;
            goto retry;
        }
        mutex_unlock(&pm_data->host_mutex);
        return -EXCEPTION_FAIL;
    }

    ret = wifi_device_mem_dump(pst_mem_dump_info, count);
    hcc_bus_disable_state(pm_data->pst_wlan_pm_info->pst_bus, OAL_BUS_STATE_ALL);
    if (ret < 0)
    {
        PS_PRINT_ERR("wifi_device_mem_dump failed, ret=[%d]\n", ret);
        if(!reset_flag)
        {
            PS_PRINT_ERR("memdump failed, try to reset wifi\n");
            excep_type = WIFI_TRANS_FAIL;
            goto retry;
        }
        mutex_unlock(&pm_data->host_mutex);
        return -EXCEPTION_FAIL;
    }

    mutex_unlock(&pm_data->host_mutex);

    return EXCEPTION_SUCCESS;
}

EXPORT_SYMBOL(wifi_exception_mem_dump_etc);


int32 wifi_exception_work_submit_etc(uint32 wifi_excp_type)
{
    struct st_exception_info *pst_exception_data = NULL;
    hcc_bus* hi_bus = hcc_get_current_110x_bus();

    get_exception_info_reference_etc(&pst_exception_data);
    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("get exception info reference is error\n");
        return -EXCEPTION_FAIL;
    }

    if (PLAT_EXCEPTION_ENABLE != pst_exception_data->exception_reset_enable)
    {
        PS_PRINT_INFO("palt exception reset not enable!");
        return EXCEPTION_SUCCESS;
    }

#ifdef _PRE_WLAN_FEATURE_DFR
    if(oal_work_is_busy(&g_pst_exception_info_etc->wifi_excp_worker))
    {
        PS_PRINT_WARNING("WIFI DFR %pF Worker is Processing:doing wifi excp_work...need't submit\n",(void*)g_pst_exception_info_etc->wifi_excp_worker.func);
        return -OAL_EBUSY;
    }
    else if (oal_work_is_busy(&g_pst_exception_info_etc->wifi_excp_recovery_worker))
    {
        PS_PRINT_WARNING("WIFI DFR %pF Recovery_Worker is Processing:doing wifi wifi_excp_recovery_worker ...need't submit\n",(void*)g_pst_exception_info_etc->wifi_excp_worker.func);
        return -OAL_EBUSY;
    }else if ( (PLAT_EXCEPTION_RESET_IDLE != atomic_read(&pst_exception_data->is_reseting_device))&&(SUBSYS_WIFI == pst_exception_data->subsys_type))
    {
        PS_PRINT_WARNING("WIFI DFR %pF Recovery_Worker is Processing:doing  plat wifi recovery ...need't submit\n",(void*)g_pst_exception_info_etc->wifi_excp_worker.func);
        return -OAL_EBUSY;
    }
    else
    {
        PS_PRINT_ERR("WiFi DFR %pF Worker Submit, excp_type[%d]\n",(void*)g_pst_exception_info_etc->wifi_excp_worker.func, wifi_excp_type);
        g_pst_exception_info_etc->wifi_excp_type = wifi_excp_type;
        hcc_bus_disable_state(hi_bus, OAL_BUS_STATE_ALL);
        queue_work(g_pst_exception_info_etc->wifi_exception_workqueue, &g_pst_exception_info_etc->wifi_excp_worker);
    }
#else
    PS_PRINT_WARNING("Geting WIFI DFR, but _PRE_WLAN_FEATURE_DFR not open!");
#endif //_PRE_WLAN_FEATURE_DFR
    return OAL_SUCC;

}
EXPORT_SYMBOL(g_pst_exception_info_etc);
EXPORT_SYMBOL(wifi_exception_work_submit_etc);

oal_workqueue_stru* wifi_get_exception_workqueue_etc(oal_void)
{
    if(NULL == g_pst_exception_info_etc)
    {
        return NULL;
    }
    return g_pst_exception_info_etc->wifi_exception_workqueue;
}
EXPORT_SYMBOL(wifi_get_exception_workqueue_etc);


int32 prepare_to_recv_bfgx_stack_etc(uint32 len)
{
    if (g_recvd_block_count_etc > BFGX_MEM_DUMP_BLOCK_COUNT - 1)
    {
        PS_PRINT_ERR("bfgx mem dump have recvd [%d] blocks\n", g_recvd_block_count_etc);
        return -EXCEPTION_FAIL;
    }

    if (NULL == g_pst_bfgx_mem_dump_etc[g_recvd_block_count_etc].exception_mem_addr)
    {
        g_pst_bfgx_mem_dump_etc[g_recvd_block_count_etc].exception_mem_addr = (uint8 *)OS_VMALLOC_GFP(len);
        if (NULL == g_pst_bfgx_mem_dump_etc[g_recvd_block_count_etc].exception_mem_addr)
        {
            PS_PRINT_ERR("prepare mem to recv bfgx stack failed\n");
            return -EXCEPTION_FAIL;
        }
        else
        {
            g_pst_bfgx_mem_dump_etc[g_recvd_block_count_etc].recved_size = 0;
            g_pst_bfgx_mem_dump_etc[g_recvd_block_count_etc].total_size  = len;
            g_pst_bfgx_mem_dump_etc[g_recvd_block_count_etc].file_name   = g_bfgx_mem_file_name_etc[g_recvd_block_count_etc];
            PS_PRINT_INFO("prepare mem [%d] to recv bfgx stack\n", len);
        }
    }

    return EXCEPTION_SUCCESS;
}

int32 free_bfgx_stack_dump_mem_etc(void)
{
    uint32 i = 0;

    for (i = 0; i < BFGX_MEM_DUMP_BLOCK_COUNT; i++)
    {
        if (NULL != g_pst_bfgx_mem_dump_etc[i].exception_mem_addr)
        {
            OS_MEM_VFREE(g_pst_bfgx_mem_dump_etc[i].exception_mem_addr);
            g_pst_bfgx_mem_dump_etc[i].recved_size = 0;
            g_pst_bfgx_mem_dump_etc[i].total_size  = 0;
            g_pst_bfgx_mem_dump_etc[i].file_name   = NULL;
            g_pst_bfgx_mem_dump_etc[i].exception_mem_addr = NULL;
        }
    }

    g_recvd_block_count_etc = 0;

    return EXCEPTION_SUCCESS;
}


int32 bfgx_exception_handler_etc(void)
{
    int32  ret = -EXCEPTION_FAIL;
    uint32 exception_type;
    struct st_exception_info *pst_exception_data = NULL;
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();

    get_exception_info_reference_etc(&pst_exception_data);
    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("pst_exception_data is NULL!\n");
        return -EXCEPTION_FAIL;
    }

    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -EXCEPTION_FAIL;
    }

    if (pst_exception_data->excetion_type == BFGX_BEATHEART_TIMEOUT)
    {
        /*bfgx异常，先删除bfg timer和心跳timer，防止重复触发bfgx异常*/
        pm_data->ps_pm_interface->operate_beat_timer(BEAT_TIMER_DELETE);
    }

    exception_type = pst_exception_data->excetion_type;

    /*ioctl下来的异常执行线程复位流程,当前执行系统全复位*/
    if (exception_type == BFGX_TIMER_TIMEOUT || exception_type == BFGX_ARP_TIMEOUT)
    {
        ret = bfgx_subthread_reset_etc();
    }
    else
    {
        /*异常恢复之前，尝试用平台命令读栈，不能保证一定成功，因为此时uart可能不通*/
        bfgx_dump_stack_etc();

        ret = bfgx_subsystem_reset_etc();
    }

    if (EXCEPTION_SUCCESS != ret)
    {
        PS_PRINT_ERR("bfgx exception recovery fail!, exception_type = [%u]\n", exception_type);
        del_timer_sync(&pm_data->bfg_timer);
        pm_data->bfg_timer_mod_cnt = 0;
        pm_data->bfg_timer_mod_cnt_pre = 0;

        pm_data->ps_pm_interface->operate_beat_timer(BEAT_TIMER_DELETE);
        return ret;
    }

    return EXCEPTION_SUCCESS;
}


int32 bfgx_subthread_reset_etc(void)
{
    /*这里执行芯片全系统复位，保证芯片下电*/
    return bfgx_system_reset_etc();
}

int32 wifi_reset_bfgx_etc(void)
{
    uint64 timeleft;
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();

    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -FAILURE;
    }

    if (EXCEPTION_SUCCESS != wlan_pm_shutdown_bcpu_cmd_etc())
    {
        PS_PRINT_ERR("wifi shutdown bcpu fail\n");
        CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_GNSS, CHR_LAYER_DRV, CHR_GNSS_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_CLOSE_BCPU);

        return -EXCEPTION_FAIL;
    }

    INIT_COMPLETION(pm_data->dev_bootok_ack_comp);

    if (EXCEPTION_SUCCESS != wlan_pm_open_bcpu_etc())
    {
        PS_PRINT_ERR("wifi reset bcpu fail\n");
        CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_GNSS, CHR_LAYER_DRV, CHR_GNSS_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_OPEN_BCPU);

        return -EXCEPTION_FAIL;
    }

    timeleft = wait_for_completion_timeout(&pm_data->dev_bootok_ack_comp, msecs_to_jiffies(WAIT_BFGX_BOOTOK_TIME));
    if (!timeleft)
    {
        PS_PRINT_ERR("wait bfgx boot ok timeout\n");
        CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_GNSS, CHR_LAYER_DRV, CHR_GNSS_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_BCPU_BOOTUP);

        return  -EXCEPTION_FAIL;
    }

    return EXCEPTION_SUCCESS;
}


int32 __bfgx_subsystem_reset_etc(void)
{
    uint64 timeleft;
    struct ps_core_s *ps_core_d = NULL;
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();
    int32 l_subchip_type = get_hi110x_subchip_type();

    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -FAILURE;
    }

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is err\n");
        return -EXCEPTION_FAIL;
    }

    if (false == ps_chk_bfg_active_etc(ps_core_d))
    {
        PS_PRINT_ERR("bfgx no subsys is opened\n");
        return EXCEPTION_SUCCESS;
    }

    if (EXCEPTION_SUCCESS != release_tty_drv_etc(ps_core_d->pm_data))
    {
        PS_PRINT_ERR("close tty is err!\n");
        if (!is_tty_open(ps_core_d->pm_data))
        {
            return -EXCEPTION_FAIL;
        }
    }

    if (EXCEPTION_SUCCESS != open_tty_drv_etc(ps_core_d->pm_data))
    {
        PS_PRINT_ERR("open tty is err!\n");
        return -EXCEPTION_FAIL;
    }

    atomic_set(&pm_data->bfg_needwait_devboot_flag, NEED_SET_FLAG);

    switch (l_subchip_type)
    {
        case BOARD_VERSION_HI1102:
            if (EXCEPTION_SUCCESS != wifi_reset_bfgx_etc())
            {
                PS_PRINT_ERR("wifi reset bfgx fail\n");
                atomic_set(&pm_data->bfg_needwait_devboot_flag, NONEED_SET_FLAG);
                return -EXCEPTION_FAIL;
            }
            break;
        case BOARD_VERSION_HI1103:
            INIT_COMPLETION(pm_data->dev_bootok_ack_comp);
            hi1103_bfgx_subsys_reset();
            timeleft = wait_for_completion_timeout(&pm_data->dev_bootok_ack_comp, msecs_to_jiffies(WAIT_BFGX_BOOTOK_TIME));
            if (!timeleft)
            {
                PS_PRINT_ERR("wait bfgx boot ok timeout\n");
                atomic_set(&pm_data->bfg_needwait_devboot_flag, NONEED_SET_FLAG);
                CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_GNSS, CHR_LAYER_DRV, CHR_GNSS_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_BCPU_BOOTUP);

                return  -EXCEPTION_FAIL;
            }
            break;
        default:
            PS_PRINT_ERR("bfgx subsys reset error! subchip=%d\n", l_subchip_type);
            atomic_set(&pm_data->bfg_needwait_devboot_flag, NONEED_SET_FLAG);
            return  -EXCEPTION_FAIL;
    }

    atomic_set(&pm_data->bfg_needwait_devboot_flag, NONEED_SET_FLAG);

    bfgx_pm_feature_set_etc();

    if (EXCEPTION_SUCCESS != bfgx_status_recovery_etc())
    {
        PS_PRINT_ERR("bfgx recovery status failed\n");

        CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_GNSS, CHR_LAYER_DRV, CHR_GNSS_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_BFGX_RECOVERY);
        return -EXCEPTION_FAIL;
    }

    plat_dfr_sysrst_type_cnt_inc(DFR_SINGLE_SYS_RST, SUBSYS_BFGX);

    CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_GNSS, CHR_LAYER_DRV, CHR_GNSS_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_BFGX_SUBSYS_DFR_SUCC);

    return EXCEPTION_SUCCESS;
}


int32 bfgx_subsystem_reset_etc(void)
{
    if (!wlan_is_shutdown_etc())
    {
        PS_PRINT_INFO("wifi is opened, start bfgx subsystem reset!\n");

        if (!is_subsystem_rst_enable())
        {
            if (EXCEPTION_SUCCESS != __bfgx_subsystem_reset_etc())
            {
                PS_PRINT_ERR("bfgx subsystem reset failed, start bfgx system reset!\n");
                return bfgx_system_reset_etc();
            }
        }
        else
        {
            PS_PRINT_INFO("in debug mode,skip subsystem rst,do bfgx system reset!\n");
            return bfgx_system_reset_etc();
        }

        return EXCEPTION_SUCCESS;
    }
    else
    {
        PS_PRINT_INFO("wifi is not opened, start bfgx system reset!\n");
        return bfgx_system_reset_etc();
    }
}

int32 bfgx_power_reset_etc(void)
{
    int32 ret = -FAILURE;
    uint64 timeleft;
    struct ps_core_s *ps_core_d = NULL;
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is err\n");
        return -FAILURE;
    }

    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -FAILURE;
    }

    if (0 != release_tty_drv_etc(ps_core_d->pm_data))
    {
        PS_PRINT_WARNING("bfgx_power_reset_etc, release_tty_drv_etc fail\n");
    }

    PS_PRINT_INFO("bfgx system reset, board power on\n");
    ret = board_power_reset(BFGX_POWER);
    if(ret)
    {
        PS_PRINT_ERR("board_power_reset bfgx failed=%d\n", ret);
        return -EXCEPTION_FAIL;
    }

    if (0 != open_tty_drv_etc(ps_core_d->pm_data))
    {
        PS_PRINT_WARNING("bfgx_power_reset_etc, open_tty_drv_etc fail\n");
    }

    INIT_COMPLETION(pm_data->dev_bootok_ack_comp);
    atomic_set(&pm_data->bfg_needwait_devboot_flag, NEED_SET_FLAG);

    if (EXCEPTION_SUCCESS != firmware_download_function_etc(BFGX_CFG))
    {
        hcc_bus_disable_state(pm_data->pst_wlan_pm_info->pst_bus, OAL_BUS_STATE_ALL);
        PS_PRINT_ERR("bfgx power reset failed!\n");
        atomic_set(&pm_data->bfg_needwait_devboot_flag, NONEED_SET_FLAG);
        return -EXCEPTION_FAIL;
    }
    hcc_bus_disable_state(pm_data->pst_wlan_pm_info->pst_bus, OAL_BUS_STATE_ALL);

    timeleft = wait_for_completion_timeout(&pm_data->dev_bootok_ack_comp, msecs_to_jiffies(WAIT_BFGX_BOOTOK_TIME));
    if (!timeleft)
    {
        PS_PRINT_ERR("wait bfgx boot ok timeout\n");
        atomic_set(&pm_data->bfg_needwait_devboot_flag, NONEED_SET_FLAG);
        CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_GNSS, CHR_LAYER_DRV, CHR_GNSS_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_BCPU_BOOTUP);

        return  -EXCEPTION_FAIL;
    }

    atomic_set(&pm_data->bfg_needwait_devboot_flag, NONEED_SET_FLAG);

    bfgx_pm_feature_set_etc();

    return EXCEPTION_SUCCESS;
}


int32 bfgx_system_reset_etc(void)
{
    struct st_exception_info *pst_exception_data = NULL;

    get_exception_info_reference_etc(&pst_exception_data);
    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("get exception info reference is error\n");
        return -EXCEPTION_FAIL;
    }

    if (!wlan_is_shutdown_etc())
    {
        if (NULL != pst_exception_data->wifi_callback->notify_wifi_to_recovery)
        {
            PS_PRINT_INFO("notify wifi bfgx start to power reset\n");
            pst_exception_data->wifi_callback->notify_wifi_to_recovery();
        }
    }

    /*重新上电，firmware重新加载*/
    if (EXCEPTION_SUCCESS != bfgx_power_reset_etc())
    {
        PS_PRINT_ERR("bfgx power reset failed!\n");
        return -EXCEPTION_FAIL;
    }

    if (EXCEPTION_SUCCESS != bfgx_status_recovery_etc())
    {
        PS_PRINT_ERR("bfgx status revocery failed!\n");

        CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_GNSS, CHR_LAYER_DRV, CHR_GNSS_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_BFGX_RECOVERY);
        return -EXCEPTION_FAIL;
    }

    if (!wlan_is_shutdown_etc())
    {
        if (EXCEPTION_SUCCESS != wifi_status_recovery_etc())
        {
            PS_PRINT_ERR("wifi status revocery failed!\n");

            CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_WIFI_RECOVERY);

            return -EXCEPTION_FAIL;
        }
        plat_dfr_sysrst_type_cnt_inc(DFR_ALL_SYS_RST, SUBSYS_BFGX);
    }
    else
    {
        plat_dfr_sysrst_type_cnt_inc(DFR_SINGLE_SYS_RST, SUBSYS_BFGX);
    }

    CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_GNSS, CHR_LAYER_DRV, CHR_GNSS_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_BFGX_SYSTEM_DFR_SUCC);

    return EXCEPTION_SUCCESS;
}


int32 bfgx_recv_dev_mem_etc(uint8 *buf_ptr, uint16 count)
{
    struct st_exception_mem_info *pst_mem_info = NULL;
    uint32 offset = 0;

    if (NULL == buf_ptr)
    {
        PS_PRINT_ERR("buf_ptr is NULL\n");
        return -EFAIL;
    }

    if(g_recvd_block_count_etc >= BFGX_MEM_DUMP_BLOCK_COUNT)
    {
        PS_PRINT_ERR("g_recvd_block_count_etc=[%d]\n", g_recvd_block_count_etc);
        return -EFAIL;
    }

    pst_mem_info = &(g_pst_bfgx_mem_dump_etc[g_recvd_block_count_etc]);

    if (NULL == pst_mem_info->exception_mem_addr)
    {
        PS_PRINT_ERR("mem addr is null, g_recvd_block_count_etc=[%d]\n", g_recvd_block_count_etc);
        return -EXCEPTION_FAIL;
    }

    offset = pst_mem_info->recved_size;
    if (offset + count > pst_mem_info->total_size)
    {
        PS_PRINT_ERR("outof buf total size, recved size is [%d], curr recved size is [%d], total size is [%d]\n", offset, count, pst_mem_info->total_size);
        return -EXCEPTION_FAIL;
    }
    else
    {
        PS_PRINT_DBG("cpy stack size [%d] to exception mem\n", count);
        OS_MEM_CPY(pst_mem_info->exception_mem_addr + offset, buf_ptr, count);
        pst_mem_info->recved_size += count;
    }

    if (pst_mem_info->recved_size == pst_mem_info->total_size)
    {
        g_recvd_block_count_etc++;
        PS_PRINT_INFO("mem block [%d] recvd done\n", g_recvd_block_count_etc);
    }

    return EXCEPTION_SUCCESS;
}
#ifndef HI110X_HAL_MEMDUMP_ENABLE

int32 bfgx_store_stack_mem_to_file_etc(void)
{
    OS_KERNEL_FILE_STRU *fp;
    char filename[100] = {0};
    uint32 i;
    mm_segment_t fs;
    struct st_exception_mem_info *pst_mem_info = NULL;

    for (i = 0; i < BFGX_MEM_DUMP_BLOCK_COUNT; i++)
    {
        pst_mem_info = &(g_pst_bfgx_mem_dump_etc[i]);
        if (NULL == pst_mem_info->exception_mem_addr)
        {
            PS_PRINT_ERR("mem addr is null, i=[%d]\n", i);
            continue;
        }
        OS_MEM_SET(filename, 0, sizeof(filename));
        snprintf(filename, sizeof(filename),BFGX_DUMP_PATH"/%s_%s.bin", UART_STORE_BFGX_STACK, pst_mem_info->file_name);
        /*打开文件，准备保存接收到的内存*/
        fp = filp_open(filename, O_RDWR | O_CREAT, 0664);
        if (OAL_IS_ERR_OR_NULL(fp))
        {
            PS_PRINT_ERR("create file error,fp = 0x%p,filename:%s\n", fp, filename);
            continue;
        }

        /*将接收到的内存写入到文件中*/
        fs = get_fs();
        set_fs(KERNEL_DS);
        //l_ret = vfs_llseek(fp, 0, SEEK_END);
        PS_PRINT_INFO("pos = %d\n", (int)fp->f_pos);
        vfs_write(fp, pst_mem_info->exception_mem_addr, pst_mem_info->recved_size, &fp->f_pos);
        set_fs(fs);

        filp_close(fp, NULL);
    }

    /*send cmd to oam_hisi to rotate file*/
    plat_send_rotate_cmd_2_app_etc(CMD_READM_BFGX_UART);

    free_bfgx_stack_dump_mem_etc();

    return EXCEPTION_SUCCESS;
}
#endif

void bfgx_dump_stack_etc(void)
{
    uint64 timeleft;
    uint32 exception_type;
    struct ps_core_s *ps_core_d = NULL;
    struct st_exception_info *pst_exception_data = NULL;

    get_exception_info_reference_etc(&pst_exception_data);
    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("get exception info reference is error\n");
        return;
    }

    exception_type = pst_exception_data->excetion_type;

    if (exception_type != BFGX_LASTWORD_PANIC)
    {
        return;
    }

    if (unlikely(NULL == pst_exception_data->ps_plat_d))
    {
        PS_PRINT_ERR("pst_exception_data->ps_plat_d is NULL\n");
        return;
    }
    ps_core_d = pst_exception_data->ps_plat_d->core_data;

    INIT_COMPLETION(pst_exception_data->wait_read_bfgx_stack);

    /*等待读栈操作完成*/
    timeleft = wait_for_completion_timeout(&pst_exception_data->wait_read_bfgx_stack, msecs_to_jiffies(WAIT_BFGX_READ_STACK_TIME));
    if (!timeleft)
    {
        ps_uart_state_dump_etc(ps_core_d->tty);
        atomic_set(&pst_exception_data->is_memdump_runing, 0);
        PS_PRINT_ERR("read bfgx stack failed!\n");
    }
    else
    {
        PS_PRINT_INFO("read bfgx stack success!\n");
    }
#ifndef HI110X_HAL_MEMDUMP_ENABLE
    plat_wait_last_rotate_finish_etc();

    bfgx_store_stack_mem_to_file_etc();
#endif
    return;
}

int32 prepare_to_recv_wifi_mem_etc(void)
{
    uint32 malloc_mem_len;
    uint32 index;

    PS_PRINT_INFO("%s\n", __func__);

    index = g_recvd_wifi_block_index_etc;
    if (index >= UART_WIFI_MEM_DUMP_BOTTOM)
    {
        PS_PRINT_ERR("g_recvd_wifi_block_index_etc is error [%d]\n", index);
        return -EXCEPTION_FAIL;
    }

    if (NULL == g_pst_uart_wifi_mem_dump_etc[index].exception_mem_addr)
    {
        malloc_mem_len = g_uart_read_wifi_mem_info_etc[index].total_size;
        g_pst_uart_wifi_mem_dump_etc[index].exception_mem_addr = (uint8 *)OS_VMALLOC_GFP(malloc_mem_len);
        if (NULL == g_pst_uart_wifi_mem_dump_etc[index].exception_mem_addr)
        {
            PS_PRINT_ERR("prepare mem to recv wifi mem failed\n");
            return -EXCEPTION_FAIL;
        }
        else
        {
            g_pst_uart_wifi_mem_dump_etc[index].recved_size = 0;
            g_pst_uart_wifi_mem_dump_etc[index].total_size  = malloc_mem_len;
            g_pst_uart_wifi_mem_dump_etc[index].file_name   = NULL;
            PS_PRINT_INFO("prepare mem [%d] to recv wifi mem, index = [%d]\n", malloc_mem_len, index);
        }
    }

    return EXCEPTION_SUCCESS;
}

int32 free_uart_read_wifi_mem_etc(void)
{
    uint32 index;

    PS_PRINT_INFO("%s\n", __func__);

    index = g_recvd_wifi_block_index_etc;
    if (index >= UART_WIFI_MEM_DUMP_BOTTOM)
    {
        PS_PRINT_ERR("g_recvd_wifi_block_index_etc is error [%d]\n", index);
        return -EXCEPTION_FAIL;
    }

    if (NULL != g_pst_uart_wifi_mem_dump_etc[index].exception_mem_addr)
    {
        OS_MEM_VFREE(g_pst_uart_wifi_mem_dump_etc[index].exception_mem_addr);
        g_pst_uart_wifi_mem_dump_etc[index].recved_size        = 0;
        g_pst_uart_wifi_mem_dump_etc[index].total_size         = 0;
        g_pst_uart_wifi_mem_dump_etc[index].exception_mem_addr = NULL;
        PS_PRINT_INFO("vfree uart read wifi mem [%d] success\n", index);
    }

    g_recvd_wifi_block_index_etc = UART_WIFI_MEM_DUMP_BOTTOM;

    return EXCEPTION_SUCCESS;
}

int32 uart_recv_wifi_mem_etc(uint8 *buf_ptr, uint16 count)
{
    struct st_exception_mem_info *pst_mem_info = NULL;
    uint32 offset = 0;
    uint32 index;

    if (NULL == buf_ptr)
    {
        PS_PRINT_ERR("buf_ptr is NULL\n");
        return -EFAIL;
    }

    index = g_recvd_wifi_block_index_etc;
    if (index >= UART_WIFI_MEM_DUMP_BOTTOM)
    {
        PS_PRINT_ERR("g_recvd_wifi_block_index_etc [%d] is error\n", index);
        return -EXCEPTION_FAIL;
    }

    pst_mem_info = &(g_pst_uart_wifi_mem_dump_etc[index]);

    if (NULL == pst_mem_info->exception_mem_addr)
    {
        PS_PRINT_ERR("mem addr is null, g_recvd_block_count_etc=[%d]\n", index);
        return -EXCEPTION_FAIL;
    }

    offset = pst_mem_info->recved_size;
    if (offset + count > pst_mem_info->total_size)
    {
        PS_PRINT_ERR("outof buf total size, index=[%d], recved size is [%d], curr recved size is [%d], total size is [%d]\n", index, offset, count, pst_mem_info->total_size);
        return -EXCEPTION_FAIL;
    }
    else
    {
        PS_PRINT_INFO("cpy wifi mem size [%d] to recv buffer\n", count);
        OS_MEM_CPY(pst_mem_info->exception_mem_addr + offset, buf_ptr, count);
        pst_mem_info->recved_size += count;
        PS_PRINT_INFO("index [%d] have recved size is [%d], total size is [%d]\n", index, pst_mem_info->recved_size, pst_mem_info->total_size);
    }

    return EXCEPTION_SUCCESS;
}

int32 __store_wifi_mem_to_file_etc(void)
{
    OS_KERNEL_FILE_STRU *fp;
    char filename[100] = {0};
    mm_segment_t fs;
    uint32 index;
    uint32 i;
    uint32 block_count;
    uint8 *block_file_name;
    uint32 block_size;
    uint32 offset = 0;
    struct st_exception_mem_info *pst_mem_info = NULL;

#ifdef PLATFORM_DEBUG_ENABLE
    struct timeval tv;
    struct rtc_time tm;

    do_gettimeofday(&tv);
    rtc_time_to_tm(tv.tv_sec, &tm);
    PS_PRINT_INFO("%4d-%02d-%02d  %02d:%02d:%02d\n",
           tm.tm_year + 1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
#endif
    PS_PRINT_INFO("%s\n", __func__);

    index = g_recvd_wifi_block_index_etc;
    if (index >= UART_WIFI_MEM_DUMP_BOTTOM)
    {
        PS_PRINT_ERR("g_recvd_wifi_block_index_etc [%d] is error\n", index);
        return -EXCEPTION_FAIL;
    }

    pst_mem_info = &(g_pst_uart_wifi_mem_dump_etc[index]);
    if (NULL == pst_mem_info->exception_mem_addr)
    {
        PS_PRINT_ERR("mem addr is null, g_recvd_wifi_block_index_etc=[%d]", index);
        return -EXCEPTION_FAIL;
    }

    block_count = g_uart_read_wifi_mem_info_etc[index].block_count;

    for (i = 0; i < block_count; i++)
    {
        block_size      = g_uart_read_wifi_mem_info_etc[index].block_info[i].size;
        block_file_name = g_uart_read_wifi_mem_info_etc[index].block_info[i].file_name;
        snprintf(filename, sizeof(filename),BFGX_DUMP_PATH"/%s_%s.bin", UART_STORE_WIFI_MEM, block_file_name);
        /*打开文件，准备保存接收到的内存*/
        fp = filp_open(filename, O_RDWR | O_CREAT, 0664);
        if (OAL_IS_ERR_OR_NULL(fp))
        {
            PS_PRINT_ERR("create file error,fp = 0x%p,filename:%s\n", fp, filename);
            return -EXCEPTION_FAIL;
        }

        /*将接收到的内存写入到文件中*/
        fs = get_fs();
        set_fs(KERNEL_DS);
        //l_ret = vfs_llseek(fp, 0, SEEK_END);
        PS_PRINT_INFO("pos = %d\n", (int)fp->f_pos);
        vfs_write(fp, pst_mem_info->exception_mem_addr + offset, block_size, &fp->f_pos);
        set_fs(fs);

        filp_close(fp, NULL);

        offset += block_size;
    }

    return EXCEPTION_SUCCESS;
}

void store_wifi_mem_to_file_work_etc(struct work_struct *work)
{
    struct st_exception_info *pst_exception_data = NULL;

    PS_PRINT_INFO("%s\n", __func__);

    get_exception_info_reference_etc(&pst_exception_data);
    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("get exception info reference is error\n");
        return;
    }

    __store_wifi_mem_to_file_etc();

    free_uart_read_wifi_mem_etc();

    complete(&pst_exception_data->wait_uart_read_wifi_mem);

    return;
}

void store_wifi_mem_to_file_etc(void)
{
    struct st_exception_info *pst_exception_data = NULL;

    PS_PRINT_INFO("%s\n", __func__);

    get_exception_info_reference_etc(&pst_exception_data);
    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("get exception info reference is error\n");
        return;
    }

    queue_work(pst_exception_data->plat_exception_rst_workqueue, &pst_exception_data->uart_store_wifi_mem_to_file_work);
    return;
}

int32 uart_halt_wcpu_etc(void)
{
    uint64 timeleft;
    hcc_bus*     pst_bus;
    hcc_bus_dev* pst_bus_dev;
    struct ps_core_s *ps_core_d = NULL;
    struct st_exception_info *pst_exception_data = NULL;

    PS_PRINT_INFO("%s\n", __func__);

    get_exception_info_reference_etc(&pst_exception_data);
    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("get exception info reference is error\n");
        return -EXCEPTION_FAIL;
    }

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is err\n");
        return -EINVAL;
    }

    INIT_COMPLETION(pst_exception_data->wait_uart_halt_wcpu);
    ps_uart_state_pre_etc(ps_core_d->tty);
    ps_tx_sys_cmd_etc(ps_core_d, SYS_MSG, SYS_CFG_HALT_WCPU);
    timeleft = wait_for_completion_timeout(&pst_exception_data->wait_uart_halt_wcpu, msecs_to_jiffies(UART_HALT_WCPU_TIMEOUT));
    if (!timeleft)
    {
        ps_uart_state_dump_etc(ps_core_d->tty);
        PS_PRINT_ERR("wait uart halt wcpu ack timeout\n");
        return -ETIMEDOUT;
    }

    pst_bus_dev = hcc_get_bus_dev(HCC_CHIP_110X_DEV);
    if(NULL == pst_bus_dev)
    {
        PS_PRINT_ERR("110x dev is null\n");
        return -ENODEV;
    }

    pst_bus = HDEV_TO_HBUS(pst_bus_dev);

    if(NULL == pst_bus)
    {
        PS_PRINT_ERR("110x bus is null\n");
        return -ENODEV;
    }

    hcc_bus_wakeup_request(pst_bus);
    return EXCEPTION_SUCCESS;
}


int32 exception_bcpu_dump_recv_etc(uint8* str, oal_netbuf_stru* netbuf)
{
    exception_bcpu_dump_header*      cmd_header={0};
    struct st_exception_info *pst_exception_data = NULL;

    get_exception_info_reference_etc(&pst_exception_data);
    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("get exception info reference is error\n");
        return -EXCEPTION_FAIL;
    }

    if (NULL == st_bcpu_dump_buff_etc.mem_addr)
    {
        PS_PRINT_ERR("st_bcpu_dump_buff_etc addr is null\n");
        return -EXCEPTION_FAIL;
    }

    cmd_header = (exception_bcpu_dump_header*)(str);
    oal_memcopy(st_bcpu_dump_buff_etc.mem_addr+st_bcpu_dump_buff_etc.data_len,
                str+sizeof(exception_bcpu_dump_header),cmd_header->men_len);
    st_bcpu_dump_buff_etc.data_len += cmd_header->men_len;

    complete(&pst_exception_data->wait_sdio_d2h_dump_ack);

    return EXCEPTION_SUCCESS;
}


int32  free_buffer_and_netbuf_etc(void)
{
    if (NULL != st_bcpu_dump_buff_etc.mem_addr)
    {
        OS_MEM_VFREE(st_bcpu_dump_buff_etc.mem_addr);
        st_bcpu_dump_buff_etc.data_limit = 0;
        st_bcpu_dump_buff_etc.data_len = 0;
        st_bcpu_dump_buff_etc.mem_addr = NULL;
    }

    if (NULL != st_bcpu_dump_netbuf_etc)
    {
        st_bcpu_dump_netbuf_etc = NULL;
    }
    return EXCEPTION_SUCCESS;
}


int32 sdio_halt_bcpu_etc(void)
{
    int32  ret;
    uint64 timeleft;
    int i;
    struct wlan_pm_s    *pst_wlan_pm = wlan_pm_get_drv_etc();

    hcc_tx_transfer_lock(hcc_get_110x_handler());

    //wake up wifi
    for (i = 0; i < WAKEUP_RETRY_TIMES; i++)
    {
        ret = wlan_pm_wakeup_dev_etc();
        if (OAL_SUCC == ret)
        {
            break;
        }
    }

    if (EXCEPTION_SUCCESS != ret)
    {
        PS_PRINT_ERR("wlan wake up fail!");
        hcc_tx_transfer_unlock(hcc_get_110x_handler());
        return -OAL_FAIL;
    }

    ret = hcc_bus_send_message(pst_wlan_pm->pst_bus, H2D_MSG_HALT_BCPU);
    if(0 == ret)
    {
        /*等待device执行命令*/
        timeleft = wait_for_completion_timeout(&pst_wlan_pm->st_halt_bcpu_done,msecs_to_jiffies(WLAN_HALT_BCPU_TIMEOUT));
        if(0 == timeleft)
        {
            PS_PRINT_ERR("sdio halt bcpu failed!\n");
            hcc_tx_transfer_unlock(hcc_get_110x_handler());
            return -OAL_FAIL;
        }
    }
    PS_PRINT_INFO("halt bcpu sucess!");
    hcc_tx_transfer_unlock(hcc_get_110x_handler());
    return OAL_SUCC;
}


int32 allocate_data_save_buffer_etc(uint32 len)
{
    //临时buff配置,用于传送数据
    st_bcpu_dump_buff_etc.mem_addr = OS_VMALLOC_GFP(len);
    if (NULL == st_bcpu_dump_buff_etc.mem_addr)
    {
        PS_PRINT_ERR("st_bcpu_dump_buff_etc allocate fail!\n");
        return -EXCEPTION_FAIL;
    }
    st_bcpu_dump_buff_etc.data_limit = len;
    st_bcpu_dump_buff_etc.data_len  = 0;
    return EXCEPTION_SUCCESS;
}


int32 allocate_send_netbuf_etc(uint32 len)
{
    st_bcpu_dump_netbuf_etc  = hcc_netbuf_alloc(len);
    if (NULL == st_bcpu_dump_netbuf_etc)
    {
        PS_PRINT_ERR("st_bcpu_dump_netbuf_etc allocate fail !\n");
        return -EXCEPTION_FAIL;
    }

    return EXCEPTION_SUCCESS;
}


int32 dump_header_init_etc(exception_bcpu_dump_header* header, uint32 align_type, uint32 addr, uint32 send_len)
{
    /*cmd 初始化*/
    header->align_type = align_type;
    header->start_addr = addr;
    header->men_len    = send_len;
    return EXCEPTION_SUCCESS;
}


int32 init_hcc_head_and_send_etc(struct hcc_transfer_param st_hcc_transfer_param,
                                         struct st_exception_info* pst_exception_data, uint32 wait_time)
{
    uint64 timeleft;
    int32 l_ret;
    //发送
    INIT_COMPLETION(pst_exception_data->wait_sdio_d2h_dump_ack);
    l_ret = hcc_tx_etc(hcc_get_110x_handler(), st_bcpu_dump_netbuf_etc, &st_hcc_transfer_param);
    if (OAL_SUCC != l_ret)
    {
        PS_PRINT_ERR("send tx  failed!\n");
        oal_netbuf_free(st_bcpu_dump_netbuf_etc);
        st_bcpu_dump_netbuf_etc = NULL;
        return -EXCEPTION_FAIL;
    }
    /*等待SDIO读数据完成*/
    timeleft = wait_for_completion_timeout(&pst_exception_data->wait_sdio_d2h_dump_ack, msecs_to_jiffies(wait_time));
    if (!timeleft)
    {
        PS_PRINT_ERR("sdio read  failed!\n");

        //oal_netbuf_free(st_bcpu_dump_netbuf_etc);
        st_bcpu_dump_netbuf_etc = NULL;
        return -EXCEPTION_FAIL;
    }
     return EXCEPTION_SUCCESS;
}


int32 sdio_get_and_save_data_etc(exception_bcpu_dump_msg* sdio_read_info, uint32 count)
{
    uint32 header_len;
    uint32 netbuf_len;
    uint32 send_len;
    uint32 index;
    uint32 i = 0;
    uint32 buffer_len;
    uint32 send_total_len;
    uint32 align_type;
    int32  error = EXCEPTION_SUCCESS;
    int8 filename[100] = {0};

    mm_segment_t fs = {0};
    OS_KERNEL_FILE_STRU *fp = {0};

    struct hcc_transfer_param st_hcc_transfer_param = {0};
    struct st_exception_info *pst_exception_data = NULL;
    exception_bcpu_dump_header      dump_header = {0};

    PS_PRINT_INFO("%s\n", __func__);

    get_exception_info_reference_etc(&pst_exception_data);
    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("get exception info reference is error\n");
        return -EXCEPTION_FAIL;
    }

    if (OAL_SUCC != sdio_halt_bcpu_etc())
    {
        PS_PRINT_ERR("halt bcpu error!\n");
        return -EXCEPTION_FAIL;
    }

    header_len = sizeof(exception_bcpu_dump_header);
    for (i = 0; i < count; i++)
    {
        index = 0;
        snprintf(filename, sizeof(filename),BFGX_DUMP_PATH"/%s_%s.bin", SDIO_STORE_BFGX_REGMEM, sdio_read_info[i].file_name);
        /*准备文件空间*/
        fp = filp_open(filename, O_RDWR | O_CREAT, 0664);
        if (OAL_IS_ERR_OR_NULL(fp))
        {
            PS_PRINT_ERR("create file error,fp = 0x%p\n", fp);
            return -EXCEPTION_FAIL;
        }
        fs = get_fs();
        set_fs(KERNEL_DS);
        //vfs_llseek(fp, 0, SEEK_END);
        PS_PRINT_INFO("%s is dumping...,pos = %d\n", sdio_read_info[i].file_name, (int)fp->f_pos);

        //prepare data buffer
        send_total_len = sdio_read_info[i].men_len;
        align_type = sdio_read_info[i].align_type;
        if (ALIGN_1_BYTE == align_type)
        {
            buffer_len = (send_total_len > DUMP_BCPU_MEM_BUFF_LEN) ? DUMP_BCPU_MEM_BUFF_LEN : sdio_read_info[i].men_len;
        }
        else
        {
            buffer_len = (send_total_len > DUMP_BCPU_REG_BUFF_LEN) ? DUMP_BCPU_REG_BUFF_LEN : sdio_read_info[i].men_len;
        }

        if(EXCEPTION_SUCCESS != allocate_data_save_buffer_etc(buffer_len))
        {
            error = -EXCEPTION_FAIL;
            goto exit;
        }

        //send cmd and save data
        while (index < send_total_len)
        {
            send_len = send_total_len - index;

            if (ALIGN_1_BYTE == align_type)
            {
                // dump mem set
                if (send_len > DUMP_BCPU_MEM_BUFF_LEN)
                {
                    send_len = DUMP_BCPU_MEM_BUFF_LEN;
                }
                hcc_hdr_param_init(&st_hcc_transfer_param, HCC_ACTION_TYPE_OAM, DUMP_MEM, 0, 0, DATA_HI_QUEUE);
            }
            else
            {   //dump reg set
                if (send_len > DUMP_BCPU_REG_BUFF_LEN)
                {
                    send_len = DUMP_BCPU_REG_BUFF_LEN;
                }
                hcc_hdr_param_init(&st_hcc_transfer_param, HCC_ACTION_TYPE_OAM, DUMP_REG, 0, 0, DATA_HI_QUEUE);
            }

            netbuf_len = header_len + send_len;
            if(EXCEPTION_SUCCESS != allocate_send_netbuf_etc(netbuf_len))
            {
                error = -EXCEPTION_FAIL;
                goto exit;
            }

            dump_header_init_etc(&dump_header, sdio_read_info[i].align_type,
                                            sdio_read_info[i].start_addr+index, send_len);
            if (NULL == st_bcpu_dump_netbuf_etc)
            {
                goto exit;
            }

            oal_memcopy(oal_netbuf_put(st_bcpu_dump_netbuf_etc, netbuf_len), &dump_header, sizeof(exception_bcpu_dump_header));

            //发送
            if (EXCEPTION_SUCCESS != init_hcc_head_and_send_etc(st_hcc_transfer_param, pst_exception_data, WIFI_DUMP_BCPU_TIMEOUT))
            {
                error = -EXCEPTION_FAIL;
                goto exit;
            }

            vfs_write(fp, st_bcpu_dump_buff_etc.mem_addr, st_bcpu_dump_buff_etc.data_len, &fp->f_pos);

            index += send_len;

            //prepare for next data
            st_bcpu_dump_buff_etc.data_len  = 0;
            st_bcpu_dump_netbuf_etc = NULL;
        }
        set_fs(fs);
        filp_close(fp, NULL);
        free_buffer_and_netbuf_etc();
    }
    return EXCEPTION_SUCCESS;
exit:
    set_fs(fs);
    filp_close(fp, NULL);
    free_buffer_and_netbuf_etc();
    complete(&pst_exception_data->wait_sdio_d2h_dump_ack);

    return error;
}


int32 debug_sdio_read_bfgx_reg_and_mem_etc(uint32 which_mem)
{
    struct ps_core_s *ps_core_d = NULL;
    struct st_exception_info *pst_exception_data = NULL;

    get_exception_info_reference_etc(&pst_exception_data);

    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("get exception info reference is error!\n");
        return -EXCEPTION_FAIL;
    }

    if (PLAT_EXCEPTION_ENABLE != pst_exception_data->exception_reset_enable)
    {
        PS_PRINT_ERR("plat dfr is not enable ,can not dump info");
        return -EXCEPTION_FAIL;
    }

    if (bfgx_is_shutdown_etc())
    {
        PS_PRINT_WARNING("bfgx is off can not dump bfgx msg !\n");
        return -EXCEPTION_FAIL;
    }

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is err\n");
        return -EXCEPTION_FAIL;
    }

    PS_PRINT_INFO("sdio dump bfgx msg begin\n");

    //prepare the wlan state
    if (wlan_is_shutdown_etc())
    {
        PS_PRINT_WARNING("wifi is closed, can not dump bcpu info!");
        return -EXCEPTION_FAIL;
    }
    else
    {
        PS_PRINT_INFO("wifi is open!\n");
    }

    //去能exception设置,halt bcpu不引发DFR
    pst_exception_data->exception_reset_enable = PLAT_EXCEPTION_DISABLE;

    //plat_wait_last_rotate_finish_etc();

    //dump data
    switch (which_mem)
    {
        case BFGX_PUB_REG:
            sdio_get_and_save_data_etc(g_sdio_read_bcpu_pub_reg_info_etc, BFGX_PUB_REG_NUM);
            break;
        case BFGX_PRIV_REG:
            sdio_get_and_save_data_etc(g_sdio_read_bcpu_priv_reg_info_etc, BFGX_PRIV_REG_NUM);
            break;
        case BFGX_MEM:
            sdio_get_and_save_data_etc(g_sdio_read_bcpu_mem_info_etc, BFGX_SHARE_RAM_NUM);
            break;
        case SDIO_BFGX_MEM_DUMP_BOTTOM:
            sdio_get_and_save_data_etc(g_sdio_read_all_etc, sizeof(g_sdio_read_all_etc)/sizeof(exception_bcpu_dump_msg));
            break;
        default:
            PS_PRINT_WARNING("input param error , which_mem is %d\n", which_mem);
            pst_exception_data->exception_reset_enable = PLAT_EXCEPTION_ENABLE;
            return -EXCEPTION_FAIL;
    }

    /*send cmd to oam_hisi to rotate file*/
    //plat_send_rotate_cmd_2_app_etc(CMD_READM_BFGX_SDIO);

    PS_PRINT_INFO("dump complete, recovery begin\n");

    //使能DFR, recovery
    pst_exception_data->exception_reset_enable = PLAT_EXCEPTION_ENABLE;
    plat_exception_handler_etc(SUBSYS_BFGX, THREAD_IDLE, SDIO_DUMPBCPU_FAIL);

    return EXCEPTION_SUCCESS;
}


int32 uart_read_wifi_mem_etc(uint32 which_mem)
{
    uint64 timeleft;
    struct ps_core_s *ps_core_d = NULL;
    struct st_exception_info *pst_exception_data = NULL;

    PS_PRINT_INFO("%s\n", __func__);

    get_exception_info_reference_etc(&pst_exception_data);
    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("get exception info reference is error\n");
        return -EXCEPTION_FAIL;
    }

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is err\n");
        return -EINVAL;
    }

    if (which_mem >= UART_WIFI_MEM_DUMP_BOTTOM)
    {
        PS_PRINT_ERR("param which_mem [%d] is err\n", which_mem);
        return -EINVAL;
    }

    g_recvd_wifi_block_index_etc = which_mem;

    if (0 > prepare_to_recv_wifi_mem_etc())
    {
        PS_PRINT_ERR("prepare mem to recv wifi mem fail, which_mem = [%d]\n", which_mem);
        g_recvd_wifi_block_index_etc = UART_WIFI_MEM_DUMP_BOTTOM;
        return -EINVAL;
    }

    INIT_COMPLETION(pst_exception_data->wait_uart_read_wifi_mem);
    ps_uart_state_pre_etc(ps_core_d->tty);
    ps_tx_sys_cmd_etc(ps_core_d, SYS_MSG, g_uart_read_wifi_mem_info_etc[which_mem].cmd);
    timeleft = wait_for_completion_timeout(&pst_exception_data->wait_uart_read_wifi_mem, msecs_to_jiffies(UART_READ_WIFI_MEM_TIMEOUT));
    if (!timeleft)
    {
        ps_uart_state_dump_etc(ps_core_d->tty);
        PS_PRINT_ERR("wait uart read wifi mem [%d] timeout\n", which_mem);
        free_uart_read_wifi_mem_etc();
        return -ETIMEDOUT;
    }

    return EXCEPTION_SUCCESS;
}

int32 debug_uart_read_wifi_mem_etc(uint32 ul_lock)
{
    uint32 i;
    uint32 read_mem_succ_count = 0;
    struct ps_core_s *ps_core_d = NULL;
    int32  l_subchip_type = get_hi110x_subchip_type();
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();

    PS_PRINT_INFO("%s\n", __func__);

    if (NULL == pm_data)
    {
       PS_PRINT_ERR("pm_data is NULL!\n");
        return -EINVAL;
    }

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is err\n");
        return -EINVAL;
    }

    if (BOARD_VERSION_HI1102 == l_subchip_type)
    {
        if(ul_lock)
        {
            mutex_lock(&pm_data->host_mutex);
        }

        if (EXCEPTION_SUCCESS != prepare_to_visit_node_etc(ps_core_d))
        {
            PS_PRINT_ERR("prepare work FAIL\n");
            goto fail_return;
        }

        if (EXCEPTION_SUCCESS != uart_halt_wcpu_etc())
        {
            PS_PRINT_ERR("uart halt wcpu fail!\n");
            post_to_visit_node_etc(ps_core_d);
            goto fail_return;
        }

    //plat_wait_last_rotate_finish_etc();

        for (i = 0; i < UART_WIFI_MEM_DUMP_BOTTOM; i++)
        {
            if (EXCEPTION_SUCCESS != uart_read_wifi_mem_etc(i))
            {
                PS_PRINT_ERR("uart read wifi mem [%d] fail!", i);
                break;
            }
            read_mem_succ_count++;
        }

        if (read_mem_succ_count > 0)
        {
            /*send cmd to oam_hisi to rotate file*/
            //plat_send_rotate_cmd_2_app_etc(CMD_READM_WIFI_UART);
        }
        else
        {
           // plat_rotate_finish_set_etc();
        }

        post_to_visit_node_etc(ps_core_d);

        if(ul_lock)
        {
            mutex_unlock(&pm_data->host_mutex);
        }

        return EXCEPTION_SUCCESS;
    }
    else
    {
        PS_PRINT_ERR("hi1103, ignore uart read wifi mem\n");
        return EXCEPTION_SUCCESS;
    }

 fail_return:
    if(ul_lock)
    {
        mutex_unlock(&pm_data->host_mutex);
    }

    return -EXCEPTION_FAIL;

}

int32 bfgx_reset_cmd_send_etc(uint32 subsys)
{
    int32 ret;
    struct ps_core_s *ps_core_d = NULL;
    struct st_bfgx_data *pst_bfgx_data = NULL;

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is null\n");
        return -EINVAL;
    }


    pst_bfgx_data = &ps_core_d->bfgx_info[subsys];

    ret = ps_push_skb_queue_etc(ps_core_d, g_ast_bfgx_reset_msg_etc[subsys].cmd, g_ast_bfgx_reset_msg_etc[subsys].len, g_bfgx_rx_queue_etc[subsys]);
    if (EXCEPTION_SUCCESS != ret)
    {
        PS_PRINT_ERR("push %s reset cmd to skb fail\n", g_bfgx_subsys_name_etc[subsys]);
        return -EXCEPTION_FAIL;
    }

    wake_up_interruptible(&pst_bfgx_data->rx_wait);

    return EXCEPTION_SUCCESS;
}


int32 bfgx_status_recovery_etc(void)
{
    uint32 i;
    struct st_exception_info *pst_exception_data = NULL;
    struct ps_core_s *ps_core_d = NULL;
    struct st_bfgx_data *pst_bfgx_data = NULL;

    get_exception_info_reference_etc(&pst_exception_data);
    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("get exception info reference is error\n");
        return -EXCEPTION_FAIL;
    }

    ps_get_core_reference_etc(&ps_core_d);
    if (unlikely(NULL == ps_core_d))
    {
        PS_PRINT_ERR("ps_core_d is null\n");
        return -EXCEPTION_FAIL;
    }

    if (EXCEPTION_SUCCESS != prepare_to_visit_node_etc(ps_core_d))
    {
        PS_PRINT_ERR("prepare work FAIL\n");
        return -EXCEPTION_FAIL;
    }

    for (i = 0; i < BFGX_BUTT; i++)
    {
        pst_bfgx_data = &ps_core_d->bfgx_info[i];
        if (POWER_STATE_SHUTDOWN == atomic_read(&pst_bfgx_data->subsys_state))
        {
            continue;
        }

        ps_kfree_skb_etc(ps_core_d, g_bfgx_rx_queue_etc[i]);

        if (EXCEPTION_SUCCESS != bfgx_open_cmd_send_etc(i))
        {
            PS_PRINT_ERR("bfgx open cmd fail\n");
            post_to_visit_node_etc(ps_core_d);
            return -EXCEPTION_FAIL;
        }

        if (EXCEPTION_SUCCESS != bfgx_reset_cmd_send_etc(i))
        {
            PS_PRINT_ERR("bfgx reset cmd send fail\n");
            post_to_visit_node_etc(ps_core_d);
            return -EXCEPTION_FAIL;
        }
    }

    post_to_visit_node_etc(ps_core_d);

    /*仅调试使用*/
    PS_PRINT_INFO("exception: set debug beat flag to 1\n");
    pst_exception_data->debug_beat_flag = 1;

    return EXCEPTION_SUCCESS;
}


int32 is_bfgx_exception_etc(void)
{
    struct st_exception_info *pst_exception_data = NULL;
    int32 is_exception;
    uint64 flag;

    get_exception_info_reference_etc(&pst_exception_data);
    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("get exception info reference is error\n");
        return 0;
    }

    spin_lock_irqsave(&pst_exception_data->exception_spin_lock, flag);
    if (PLAT_EXCEPTION_RESET_BUSY == atomic_read(&pst_exception_data->is_reseting_device))
    {
        is_exception = PLAT_EXCEPTION_RESET_BUSY;
    }
    else
    {
        is_exception = PLAT_EXCEPTION_RESET_IDLE;
    }
    spin_unlock_irqrestore(&pst_exception_data->exception_spin_lock, flag);

    return is_exception;
}


int32 plat_bfgx_exception_rst_register_etc(struct ps_plat_s *data)
{
	struct st_exception_info *pst_exception_data = NULL;

    get_exception_info_reference_etc(&pst_exception_data);
    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("get exception info reference is error\n");
        return -EXCEPTION_FAIL;
    }

    if (NULL == data)
    {
        PS_PRINT_ERR("para data is NULL!\n");
        return -EXCEPTION_FAIL;
    }

    pst_exception_data->ps_plat_d = data;

	return EXCEPTION_SUCCESS;
}

EXPORT_SYMBOL_GPL(plat_bfgx_exception_rst_register_etc);


int32 plat_wifi_exception_rst_register_etc(void *data)
{
	struct st_exception_info *pst_exception_data = NULL;
	struct st_wifi_dfr_callback *pst_wifi_callback = NULL;

    get_exception_info_reference_etc(&pst_exception_data);
    if (NULL == pst_exception_data)
    {
        PS_PRINT_ERR("get exception info reference is error\n");
        return -EXCEPTION_FAIL;
    }

    if (NULL == data)
    {
        PS_PRINT_ERR("param data is null\n");
        return -EXCEPTION_FAIL;
    }

    /*wifi异常回调函数注册*/
    pst_wifi_callback = (struct st_wifi_dfr_callback *)data;
    pst_exception_data->wifi_callback = pst_wifi_callback;

	return EXCEPTION_SUCCESS;
}

EXPORT_SYMBOL_GPL(plat_wifi_exception_rst_register_etc);


int32 plat_exception_reset_init_etc(void)
{
    struct st_exception_info *p_exception_data = NULL;

    p_exception_data = (struct st_exception_info *)kzalloc(sizeof(struct st_exception_info), GFP_KERNEL);
    if (NULL == p_exception_data)
    {
        PS_PRINT_ERR("kzalloc p_exception_data is failed!\n");
        return -EXCEPTION_FAIL;
    }

    p_exception_data->wifi_callback = NULL;

    p_exception_data->subsys_type   = SUBSYS_BOTTOM;
    p_exception_data->thread_type   = BFGX_THREAD_BOTTOM;
    p_exception_data->excetion_type = EXCEPTION_TYPE_BOTTOM;

    p_exception_data->exception_reset_enable   = PLAT_EXCEPTION_DISABLE;
    p_exception_data->subsystem_rst_en          = DFR_TEST_ENABLE;

    p_exception_data->ps_plat_d                = NULL;

    atomic_set(&p_exception_data->bfgx_beat_flag, BFGX_NOT_RECV_BEAT_INFO);
    atomic_set(&p_exception_data->is_reseting_device, PLAT_EXCEPTION_RESET_IDLE);

    /*初始化异常处理workqueue和work*/
    p_exception_data->plat_exception_rst_workqueue = create_singlethread_workqueue("plat_exception_reset_queue");
    INIT_WORK(&p_exception_data->plat_exception_rst_work, plat_exception_reset_work_etc);
    oal_wake_lock_init(&p_exception_data->plat_exception_rst_wlock, "hi11xx_excep_rst_wlock");
    INIT_WORK(&p_exception_data->uart_store_wifi_mem_to_file_work, store_wifi_mem_to_file_work_etc);

    /*初始化心跳timer*/
    init_timer(&p_exception_data->bfgx_beat_timer);
    p_exception_data->bfgx_beat_timer.function = bfgx_beat_timer_expire_etc;
    p_exception_data->bfgx_beat_timer.expires  = jiffies + BFGX_BEAT_TIME*HZ;
    p_exception_data->bfgx_beat_timer.data     = 0;

    /*初始化异常处理自旋锁*/
    spin_lock_init(&p_exception_data->exception_spin_lock);

    /*初始化bfgx读栈完成量*/
    init_completion(&p_exception_data->wait_read_bfgx_stack);
    /*初始化sdio读取bcpu完成量*/
    init_completion(&p_exception_data->wait_sdio_d2h_dump_ack);

    /*调试使用的变量初始化*/
    p_exception_data->debug_beat_flag          = 1;
    atomic_set(&p_exception_data->is_memdump_runing, 0);
    p_exception_data->wifi_open_bcpu_enable    = false;

    init_completion(&p_exception_data->wait_uart_read_wifi_mem);
    init_completion(&p_exception_data->wait_uart_halt_wcpu);

    g_pst_exception_info_etc = p_exception_data;

    /*初始化dump文件轮替模块*/
    plat_exception_dump_file_rotate_init_etc();

    hisi_conn_rdr_init();

    PS_PRINT_SUC("plat exception reset init success\n");

	return EXCEPTION_SUCCESS;
}

EXPORT_SYMBOL_GPL(plat_exception_reset_init_etc);


int32 plat_exception_reset_exit_etc(void)
{
    struct st_exception_info *p_exception_data = NULL;

    p_exception_data = g_pst_exception_info_etc;
    if (p_exception_data == NULL)
    {
        PS_PRINT_ERR("g_pst_exception_info_etc is NULL!\n");
        return -EXCEPTION_FAIL;
    }

    destroy_workqueue(p_exception_data->plat_exception_rst_workqueue);
    del_timer_sync(&p_exception_data->bfgx_beat_timer);

    oal_wake_lock_exit(&p_exception_data->plat_exception_rst_wlock);

    kfree(p_exception_data);
    g_pst_exception_info_etc = NULL;

    PS_PRINT_SUC("plat exception reset exit success\n");

    hisi_conn_rdr_exit();

    return EXCEPTION_SUCCESS;
}

EXPORT_SYMBOL_GPL(plat_exception_reset_exit_etc);

#ifdef PLATFORM_DEBUG_ENABLE
int32 is_dfr_test_en(enum excp_test_cfg_em excp_cfg)
{
    if (excp_cfg >= EXCP_TEST_CFG_BOTT) {
        return -1;
    }

    if (DFR_TEST_ENABLE == gst_excp_test_cfg[excp_cfg])
    {
        gst_excp_test_cfg[excp_cfg] = DFR_TEST_DISABLE;
        return 0;
    }
    return  -1;
}
EXPORT_SYMBOL_GPL(is_dfr_test_en);
void set_excp_test_en(enum excp_test_cfg_em excp_cfg)
{
    if (excp_cfg >= EXCP_TEST_CFG_BOTT) {
        return;
    }

    gst_excp_test_cfg[excp_cfg] = DFR_TEST_ENABLE;
}
#endif

#ifdef HI110X_HAL_MEMDUMP_ENABLE

void plat_exception_dump_file_rotate_init_etc(void)
{
    init_waitqueue_head(&bcpu_memdump_cfg_etc.dump_type_wait);
    skb_queue_head_init(&bcpu_memdump_cfg_etc.dump_type_queue);
    skb_queue_head_init(&bcpu_memdump_cfg_etc.quenue);
    init_waitqueue_head(&wcpu_memdump_cfg_etc.dump_type_wait);
    skb_queue_head_init(&wcpu_memdump_cfg_etc.dump_type_queue);
    skb_queue_head_init(&wcpu_memdump_cfg_etc.quenue);
    PS_PRINT_INFO("plat exception dump file rotate init success\n");
}

void excp_memdump_quenue_clear_etc(memdump_info_t* memdump_t)
{
    struct sk_buff *skb = NULL;
    while(NULL != (skb = skb_dequeue(&memdump_t->quenue)))
    {
        kfree_skb(skb);
    }
}
int32 bfgx_memdump_quenue_clear_etc(void)
{
    PS_PRINT_DBG("bfgx_memdump_quenue_clear_etc\n");
    excp_memdump_quenue_clear_etc(&bcpu_memdump_cfg_etc);
    return 0;
}
void wifi_memdump_quenue_clear_etc(void)
{
    PS_PRINT_DBG("wifi_memdump_quenue_clear_etc\n");
    excp_memdump_quenue_clear_etc(&wcpu_memdump_cfg_etc);
}
void bfgx_memdump_finish_etc(void)
{
    bcpu_memdump_cfg_etc.is_working =0;
}
void wifi_memdump_finish_etc(void)
{
    wcpu_memdump_cfg_etc.is_working =0;
}

int32 plat_excp_send_rotate_cmd_2_app_etc(uint32 which_dump, memdump_info_t* memdump_info )
{
    struct sk_buff  *skb =NULL;

    if (CMD_DUMP_BUFF <= which_dump)
    {
        PS_PRINT_WARNING("which dump:%d error\n", which_dump);
        return -EINVAL;
    }
    if (skb_queue_len(&memdump_info->dump_type_queue) > MEMDUMP_ROTATE_QUEUE_MAX_LEN)
    {
        PS_PRINT_WARNING("too many dump type in queue,dispose type:%d", which_dump);
        return -EINVAL;
    }

    skb = alloc_skb(sizeof(which_dump), GFP_KERNEL);
    if( NULL == skb)
    {
        PS_PRINT_ERR("alloc errno skbuff failed! len=%d, errno=%x\n", (int32)sizeof(which_dump), which_dump);
        return -EINVAL;
    }
    skb_put(skb, sizeof(which_dump));
    *(uint32*)skb->data = which_dump;
    skb_queue_tail(&memdump_info->dump_type_queue, skb);
    PS_PRINT_INFO("save rotate cmd [%d] in queue\n", which_dump);

    wake_up_interruptible(&memdump_info->dump_type_wait);

    return 0;
}

int32 notice_hal_memdump_etc(memdump_info_t* memdump_t, uint32 which_dump)
{
    PS_PRINT_FUNCTION_NAME;
    if (memdump_t->is_working) {
        PS_PRINT_ERR("is doing memdump\n");
        return -1;
    }
    excp_memdump_quenue_clear_etc(memdump_t);
    plat_excp_send_rotate_cmd_2_app_etc(which_dump, memdump_t);
    memdump_t->is_working =1;
    return 0;
}
int32 bfgx_notice_hal_memdump_etc(void)
{
    return notice_hal_memdump_etc(&bcpu_memdump_cfg_etc, CMD_READM_BFGX_UART);
}
int32 wifi_notice_hal_memdump_etc(void)
{
    return notice_hal_memdump_etc(&wcpu_memdump_cfg_etc, CMD_READM_WIFI_SDIO);
}
int32 excp_memdump_queue_etc(uint8 *buf_ptr, uint16 count, memdump_info_t* memdump_t)
{
    struct sk_buff *skb = NULL;

    PS_PRINT_DBG("[send] len:%d\n",count);
    if (!memdump_t->is_working) {
        PS_PRINT_ERR("excp_memdump_queue_etc not allow\n");
        return -EINVAL;;
    }
    if (NULL == buf_ptr)
    {
        PS_PRINT_ERR("buf_ptr is NULL\n");
        return -EINVAL;
    }
    if (in_atomic() || in_softirq() || in_interrupt() || irqs_disabled())
    {
        skb = alloc_skb(count, GFP_ATOMIC);
    }
    else
    {
        skb = alloc_skb(count, GFP_KERNEL);
    }

    if (NULL == skb) {
        PS_PRINT_ERR("can't allocate mem for new debug skb, len=%d\n", count);
        return -EINVAL;
    }

    memcpy(skb_tail_pointer(skb), buf_ptr, count);
    skb_put(skb, count);
    skb_queue_tail(&memdump_t->quenue, skb);
    PS_PRINT_DBG("[excp_memdump_queue_etc]qlen:%d,count:%d\n",memdump_t->quenue.qlen,count);
    return 0;
}
int32 bfgx_memdump_enquenue_etc(uint8 *buf_ptr, uint16 count)
{
    return excp_memdump_queue_etc(buf_ptr, count, &bcpu_memdump_cfg_etc);
}
int32 wifi_memdump_enquenue_etc(uint8 *buf_ptr, uint16 count)
{
    return excp_memdump_queue_etc(buf_ptr, count, &wcpu_memdump_cfg_etc);
}
#endif
