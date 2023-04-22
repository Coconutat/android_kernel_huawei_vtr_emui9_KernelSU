
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/regulator/consumer.h>
#include <linux/err.h>

#include<linux/init.h>
#include<linux/module.h>

#include <linux/lcdkit_dsm.h>
#include <linux/of.h>

#ifdef CONFIG_HUAWEI_DSM
#include <dsm/dsm_pub.h>
#include <linux/sched.h>
#include "lcdkit_dbg.h"

static int lcd_mdp_error_debug = 0;
struct dsm_client* lcd_dclient = NULL;

int lcdkit_record_dsm_err(u32 *dsi_status)
{
    if ( NULL == lcd_dclient )
    {
        LCDKIT_ERR(": there is no lcd_dclient!\n");
        return -1;
    }

    /* try to get permission to use the buffer */
    if (dsm_client_ocuppy(lcd_dclient))
    {
        /* buffer is busy */
        LCDKIT_ERR(": buffer is busy!\n");
        return -1;
    }

    LCDKIT_INFO(": entry!\n");

    if (dsi_status[0])
        dsm_client_record(lcd_dclient,
                          "DSI_ACK_ERR_STATUS is wrong ,err number :%x\n", dsi_status[0]);

    if (dsi_status[1] & 0x0111)
        dsm_client_record(lcd_dclient,
                          "DSI_TIMEOUT_STATUS is wrong ,err number :%x\n", dsi_status[1]);

    if (dsi_status[2] & 0x011111)
        dsm_client_record(lcd_dclient,
                          "DSI_DLN0_PHY_ERR is wrong ,err number :%x\n", dsi_status[2]);

    //Disable check reg 00c because the register can not show dsi status accurately
    if (dsi_status[3] & 0xcccc4489)
    { return 0; }

    if (dsi_status[4] & 0x80000000)
        dsm_client_record(lcd_dclient,
                          "DSI_STATUS is wrong ,err number :%x\n", dsi_status[4]);

    dsm_client_notify(lcd_dclient, DSM_LCD_MDSS_DSI_ISR_ERROR_NO);

    return 0;
}

/* remove APR web LCD report log information  */
int lcdkit_report_dsm_err(int type, char* reg_name, int read_value, int expect_value)
{
    char* reg_string = (reg_name == NULL) ? "noinput" : reg_name;;
    /* we will ignore lcd error 20100 for 0x51 */
    if ((DSM_LCD_MIPI_ERROR_NO == type && 0x51 == expect_value)
        || DSM_LCD_MDSS_DSI_ISR_ERROR_NO == type)
    {
        return 0;
    }

    LCDKIT_INFO(": entry! type:%d\n", type);


    if ( NULL == lcd_dclient )
    {
        LCDKIT_ERR(": there is not lcd_dclient!\n");
        return -1;
    }

    /* try to get permission to use the buffer */
    if (dsm_client_ocuppy(lcd_dclient))
    {
        /* buffer is busy */
        LCDKIT_ERR(": buffer is busy!\n");
        return -1;
    }

    /* lcd report err according to err type */
    switch (type)
    {
        case DSM_LCD_DISPLAY_UNDERFLOW_ERROR_NO:
            dsm_client_record(lcd_dclient, "ldi underflow!\n");
            break;

        case DSM_LCD_TE_TIME_OUT_ERROR_NO:
            dsm_client_record(lcd_dclient, "TE time out!\n");
            break;

        case DSM_LCD_STATUS_ERROR_NO:
            dsm_client_record(lcd_dclient, "lcd register %s status wrong, read value :%x, but expect value: %x\n", reg_name, read_value, expect_value);
            break;

        case DSM_LCD_POWER_STATUS_ERROR_NO:
            dsm_client_record(lcd_dclient, "lcd power status error!\n");
            break;

            /* add for lcd esd */
        case DSM_LCD_ESD_STATUS_ERROR_NO:
            dsm_client_record(lcd_dclient, "lcd esd register %s status wrong, read value :%x, but expect value: %x\n", reg_name, read_value, expect_value);
            break;

        case DSM_LCD_ESD_RECOVERY_NO:
            dsm_client_record(lcd_dclient, "lcd esd recover happend!\n");
            break;

        case DSM_LCD_ESD_OCP_RECOVERY_NO:
            dsm_client_record(lcd_dclient,
                              "esd ocp happend, register %x status wrong, read value :%x, but expect value: %x\n",  reg_name, read_value, expect_value);
            break;

        case DSM_LCD_MIPI_ERROR_NO:
            dsm_client_record(lcd_dclient,
                              "mipi transmit register %x time out ,err number :%x\n",
                              add_value, err_value );
            break;

        case DSM_LCD_MDSS_IOMMU_ERROR_NO:
            dsm_client_record(lcd_dclient,
                              "mdss iommu attach/detach or map memory fail (%d)\n", err_value);

            if (!lcd_mdp_error_debug)
            {
                lcd_mdp_error_debug = 1;
                dsm_client_record(lcd_dclient,
                                  "cmd:@echo 1 >  /sys/kernel/debug/mdp/xlog/enable");
                dsm_client_record(lcd_dclient,
                                  "@echo 1 >  /sys/kernel/debug/mdp/xlog/reg_dump");
                dsm_client_record(lcd_dclient,
                                  "echo 1 >  /sys/kernel/debug/mdp/xlog/panic");
            }
            else
            {
                dsm_client_record(lcd_dclient,
                                  "cmd:@rm -rf /data/hwzd_logs/dmd_log/dmd_spcial_log/*");
                dsm_client_record(lcd_dclient,
                                  "cp /sys/kernel/debug/mdp/xlog/dump /data/hwzd_logs/dmd_spcial_log/debug_log/xlog_and_mdss_register.txt");
            }

            break;

        case DSM_LCD_MDSS_PIPE_ERROR_NO:
            dsm_client_record(lcd_dclient,
                              "mdss pipe status error (%d)\n", err_value);

            if (!lcd_mdp_error_debug)
            {
                lcd_mdp_error_debug = 1;
                dsm_client_record(lcd_dclient,
                                  "cmd:@echo 1 >  /sys/kernel/debug/mdp/xlog/enable");
                dsm_client_record(lcd_dclient,
                                  "@echo 1 >  /sys/kernel/debug/mdp/xlog/reg_dump");
                dsm_client_record(lcd_dclient,
                                  "echo 1 >  /sys/kernel/debug/mdp/xlog/panic");
            }
            else
            {
                dsm_client_record(lcd_dclient,
                                  "cmd:@rm -rf /data/hwzd_logs/dmd_log/dmd_spcial_log/*");
                dsm_client_record(lcd_dclient,
                                  "cp /sys/kernel/debug/mdp/xlog/dump /data/hwzd_logs/dmd_spcial_log/debug_log/xlog_and_mdss_register.txt");
            }

            break;

        case DSM_LCD_MDSS_PINGPONG_ERROR_NO:
            dsm_client_record(lcd_dclient,
                              "mdss wait pingpong time out (%d)\n", err_value);

            if (!lcd_mdp_error_debug)
            {
                lcd_mdp_error_debug = 1;
                dsm_client_record(lcd_dclient,
                                  "cmd:@echo 1 >  /sys/kernel/debug/mdp/xlog/enable");
                dsm_client_record(lcd_dclient,
                                  "@echo 1 >  /sys/kernel/debug/mdp/xlog/reg_dump");
                dsm_client_record(lcd_dclient,
                                  "echo 1 >  /sys/kernel/debug/mdp/xlog/panic");
            }
            else
            {
                dsm_client_record(lcd_dclient,
                                  "cmd:@rm -rf /data/hwzd_logs/dmd_log/dmd_spcial_log/*");
                dsm_client_record(lcd_dclient,
                                  "cp /sys/kernel/debug/mdp/xlog/dump /data/hwzd_logs/dmd_spcial_log/debug_log/xlog_and_mdss_register.txt");
            }

            break;

        case DSM_LCD_MDSS_VSP_ERROR_NO:
            dsm_client_record(lcd_dclient,
                              "get vsp/vsn(%d) register fail (%d) \n", expect_value, read_value);
            break;

        case DSM_LCD_MDSS_ROTATOR_ERROR_NO:
            dsm_client_record(lcd_dclient,
                              "mdss rotator queue fail (%d) \n", read_value);

            if (!lcd_mdp_error_debug)
            {
                lcd_mdp_error_debug = 1;
                dsm_client_record(lcd_dclient,
                                  "cmd:@echo 1 >  /sys/kernel/debug/mdp/xlog/enable");
                dsm_client_record(lcd_dclient,
                                  "@echo 1 >  /sys/kernel/debug/mdp/xlog/reg_dump");
                dsm_client_record(lcd_dclient,
                                  "echo 1 >  /sys/kernel/debug/mdp/xlog/panic");
            }
            else
            {
                dsm_client_record(lcd_dclient,
                                  "cmd:@rm -rf /data/hwzd_logs/dmd_log/dmd_spcial_log/*");
                dsm_client_record(lcd_dclient,
                                  "cp /sys/kernel/debug/mdp/xlog/dump /data/hwzd_logs/dmd_log/dmd_spcial_log/xlog_and_mdss_register.txt");
            }

            break;

        case DSM_LCD_MDSS_FENCE_ERROR_NO:
            dsm_client_record(lcd_dclient,
                              "mdss sync_fence_wait fail (%d) \n", read_value);

            if (!lcd_mdp_error_debug)
            {
                lcd_mdp_error_debug = 1;
                dsm_client_record(lcd_dclient,
                                  "cmd:@echo 1 >  /sys/kernel/debug/mdp/xlog/enable");
                dsm_client_record(lcd_dclient,
                                  "@echo 1 >  /sys/kernel/debug/mdp/xlog/reg_dump");
                dsm_client_record(lcd_dclient,
                                  "echo 1 >  /sys/kernel/debug/mdp/xlog/panic");
            }
            else
            {
                dsm_client_record(lcd_dclient,
                                  "cmd:@rm -rf /data/hwzd_logs/dmd_log/dmd_spcial_log/*");
                dsm_client_record(lcd_dclient,
                                  "cp /sys/kernel/debug/mdp/xlog/dump /data/hwzd_logs/dmd_log/dmd_spcial_log/xlog_and_mdss_register.txt");
            }

            break;

        case DSM_LCD_MDSS_CMD_STOP_ERROR_NO:
            dsm_client_record(lcd_dclient,
                              "mdss stop cmd time out (%d) \n", read_value);

            if (!lcd_mdp_error_debug)
            {
                lcd_mdp_error_debug = 1;
                dsm_client_record(lcd_dclient,
                                  "cmd:@echo 1 >  /sys/kernel/debug/mdp/xlog/enable");
                dsm_client_record(lcd_dclient,
                                  "@echo 1 >  /sys/kernel/debug/mdp/xlog/reg_dump");
                dsm_client_record(lcd_dclient,
                                  "echo 1 >  /sys/kernel/debug/mdp/xlog/panic");
            }
            else
            {
                dsm_client_record(lcd_dclient,
                                  "cmd:@rm -rf /data/hwzd_logs/dmd_log/dmd_spcial_log/*");
                dsm_client_record(lcd_dclient,
                                  "cp /sys/kernel/debug/mdp/xlog/dump /data/hwzd_logs/dmd_log/dmd_spcial_log/xlog_and_mdss_register.txt");
            }

            break;

        case DSM_LCD_MDSS_VIDEO_DISPLAY_ERROR_NO:
            dsm_client_record(lcd_dclient,
                              "mdss commit without wait! ctl=%d", read_value);

            if (!lcd_mdp_error_debug)
            {
                lcd_mdp_error_debug = 1;
                dsm_client_record(lcd_dclient,
                                  "cmd:@echo 1 >  /sys/kernel/debug/mdp/xlog/enable");
                dsm_client_record(lcd_dclient,
                                  "@echo 1 >  /sys/kernel/debug/mdp/xlog/reg_dump");
                dsm_client_record(lcd_dclient,
                                  "echo 1 >  /sys/kernel/debug/mdp/xlog/panic");
            }
            else
            {
                dsm_client_record(lcd_dclient,
                                  "cmd:@rm -rf /data/hwzd_logs/dmd_log/dmd_spcial_log/*");
                dsm_client_record(lcd_dclient,
                                  "cp /sys/kernel/debug/mdp/xlog/dump /data/hwzd_logs/dmd_log/dmd_spcial_log/xlog_and_mdss_register.txt");
            }

            break;

        case DSM_LCD_MDSS_MDP_CLK_ERROR_NO:
            dsm_client_record(lcd_dclient,
                              "mdss mdp clk can't be turned off\n", read_value);

            if (!lcd_mdp_error_debug)
            {
                lcd_mdp_error_debug = 1;
                dsm_client_record(lcd_dclient,
                                  "cmd:@echo 1 >  /sys/kernel/debug/mdp/xlog/enable");
                dsm_client_record(lcd_dclient,
                                  "@echo 1 >  /sys/kernel/debug/mdp/xlog/reg_dump");
                dsm_client_record(lcd_dclient,
                                  "echo 1 >  /sys/kernel/debug/mdp/xlog/panic");
            }
            else
            {
                dsm_client_record(lcd_dclient,
                                  "cmd:@rm -rf /data/hwzd_logs/dmd_log/dmd_spcial_log/*");
                dsm_client_record(lcd_dclient,
                                  "cp /sys/kernel/debug/mdp/xlog/dump /data/hwzd_logs/dmd_log/dmd_spcial_log/xlog_and_mdss_register.txt");
            }

            break;

        case DSM_LCD_MDSS_MDP_BUSY_ERROR_NO:
            dsm_client_record(lcd_dclient,
                              "mdss mdp dma tx time out (%d)\n", read_value);

            if (!lcd_mdp_error_debug)
            {
                lcd_mdp_error_debug = 1;
                dsm_client_record(lcd_dclient,
                                  "cmd:@echo 1 >  /sys/kernel/debug/mdp/xlog/enable");
                dsm_client_record(lcd_dclient,
                                  "@echo 1 >  /sys/kernel/debug/mdp/xlog/reg_dump");
                dsm_client_record(lcd_dclient,
                                  "echo 1 >  /sys/kernel/debug/mdp/xlog/panic");
            }
            else
            {
                dsm_client_record(lcd_dclient,
                                  "cmd:@rm -rf /data/hwzd_logs/dmd_log/dmd_spcial_log/*");
                dsm_client_record(lcd_dclient,
                                  "cp /sys/kernel/debug/mdp/xlog/dump /data/hwzd_logs/dmd_log/dmd_spcial_log/xlog_and_mdss_register.txt");
            }

            break;

        default:
            break;
    }

    dsm_client_notify(lcd_dclient, type);

    return 0;
}

/* remove APR web LCD report log information  */

/*
*
*bit 0  do unblank
*bit 1  lcd on
*bit 2  set frame
*bit 3  set backlgiht
*/
/*if did the operation the bit will be set to 1 or the bit is 0*/
void lcd_dcm_pwr_status_handler(unsigned long data)
{
    if (lcd_pwr_status.lcd_dcm_pwr_status != LCD_PWR_STAT_GOOD)
    {
        show_state_filter(TASK_UNINTERRUPTIBLE);
        dsm_client_record(lcd_dclient, "lcd power status wrong, value :%x\n",
                          lcd_pwr_status.lcd_dcm_pwr_status);
        dsm_client_record(lcd_dclient, "lcd power status :bit 0  do unblank\n");
        dsm_client_record(lcd_dclient, "lcd power status :bit 1  lcd on\n");
        dsm_client_record(lcd_dclient, "lcd power status :bit 2  set frame\n");
        dsm_client_record(lcd_dclient, "lcd power status :bit 3  set backlgiht\n");
        dsm_client_record(lcd_dclient,
                          "lcd power status :if did the operation the bit will be set to 1 or the bit is 0\n");
        dsm_client_record(lcd_dclient, "unblank at [%d-%d-%d]%d:%d:%d:%d\n",
                          lcd_pwr_status.tm_unblank.tm_year + 1900, lcd_pwr_status.tm_unblank.tm_mon + 1,
                          lcd_pwr_status.tm_unblank.tm_mday, lcd_pwr_status.tm_unblank.tm_hour,
                          lcd_pwr_status.tm_unblank.tm_min, lcd_pwr_status.tm_unblank.tm_sec,
                          lcd_pwr_status.tvl_unblank.tv_usec % 1000);
        dsm_client_record(lcd_dclient, "lcd on at [%d-%d-%d]%d:%d:%d:%d\n",
                          lcd_pwr_status.tm_lcd_on.tm_year + 1900, lcd_pwr_status.tm_lcd_on.tm_mon + 1,
                          lcd_pwr_status.tm_lcd_on.tm_mday, lcd_pwr_status.tm_lcd_on.tm_hour,
                          lcd_pwr_status.tm_lcd_on.tm_min, lcd_pwr_status.tm_lcd_on.tm_sec,
                          lcd_pwr_status.tvl_lcd_on.tv_usec % 1000);
        dsm_client_record(lcd_dclient, "set frame at [%d-%d-%d]%d:%d:%d:%d\n",
                          lcd_pwr_status.tm_set_frame.tm_year + 1900, lcd_pwr_status.tm_set_frame.tm_mon + 1,
                          lcd_pwr_status.tm_set_frame.tm_mday, lcd_pwr_status.tm_set_frame.tm_hour,
                          lcd_pwr_status.tm_set_frame.tm_min, lcd_pwr_status.tm_set_frame.tm_sec,
                          lcd_pwr_status.tvl_set_frame.tv_usec % 1000);
        dsm_client_record(lcd_dclient, "set backlight at [%d-%d-%d]%d:%d:%d:%d\n",
                          lcd_pwr_status.tm_backlight.tm_year + 1900, lcd_pwr_status.tm_backlight.tm_mon + 1,
                          lcd_pwr_status.tm_backlight.tm_mday, lcd_pwr_status.tm_backlight.tm_hour,
                          lcd_pwr_status.tm_backlight.tm_min, lcd_pwr_status.tm_backlight.tm_sec,
                          lcd_pwr_status.tvl_backlight.tv_usec % 1000);
        dsm_client_notify(lcd_dclient, DSM_LCD_POWER_STATUS_ERROR_NO);
    }
}

/* remove APR web LCD report log information  */
void lcdkit_underrun_dsm_report(unsigned long num,unsigned long underrun_cnt,
            int cpu_freq,unsigned long clk_axi,unsigned long clk_ahb)
{
    /* try to get permission to use the buffer */
    if (dsm_client_ocuppy(lcd_dclient))
    {
        /* buffer is busy */
        LCDKIT_ERR(": buffer is busy!\n");
        return;
    }

    dsm_client_record(lcd_dclient,
                      "Lcd underrun detected for ctl=%d,count=%d \n", num, underrun_cnt);

    if (!lcd_mdp_error_debug)
    {
        lcd_mdp_error_debug = 1;
        dsm_client_record(lcd_dclient,
                          "cmd:@echo 1 >  /sys/kernel/debug/mdp/xlog/enable");
        dsm_client_record(lcd_dclient,
                          "@echo 1 >  /sys/kernel/debug/mdp/xlog/reg_dump");
        dsm_client_record(lcd_dclient,
                          "@echo 1 >  /sys/kernel/debug/mdp/xlog/panic");
        dsm_client_record(lcd_dclient,
                          "@echo 1 > /sys/kernel/debug/tracing/events/mdss/mdp_perf_update_bus/enable");
        dsm_client_record(lcd_dclient,
                          "@echo 1 > /sys/kernel/debug/tracing/events/mdss/mdp_commit/enable");
        dsm_client_record(lcd_dclient,
                          "@echo 1 > /sys/kernel/debug/tracing/events/mdss/mdp_sspp_set/enable");
        dsm_client_record(lcd_dclient,
                          "@echo 1 > /sys/kernel/debug/tracing/events/mdss/mdp_sspp_change/enable");
        dsm_client_record(lcd_dclient,
                          "@echo 1 > /sys/kernel/debug/tracing/events/mdss/mdp_video_underrun_done/enable");
        dsm_client_record(lcd_dclient,
                          "@echo 1 > /sys/kernel/debug/tracing/events/mdss/mdp_mixer_update/enable");
        dsm_client_record(lcd_dclient,
                          "@echo 1 > /sys/kernel/debug/events/power/clock_set_rate/enable");
        dsm_client_record(lcd_dclient,
                          "@echo 1 > /sys/kernel/debug/tracing/events/msm_bus/bus_update_request/enable");
        dsm_client_record(lcd_dclient,
                          "@echo 1 > /sys/kernel/debug/tracing/events/msm_bus/bus_agg_bw/enable");
    }
    else
    {
        dsm_client_record(lcd_dclient,
                          "cmd:@rm -rf /data/hwzd_logs/dmd_log/dmd_spcial_log/*");
        dsm_client_record(lcd_dclient,
                          "cp /sys/kernel/debug/mdp/xlog/dump /data/hwzd_logs/dmd_log/dmd_spcial_log/xlog_and_mdss_register.txt");
        dsm_client_record(lcd_dclient,
                          "cp /sys/kernel/debug/tracing/trace /data/hwzd_logs/dmd_log/dmd_spcial_log/trace_log");
    }

    dsm_client_notify(lcd_dclient, DSM_LCD_DISPLAY_UNDERFLOW_ERROR_NO);
}

#endif

