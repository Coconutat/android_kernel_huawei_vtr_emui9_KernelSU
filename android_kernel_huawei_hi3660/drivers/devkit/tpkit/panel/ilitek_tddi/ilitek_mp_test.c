/*
 * ILITEK Touch IC driver
 *
 * Copyright (C) 2011 ILI Technology Corporation.
 *
 * Author: Dicky Chiang <dicky_chiang@ilitek.com>
 * Based on TDD v7.0 implemented by Mstar & ILITEK
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */
#include "ilitek_common.h"
#include "ilitek_protocol.h"
#include "ilitek_config.h"
#include "ilitek_mp_test.h"
#include "ilitek_parser.h"
#include "ilitek_test.h"

#define EXEC_READ                              0
#define EXEC_WRITE                             1

#define INT_CHECK                              0
#define POLL_CHECK                             1
#define DELAY_CHECK                            2

#define RETRY_COUNT                            1

#define CSV_FILE_SIZE                          (400 * 1024)

#define Mathabs(x) ({                        \
        long ret;                            \
        if (sizeof(x) == sizeof(long)) {    \
        long __x = (x);                        \
        ret = (__x < 0) ? -__x : __x;        \
        } else {                            \
        int __x = (x);                        \
        ret = (__x < 0) ? -__x : __x;        \
        }                                    \
        ret;                                \
    })

#define DUMP(level, fmt, arg...)        \
    do {                                \
        if(level & ilitek_debug_level)    \
        printk(KERN_CONT fmt, ##arg);                \
    } while (0)

enum mp_test_catalog {
    MUTUAL_TEST = 0,
    SELF_TEST = 1,
    KEY_TEST = 2,
    ST_TEST = 3,
    TX_RX_DELTA = 4,
    UNTOUCH_P2P = 5,
    PIXEL = 6,
    OPEN_TEST = 7,
    PEAK_TO_PEAK_TEST = 8,
    SHORT_TEST = 9,
};

enum open_test_node_type {
    NO_COMPARE = 0x00,  //Not A Area, No Compare
    AA_Area = 0x01,        //AA Area, Compare using Charge_AA
    Border_Area = 0x02, //Border Area, Compare using Charge_Border
    Notch = 0x04,       //Notch Area, Compare using Charge_Notch
    Round_Corner = 0x08 //Round Corner, No Compare
};

/* You must declare a new test in this struct before running a new process of mp test */
struct mp_test_items tItems[] = {
    {.name = "mutual_dac", .desp = "Calibration Data(DAC)", .result = "FAIL", .catalog = MUTUAL_TEST},
    {.name = "mutual_bg", .desp = "Baseline Data(BG)", .result = "FAIL", .catalog = MUTUAL_TEST},
    {.name = "mutual_signal", .desp = "Untouch Signal Data(BG-Raw-4096) - Mutual", .result = "FAIL", .catalog = MUTUAL_TEST},
    {.name = "mutual_no_bk", .desp = "Raw Data(No BK)", .result = "FAIL", .catalog = MUTUAL_TEST},
    {.name = "mutual_no_bk_lcm_off", .desp = "Raw Data(No BK) (LCM OFF)", .result = "FAIL", .catalog = MUTUAL_TEST},
    {.name = "mutual_has_bk", .desp = "Raw Data(Have BK)", .result = "FAIL", .catalog = MUTUAL_TEST},
    {.name = "mutual_has_bk_lcm_off", .desp = "Raw Data(Have BK) (LCM OFF)", .result = "FAIL", .catalog = MUTUAL_TEST},
    {.name = "mutual_bk_dac", .desp = "Manual BK Data(Mutual)", .result = "FAIL", .catalog = MUTUAL_TEST},

    {.name = "self_dac", .desp = "Calibration Data(DAC) - Self", .result = "FAIL", .catalog = SELF_TEST},
    {.name = "self_bg", .desp = "Baselin Data(BG,Self_Tx,Self_Rx)", .result = "FAIL", .catalog = SELF_TEST},
    {.name = "self_signal", .desp = "Untouch Signal Data(BG–Raw-4096) - Self", .result = "FAIL", .catalog = SELF_TEST},
    {.name = "self_no_bk", .desp = "Raw Data(No BK) - Self", .result = "FAIL", .catalog = SELF_TEST},
    {.name = "self_has_bk", .desp = "Raw Data(Have BK) - Self", .result = "FAIL", .catalog = SELF_TEST},
    {.name = "self_bk_dac", .desp = "Manual BK DAC Data(Self_Tx,Self_Rx)", .result = "FAIL", .catalog = SELF_TEST},

    {.name = "key_dac", .desp = "Calibration Data(DAC/ICON)", .result = "FAIL", .catalog = KEY_TEST},
    {.name = "key_bg", .desp = "Key Baseline Data", .result = "FAIL", .catalog = KEY_TEST},
    {.name = "key_no_bk", .desp = "Key Raw Data", .result = "FAIL", .catalog = KEY_TEST},
    {.name = "key_has_bk", .desp = "Key Raw BK DAC", .result = "FAIL", .catalog = KEY_TEST},
    {.name = "key_open", .desp = "Key Raw Open Test", .result = "FAIL", .catalog = KEY_TEST},
    {.name = "key_short", .desp = "Key Raw Short Test", .result = "FAIL", .catalog = KEY_TEST},

    {.name = "st_dac", .desp = "ST Calibration Data(DAC)", .result = "FAIL", .catalog = ST_TEST},
    {.name = "st_bg", .desp = "ST Baseline Data(BG)", .result = "FAIL", .catalog = ST_TEST},
    {.name = "st_no_bk", .desp = "ST Raw Data(No BK)", .result = "FAIL", .catalog = ST_TEST},
    {.name = "st_has_bk", .desp = "ST Raw(Have BK)", .result = "FAIL", .catalog = ST_TEST},
    {.name = "st_open", .desp = "ST Open Data", .result = "FAIL", .catalog = ST_TEST},

    {.name = "tx_short", .desp = "Tx Short Test", .result = "FAIL", .catalog = MUTUAL_TEST},
    {.name = "rx_short", .desp = "Short Test -ILI9881", .result = "FAIL", .catalog = SHORT_TEST},
    {.name = "rx_open", .desp = "RX Open", .result = "FAIL", .catalog = MUTUAL_TEST},

    {.name = "cm_data", .desp = "Untouch Cm Data", .result = "FAIL", .catalog = MUTUAL_TEST},
    {.name = "cs_data", .desp = "Untouch Cs Data", .result = "FAIL", .catalog = MUTUAL_TEST},

    {.name = "tx_rx_delta", .desp = "Tx/Rx Delta", .result = "FAIL", .catalog = TX_RX_DELTA},

    {.name = "p2p", .desp = "Untouch Peak to Peak", .result = "FAIL", .catalog = UNTOUCH_P2P},

    {.name = "pixel_no_bk", .desp = "Pixel Raw (No BK)", .result = "FAIL", .catalog = PIXEL},
    {.name = "pixel_has_bk", .desp = "Pixel Raw (Have BK)", .result = "FAIL", .catalog = PIXEL},

    {.name = "open_integration", .desp = "Open Test(integration)", .result = "FAIL", .catalog = OPEN_TEST},
    {.name = "open_integration_sp", .desp = "Open Test(integration)_SP", .result = "FAIL", .catalog = OPEN_TEST},
    {.name = "open_cap", .desp = "Open Test(Cap)", .result = "FAIL", .catalog = OPEN_TEST},

    {.name = "noise_peak_to_peak_ic", .desp = "Noise Peak to Peak(IC Only)", .result = "FAIL", .catalog = PEAK_TO_PEAK_TEST},
    {.name = "noise_peak_to_peak_ic_lcm_off", .desp = "Noise Peak to Peak(IC Only) (LCM OFF)", .result = "FAIL", .catalog = PEAK_TO_PEAK_TEST},
    {.name = "noise_peak_to_peak_panel", .desp = "Noise Peak To Peak(With Panel)", .result = "FAIL", .catalog = PEAK_TO_PEAK_TEST},
    {.name = "noise_peak_to_peak_panel_lcm_off", .desp = "Noise Peak to Peak(With Panel) (LCM OFF)", .result = "FAIL", .catalog = PEAK_TO_PEAK_TEST},

    {.name = "doze_raw", .desp = "Doze Raw Data", .result = "FAIL", .catalog = MUTUAL_TEST},
    {.name = "doze_p2p", .desp = "Doze Peak To Peak", .result = "FAIL", .catalog = PEAK_TO_PEAK_TEST},
    {.name = "doze_raw_td_lcm_off", .desp = "Raw Data_TD(LCM OFF)", .result = "FAIL", .catalog = MUTUAL_TEST},
    {.name = "doze_p2p_td_lcm_off", .desp = "Peak To Peak_TD(LCM OFF)", .result = "FAIL", .catalog = PEAK_TO_PEAK_TEST},
};

s32 *frame_buf = NULL;
s32 *key_buf = NULL;
int combined_flag = 0;
struct core_mp_test_data *core_mp = NULL;

void dump_data(void *data, int type, int len, int row_len, const char *name)
{
    int i, row = 31;
    u8 *p8 = NULL;
    s32 *p32 = NULL;

    if(row_len > 0)
        row = row_len;

    if (ilitek_debug_level & DEBUG_MP_TEST) {
        if (data == NULL) {
            ilitek_err("The data going to dump is NULL\n");
            return;
        }

        printk("\n\n");
        printk("Dump %s data\n", name);

        if (type == 8)
            p8 = (u8 *) data;
        if (type == 32 || type == 10)
            p32 = (s32 *) data;

        for (i = 0; i < len; i++) {
            if (type == 8)
                printk(KERN_CONT " %4x ", p8[i]);
            else if (type == 32)
                printk(KERN_CONT " %4x ", p32[i]);
            else if (type == 10)
                printk(KERN_CONT " %4d ", p32[i]);
            if ((i % row) == row-1)
                printk("\n");
        }
        printk("\n\n");
    }
}
static void dump_benchmark_data(s32* max_ptr, s32* min_ptr)
{
    int i;

    if (ilitek_debug_level & DEBUG_MP_TEST) {
        ilitek_info("benchmark max\n");

        for(i = 0; i < core_mp->frame_len; i++) {
            printk(KERN_CONT "%d, ", max_ptr[i]);
            if(i % core_mp->xch_len == core_mp->xch_len - 1)
                printk("\n");
        }

        ilitek_info("benchmark min\n");

        for(i = 0; i < core_mp->frame_len; i++) {
            printk(KERN_CONT "%d, ", min_ptr[i]);
            if(i % core_mp->xch_len == core_mp->xch_len - 1)
                printk("\n");
        }
    }
}

void dump_node_type_buffer(s32* node_ptr, u8 *name)
{
    int i;

    if (ilitek_debug_level & DEBUG_MP_TEST) {
        ilitek_info("Node Type buffer: %s\n", name);
        for(i=0; i<core_mp->frame_len ; i++)
        {
            printk(KERN_CONT "%d, ",node_ptr[i]);
            if(i % core_mp->xch_len == core_mp->xch_len-1)
                printk("\n");
        }
    }

}

static void mp_show_sorting_result(int index, bool max, bool tx, char *csv, int *csv_len)
{
    int x, y, tmp_len = *csv_len;
    int  mp_result = MP_PASS;
    char *line_breaker = "\n";
    s32 *tmp = NULL;

    pr_info("%s\n", tItems[index].desp);
    tmp_len += sprintf(csv + tmp_len, " %s ", tItems[index].desp);

    pr_info("\n");
    tmp_len += sprintf(csv + tmp_len, "%s", line_breaker);

    pr_info("Max = %d\n",tItems[index].max);
    tmp_len += sprintf(csv + tmp_len, "Max = %d", tItems[index].max);

    pr_info("\n");
    tmp_len += sprintf(csv + tmp_len, "%s", line_breaker);

    pr_info("Min = %d\n",tItems[index].min);
    tmp_len += sprintf(csv + tmp_len, "Min = %d", tItems[index].min);

    pr_info("\n");
    tmp_len += sprintf(csv + tmp_len, "%s", line_breaker);

    pr_info("Frame count = %d\n",tItems[index].frame_count);
    tmp_len += sprintf(csv + tmp_len, "Frame count = %d", tItems[index].frame_count);

    pr_info("\n");
    tmp_len += sprintf(csv + tmp_len, "%s", line_breaker);

    DUMP(DEBUG_MP_TEST, " %s \n", "Sorting result");
    tmp_len += sprintf(csv + tmp_len, "Sorting result\n");

    tmp = tItems[index].buf;
    /* print X raw only */
    for (x = 0; x < core_mp->xch_len; x++) {
        if (x == 0) {
            DUMP(DEBUG_MP_TEST, "           ");
            tmp_len += sprintf(csv + tmp_len, ",");
        }

        DUMP(DEBUG_MP_TEST, "X%02d      ", x);
        tmp_len += sprintf(csv + tmp_len, "X%02d, ", x);
    }

    DUMP(DEBUG_MP_TEST, "\n");
    tmp_len += sprintf(csv + tmp_len, "%s", line_breaker);

    for (y = 0; y < core_mp->ych_len; y++) {
        DUMP(DEBUG_MP_TEST, " Y%02d ", y);
        tmp_len += sprintf(csv + tmp_len, "Y%02d, ", y);

        for (x = 0; x < core_mp->xch_len; x++) {
            int shift = y * core_mp->xch_len + x;

            if (tmp[shift] <= tItems[index].max && tmp[shift] >=  tItems[index].min) {
                DUMP(DEBUG_MP_TEST, " %7d ", tmp[shift]);
                tmp_len += sprintf(csv + tmp_len, " %7d, ", tmp[shift]);
            } else {
                if (tmp[shift] > tItems[index].max) {
                    DUMP(DEBUG_MP_TEST, " *%7d ", tmp[shift]);
                    tmp_len += sprintf(csv + tmp_len, "*%7d,", tmp[shift]);
                } else {
                    DUMP(DEBUG_MP_TEST, " #%7d ", tmp[shift]);
                    tmp_len += sprintf(csv + tmp_len, "#%7d,", tmp[shift]);
                }
                mp_result = MP_FAIL;
            }
        }

        DUMP(DEBUG_MP_TEST, "\n");
        tmp_len += sprintf(csv + tmp_len, "%s", line_breaker);
    }

    if (mp_result == MP_PASS)
        sprintf(tItems[index].result, "%s", "PASS");
    else
        sprintf(tItems[index].result, "%s", "FAIL");


    pr_info("\n Result : %s\n", tItems[index].result);
    tmp_len += sprintf(csv + tmp_len, " Result : %s ", tItems[index].result);

    pr_info("\n");
    tmp_len += sprintf(csv + tmp_len, "%s", line_breaker);

    tmp_len += sprintf(csv + tmp_len, "%s", line_breaker);

    *csv_len = tmp_len;
}

static void mp_compare_benchmark_result(int index, bool max, bool tx, char *csv, int *csv_len)
{
    int x, y, tmp_len = *csv_len;
    int  mp_result = MP_PASS;
    char *line_breaker = "\n";
    s32 *tmp = NULL;

    pr_info("%s\n", tItems[index].desp);
    tmp_len += sprintf(csv + tmp_len, " %s ", tItems[index].desp);

    pr_info("\n");
    tmp_len += sprintf(csv + tmp_len, "%s", line_breaker);

    pr_info("Frame count = %d\n",tItems[index].frame_count);
    tmp_len += sprintf(csv + tmp_len, "Frame count = %d", tItems[index].frame_count);

    pr_info("\n");
    tmp_len += sprintf(csv + tmp_len, "%s", line_breaker);

    DUMP(DEBUG_MP_TEST, " %s \n", "Bench mark max threshold");
    tmp_len += sprintf(csv + tmp_len, "Bench mark max threshold\n");

    if (ERR_ALLOC_MEM(tItems[index].buf) || ERR_ALLOC_MEM(tItems[index].bench_mark_max)
            || ERR_ALLOC_MEM(tItems[index].bench_mark_min)) {
        ilitek_err("This test item (%s) has no data inside its buffer\n", tItems[index].desp);
        mp_result = MP_FAIL;
        goto out;
    }

    tmp = tItems[index].bench_mark_max;
    /* print X raw only */
    for (x = 0; x < core_mp->xch_len; x++) {
        if (x == 0) {
            DUMP(DEBUG_MP_TEST, "           ");
            tmp_len += sprintf(csv + tmp_len, ",");
        }

        DUMP(DEBUG_MP_TEST, "X%02d      ", x);
        tmp_len += sprintf(csv + tmp_len, "X%02d, ", x);
    }

    DUMP(DEBUG_MP_TEST, "\n");
    tmp_len += sprintf(csv + tmp_len, "%s", line_breaker);

    for (y = 0; y < core_mp->ych_len; y++) {
        DUMP(DEBUG_MP_TEST, " Y%02d ", y);
        tmp_len += sprintf(csv + tmp_len, "Y%02d, ", y);

        for (x = 0; x < core_mp->xch_len; x++) {
            int shift = y * core_mp->xch_len + x;
            if(tmp[shift] == INT_MAX)
            {
                DUMP(DEBUG_MP_TEST, "%s", "BYPASS,");
                tmp_len += sprintf(csv + tmp_len,"BYPASS,");
            }
            else{
                DUMP(DEBUG_MP_TEST, " %7d ", tmp[shift]);
                tmp_len += sprintf(csv + tmp_len, " %7d, ", tmp[shift]);
            }
        }
        DUMP(DEBUG_MP_TEST, "\n");
        tmp_len += sprintf(csv + tmp_len, "%s", line_breaker);
    }

    DUMP(DEBUG_MP_TEST, " %s \n", "Bench mark min threshold");
    tmp_len += sprintf(csv + tmp_len, "Bench mark min threshold\n");

    tmp = tItems[index].bench_mark_min;
    /* print X raw only */
    for (x = 0; x < core_mp->xch_len; x++) {
        if (x == 0) {
            DUMP(DEBUG_MP_TEST, "           ");
            tmp_len += sprintf(csv + tmp_len, ",");
        }

        DUMP(DEBUG_MP_TEST, "X%02d      ", x);
        tmp_len += sprintf(csv + tmp_len, "X%02d, ", x);
    }

    DUMP(DEBUG_MP_TEST, "\n");
    tmp_len += sprintf(csv + tmp_len, "%s", line_breaker);

    for (y = 0; y < core_mp->ych_len; y++) {
        DUMP(DEBUG_MP_TEST, " Y%02d ", y);
        tmp_len += sprintf(csv + tmp_len, "Y%02d, ", y);

        for (x = 0; x < core_mp->xch_len; x++) {
            int shift = y * core_mp->xch_len + x;
            if(tmp[shift] == INT_MIN)
            {
                DUMP(DEBUG_MP_TEST, " %s", "BYPASS,");
                tmp_len += sprintf(csv + tmp_len,"BYPASS,");
            }
            else{
                DUMP(DEBUG_MP_TEST, " %7d ", tmp[shift]);
                tmp_len += sprintf(csv + tmp_len, " %7d, ", tmp[shift]);
            }
        }
        DUMP(DEBUG_MP_TEST, "\n");
        tmp_len += sprintf(csv + tmp_len, "%s", line_breaker);
    }

    DUMP(DEBUG_MP_TEST, " %s \n", "Bench mark data");
    tmp_len += sprintf(csv + tmp_len, "Bench mark data\n");

    tmp = tItems[index].buf;
    /* print X raw only */
    for (x = 0; x < core_mp->xch_len; x++) {
        if (x == 0) {
            DUMP(DEBUG_MP_TEST, "           ");
            tmp_len += sprintf(csv + tmp_len, ",");
        }

        DUMP(DEBUG_MP_TEST, "X%02d      ", x);
        tmp_len += sprintf(csv + tmp_len, "X%02d, ", x);
    }

    DUMP(DEBUG_MP_TEST, "\n");
    tmp_len += sprintf(csv + tmp_len, "%s", line_breaker);

    for (y = 0; y < core_mp->ych_len; y++) {
        DUMP(DEBUG_MP_TEST, " Y%02d ", y);
        tmp_len += sprintf(csv + tmp_len, "Y%02d, ", y);

        for (x = 0; x < core_mp->xch_len; x++) {
            int shift = y * core_mp->xch_len + x;

            if (tmp[shift] <= tItems[index].bench_mark_max[shift] && tmp[shift] >=  tItems[index].bench_mark_min[shift]) {
                DUMP(DEBUG_MP_TEST, " %7d ", tmp[shift]);
                tmp_len += sprintf(csv + tmp_len, " %7d, ", tmp[shift]);
            } else {
                if (tmp[shift] > tItems[index].bench_mark_max[shift]) {
                    DUMP(DEBUG_MP_TEST, " *%7d ", tmp[shift]);
                    tmp_len += sprintf(csv + tmp_len, "*%7d,", tmp[shift]);
                } else {
                    DUMP(DEBUG_MP_TEST, " #%7d ", tmp[shift]);
                    tmp_len += sprintf(csv + tmp_len, "#%7d,", tmp[shift]);
                }
                mp_result = MP_FAIL;
            }
        }

        DUMP(DEBUG_MP_TEST, "\n");
        tmp_len += sprintf(csv + tmp_len, "%s", line_breaker);
    }

out:
    if (mp_result == MP_PASS)
        sprintf(tItems[index].result, "%s", "PASS");
    else
        sprintf(tItems[index].result, "%s", "FAIL");


    pr_info("\n Result : %s\n", tItems[index].result);
    tmp_len += sprintf(csv + tmp_len, " Result : %s ", tItems[index].result);

    pr_info("\n");
    tmp_len += sprintf(csv + tmp_len, "%s", line_breaker);

    tmp_len += sprintf(csv + tmp_len, "%s", line_breaker);

    *csv_len = tmp_len;
}

static void mp_compare_cdc_result(int index, bool max, bool tx, char *csv, int *csv_len)
{
    int x, y, tmp_len = *csv_len;
    int max_ts, min_ts, mp_result = MP_PASS;
    char *line_breaker = "\n";
    s32 *tmp = NULL;

    if (tItems[index].catalog == TX_RX_DELTA) {
        if (tx) {
            if (max)
                tmp = core_mp->tx_max_buf;
            else
                tmp = core_mp->tx_min_buf;

            max_ts = core_mp->TxDeltaMax;
            min_ts = core_mp->TxDeltaMin;
        } else {
            if (max)
                tmp = core_mp->rx_max_buf;
            else
                tmp = core_mp->rx_min_buf;

            max_ts = core_mp->RxDeltaMax;
            min_ts = core_mp->RxDeltaMin;
        }
    } else {
        if (max)
            tmp = tItems[index].max_buf;
        else
            tmp = tItems[index].min_buf;

        max_ts = tItems[index].max;
        min_ts = tItems[index].min;
    }

    /* print X raw only */
    for (x = 0; x < core_mp->xch_len; x++) {
        if (x == 0) {
            DUMP(DEBUG_MP_TEST, "           ");
            tmp_len += sprintf(csv + tmp_len, ",");
        }

        DUMP(DEBUG_MP_TEST, "X%02d      ", x);
        tmp_len += sprintf(csv + tmp_len, "X%02d, ", x);
    }

    DUMP(DEBUG_MP_TEST, "\n");
    tmp_len += sprintf(csv + tmp_len, "%s", line_breaker);

    for (y = 0; y < core_mp->ych_len; y++) {
        DUMP(DEBUG_MP_TEST, " Y%02d ", y);
        tmp_len += sprintf(csv + tmp_len, "Y%02d, ", y);

        for (x = 0; x < core_mp->xch_len; x++) {
            int shift = y * core_mp->xch_len + x;

            if (tmp[shift] <= max_ts && tmp[shift] >= min_ts) {
                DUMP(DEBUG_MP_TEST, " %7d ", tmp[shift]);
                tmp_len += sprintf(csv + tmp_len, " %7d, ", tmp[shift]);
            } else {
                if (tmp[shift] > max_ts) {
                    DUMP(DEBUG_MP_TEST, " *%7d ", tmp[shift]);
                    tmp_len += sprintf(csv + tmp_len, "*%7d,", tmp[shift]);
                } else {
                    DUMP(DEBUG_MP_TEST, " #%7d ", tmp[shift]);
                    tmp_len += sprintf(csv + tmp_len, "#%7d,", tmp[shift]);
                }
                mp_result = MP_FAIL;
            }
        }

        DUMP(DEBUG_MP_TEST, "\n");
        tmp_len += sprintf(csv + tmp_len, "%s", line_breaker);
    }

    if (max) {
        if (mp_result == MP_PASS)
            tItems[index].max_res = MP_PASS;
        else
            tItems[index].max_res = MP_FAIL;
    } else {
        if (mp_result == MP_PASS)
            tItems[index].min_res = MP_PASS;
        else
            tItems[index].min_res = MP_FAIL;
    }

    *csv_len = tmp_len;
}

static int create_mp_test_frame_buffer(int index, int frame_count)
{
    ilitek_debug(DEBUG_MP_TEST, "Create MP frame buffers (index = %d)\n",index);

    if (tItems[index].catalog == TX_RX_DELTA) {
        core_mp->tx_delta_buf = kcalloc(core_mp->frame_len, sizeof(s32), GFP_KERNEL);
        core_mp->rx_delta_buf = kcalloc(core_mp->frame_len, sizeof(s32), GFP_KERNEL);

        core_mp->tx_max_buf = kcalloc(core_mp->frame_len, sizeof(s32), GFP_KERNEL);
        core_mp->tx_min_buf = kcalloc(core_mp->frame_len, sizeof(s32), GFP_KERNEL);

        core_mp->rx_max_buf = kcalloc(core_mp->frame_len, sizeof(s32), GFP_KERNEL);
        core_mp->rx_min_buf = kcalloc(core_mp->frame_len, sizeof(s32), GFP_KERNEL);

        if (ERR_ALLOC_MEM(core_mp->tx_delta_buf) || ERR_ALLOC_MEM(core_mp->rx_delta_buf)) {
            ilitek_err("Failed to allocate Tx/Rx Delta buffer\n");
            return -ENOMEM;
        }

        if (ERR_ALLOC_MEM(core_mp->tx_max_buf) || ERR_ALLOC_MEM(core_mp->tx_min_buf)) {
            ilitek_err("Failed to allocate Tx Max/Min buffer\n");
            return -ENOMEM;
        }

        if (ERR_ALLOC_MEM(core_mp->rx_max_buf) || ERR_ALLOC_MEM(core_mp->rx_min_buf)) {
            ilitek_err("Failed to allocate Rx Max/Min buffe\n");
            return -ENOMEM;
        }

    } else {
        if (tItems[index].buf == NULL && tItems[index].max_buf == NULL
                && tItems[index].min_buf == NULL) {
        tItems[index].buf = kcalloc(frame_count * core_mp->frame_len, sizeof(s32), GFP_KERNEL);
        tItems[index].max_buf = kcalloc(core_mp->frame_len, sizeof(s32), GFP_KERNEL);
        tItems[index].min_buf = kcalloc(core_mp->frame_len, sizeof(s32), GFP_KERNEL);
        }

        if (tItems[index].spec_option == BENCHMARK) {
            if (tItems[index].bench_mark_max == NULL && tItems[index].bench_mark_min == NULL) {
            tItems[index].bench_mark_max = kcalloc(core_mp->frame_len, sizeof(s32), GFP_KERNEL);
            tItems[index].bench_mark_min = kcalloc(core_mp->frame_len, sizeof(s32), GFP_KERNEL);
            }

            if (ERR_ALLOC_MEM(tItems[index].bench_mark_max) || ERR_ALLOC_MEM(tItems[index].bench_mark_min)){
                ilitek_err("Failed to allocate bench_mark FRAME buffer\n");
                return -ENOMEM;
            }
        }

        if (ERR_ALLOC_MEM(tItems[index].buf) || ERR_ALLOC_MEM(tItems[index].max_buf) ||
                ERR_ALLOC_MEM(tItems[index].min_buf)) {
            ilitek_err("Failed to allocate FRAME buffer\n");
            return -ENOMEM;
        }
    }

    return 0;
}

static int mp_ctrl_lcm_status(bool on)
{
    int ret = 0, ctrl = 0, delay = 0;
    u8 lcd[15] = {0};
    u8 header = 0x0F;
    struct protocol_test * p_test = g_ilitek_ts->pro->test;

    memset(&lcd, 0xFF, ARRAY_SIZE(lcd));

    ctrl = ((on) ? 1 : 2);
    delay = ((on) ? 100 : 10);

    lcd[0] = header;
    lcd[1] = p_test->mutual_bg;
    lcd[2] = 0;
    lcd[3] = ctrl;

    dump_data(lcd, 8, ARRAY_SIZE(lcd), 0, "LCM Command");

    ret = ilitek_i2c_write(lcd, ARRAY_SIZE(lcd));
    if (ret < 0) {
        ilitek_err("Failed to write LCM command\n");
        goto out;
    }

    mdelay(delay);

out:
    return ret;
}

static void mp_calc_nodp(bool long_v)
{
    u8 at, phase;
    u8 r2d, rst, rst_back, dac_td, qsh_pw, qsh_td, dn;
    uint16_t tshd, tsvd_to_tshd, qsh_tdf, dp2tp, twc, twcm, tp2dp;
    uint16_t multi_term_num, tp_tsdh_wait, ddi_width;
    uint32_t real_tx_dura, tmp, tmp1;

    ilitek_info("DDI Mode = %d\n", long_v);

    tshd = core_mp->nodp.tshd;
    tsvd_to_tshd = core_mp->nodp.tsvd_to_tshd;
    multi_term_num = core_mp->nodp.multi_term_num_120;
    qsh_tdf = core_mp->nodp.qsh_tdf;
    at = core_mp->nodp.auto_trim;
    tp_tsdh_wait = core_mp->nodp.tp_tshd_wait_120;
    ddi_width = core_mp->nodp.ddi_width_120;
    dp2tp = core_mp->nodp.dp_to_tp;
    twc = core_mp->nodp.tx_wait_const;
    twcm = core_mp->nodp.tx_wait_const_multi;
    tp2dp = core_mp->nodp.tp_to_dp;
    phase = core_mp->nodp.phase_adc;
    r2d = core_mp->nodp.r2d_pw;
    rst = core_mp->nodp.rst_pw;
    rst_back = core_mp->nodp.rst_pw_back;
    dac_td = core_mp->nodp.dac_td;
    qsh_pw = core_mp->nodp.qsh_pw;
    qsh_td = core_mp->nodp.qsh_td;
    dn = core_mp->nodp.drop_nodp;

    /* NODP formulation */
    if (!long_v) {
        if (core_mp->nodp.is60HZ) {
            multi_term_num = core_mp->nodp.multi_term_num_60;
            tp_tsdh_wait = core_mp->nodp.tp_tshd_wait_60;
            ddi_width = core_mp->nodp.ddi_width_60;
        }

        if (multi_term_num == 0)
            multi_term_num = 1;

        tmp = ((tshd << 2) - (at << 6) - tp_tsdh_wait - ddi_width * (multi_term_num - 1) - 64 - dp2tp - twc) * 5;
        tmp1 = (phase << 5) - ((twcm * 5 + (phase << 5) + (tp2dp * 5 << 6)) * (multi_term_num - 1));
        real_tx_dura = (tmp - tmp1) / (multi_term_num * 5);

        core_mp->nodp.first_tp_width = (dp2tp * 5  + twc * 5  + ( phase << 5 ) + real_tx_dura * 5 ) / 5;
        core_mp->nodp.tp_width = ( ( ( tp2dp * 10 + phase ) << 6 )  + real_tx_dura * 10 ) / 10;
        core_mp->nodp.txpw = ( qsh_tdf + rst + qsh_pw + qsh_td + 2 );

        if ( core_mp->nodp.txpw % 2 == 1 )
            core_mp->nodp.txpw = core_mp->nodp.txpw + 1;

        core_mp->nodp.nodp = real_tx_dura / core_mp->nodp.txpw / 2;
    } else {
        if (multi_term_num == 0)
            multi_term_num = 1;

        real_tx_dura = (((tshd << 2) - (at << 6) - ddi_width * (11) - 64 - dp2tp - twc) * 5 - (phase << 5) - ((twcm * 5 + (phase << 5) + (tp2dp * 5 << 6)) * (11))) / (12 * 5);

        core_mp->nodp.long_tsdh_wait = (tsvd_to_tshd + 10 ) << 6;

        core_mp->nodp.first_tp_width = (dp2tp * 5  + twc * 5  + ( phase << 5 ) + real_tx_dura * 5) / 5;
        core_mp->nodp.tp_width = (((tp2dp * 10 + phase ) << 6 )  + real_tx_dura * 10) / 10;
        core_mp->nodp.txpw = (qsh_tdf + rst + qsh_pw + qsh_td + 2);

        if ( core_mp->nodp.txpw % 2 == 1 )
            core_mp->nodp.txpw = core_mp->nodp.txpw + 1;

        core_mp->nodp.nodp = real_tx_dura / core_mp->nodp.txpw / 2;
    }

    ilitek_info("Read Tx Duration = %d\n",real_tx_dura);
    ilitek_info("First TP Width = %d\n",core_mp->nodp.first_tp_width);
    ilitek_info("TP Width = %d\n",core_mp->nodp.tp_width);
    ilitek_info("TXPW = %d\n",core_mp->nodp.txpw);
    ilitek_info("NODP = %d\n",core_mp->nodp.nodp);
}

static int allnode_key_cdc_data(int index)
{
    int i, res = 0, len = 0;
    int inDACp = 0, inDACn = 0;
    u8 cmd[3] = { 0 };
    u8 *ori = NULL;
    struct ilitek_protocol *p_pro = g_ilitek_ts->pro;
    struct protocol_test * p_test = g_ilitek_ts->pro->test;

    len = core_mp->key_len * 2;

    ilitek_debug(DEBUG_MP_TEST, "Read key's length = %d\n", len);
    ilitek_debug(DEBUG_MP_TEST, "core_mp->key_len = %d\n", core_mp->key_len);

    if (len <= 0) {
        ilitek_err("Length is invalid\n");
        res = -1;
        goto out;
    }

    /* CDC init */
    cmd[0] = p_test->cmd_cdc;
    cmd[1] = tItems[index].cmd;
    cmd[2] = 0;

    res = ilitek_i2c_write(cmd, 3);
    if (res < 0) {
        ilitek_err("I2C Write Error while initialising cdc\n");
        goto out;
    }

    mdelay(1);

    /* Check busy */
    if (ilitek_config_check_cdc_busy(50, 100) < 0) {
        ilitek_err("Check busy is timout !\n");
        res = -1;
        goto out;
    }

    mdelay(1);

    /* Prepare to get cdc data */
    cmd[0] = p_pro->cmd_read_ctrl;
    cmd[1] = p_test->cmd_get_cdc;

    res = ilitek_i2c_write(cmd, 2);
    if (res < 0) {
        ilitek_err("I2C Write Error\n");
        goto out;
    }

    mdelay(1);

    res = ilitek_i2c_write(&cmd[1], 1);
    if (res < 0) {
        ilitek_err("I2C Write Error\n");
        goto out;
    }

    /* Allocate a buffer for the original */
    ori = kcalloc(len, sizeof(u8), GFP_KERNEL);
    if (ERR_ALLOC_MEM(ori)) {
        ilitek_err("Failed to allocate ori mem (%ld)\n", PTR_ERR(ori));
        goto out;
    }

    mdelay(1);

    /* Get original frame(cdc) data */
    res = ilitek_i2c_read(ori, len);
    if (res < 0) {
        ilitek_err("I2C Read Error while getting original cdc data\n");
        goto out;
    }

    dump_data(ori, 8, len, 0, "Key CDC original");

    if (key_buf == NULL) {
        key_buf = kcalloc(core_mp->key_len, sizeof(s32), GFP_KERNEL);
        if (ERR_ALLOC_MEM(key_buf)) {
            ilitek_err("Failed to allocate FrameBuffer mem (%ld)\n", PTR_ERR(key_buf));
            goto out;
        }
    } else {
        memset(key_buf, 0x0, core_mp->key_len);
    }

    /* Convert original data to the physical one in each node */
    for (i = 0; i < core_mp->frame_len; i++) {
        if (tItems[index].cmd == p_test->key_dac) {
            /* DAC - P */
            if (((ori[(2 * i) + 1] & 0x80) >> 7) == 1) {
                /* Negative */
                inDACp = 0 - (int)(ori[(2 * i) + 1] & 0x7F);
            } else {
                inDACp = ori[(2 * i) + 1] & 0x7F;
            }

            /* DAC - N */
            if (((ori[(1 + (2 * i)) + 1] & 0x80) >> 7) == 1) {
                /* Negative */
                inDACn = 0 - (int)(ori[(1 + (2 * i)) + 1] & 0x7F);
            } else {
                inDACn = ori[(1 + (2 * i)) + 1] & 0x7F;
            }

            key_buf[i] = (inDACp + inDACn) / 2;
        }
    }

    dump_data(key_buf, 32, core_mp->frame_len, core_mp->xch_len, "Key CDC combined data");

out:
    ipio_kfree((void **)&ori);
    return res;
}

static int mp_calc_timing_nodp(void)
{
    int ret = 0;
    u8 test_type = 0x0;
    u8 timing_cmd[15] = {0};
    u8 get_timing[64] = {0};
    struct protocol_test *p_test = g_ilitek_ts->pro->test;

    memset(timing_cmd, 0xFF, sizeof(timing_cmd));

    timing_cmd[0] = p_test->cmd_cdc;
    timing_cmd[1] = p_test->get_timing;
    timing_cmd[2] = test_type;

    /*
     * To calculate NODP, we need to get timing parameters first from fw,
     * which returnes 40 bytes data.
     */
    dump_data(timing_cmd, 8, sizeof(timing_cmd), 0, "Timing command");

    ret = ilitek_i2c_write(timing_cmd, sizeof(timing_cmd));
    if (ret < 0) {
        ilitek_err("Failed to write timing command\n");
        goto out;
    }

    ret = ilitek_i2c_read(get_timing, sizeof(get_timing));
    if (ret < 0) {
        ilitek_err("Failed to read timing parameters\n");
        goto out;
    }

    dump_data(get_timing, 8, 41, 0, "Timing parameters (41bytes)");

    /* Combine timing data */
    core_mp->nodp.is60HZ = false; // This will get from ini file by default.
    core_mp->nodp.isLongV = get_timing[2];
    core_mp->nodp.tshd = (get_timing[3] << 8 ) + get_timing[4];
    core_mp->nodp.multi_term_num_120 = get_timing[5];
    core_mp->nodp.multi_term_num_60 = get_timing[6];
    core_mp->nodp.tsvd_to_tshd = (get_timing[7] << 8 ) + get_timing[8];
    core_mp->nodp.qsh_tdf = (get_timing[9] << 8 ) + get_timing[10];
    core_mp->nodp.auto_trim = get_timing[11];
    core_mp->nodp.tp_tshd_wait_120 = (get_timing[12] << 8 ) + get_timing[13];
    core_mp->nodp.ddi_width_120 = (get_timing[14] << 8 ) + get_timing[15];
    core_mp->nodp.tp_tshd_wait_60 = (get_timing[16] << 8 ) + get_timing[17];
    core_mp->nodp.ddi_width_60 = (get_timing[18] << 8 ) + get_timing[19];
    core_mp->nodp.dp_to_tp = (get_timing[20] << 8 ) + get_timing[21];
    core_mp->nodp.tx_wait_const = (get_timing[22] << 8 ) + get_timing[23];
    core_mp->nodp.tx_wait_const_multi = (get_timing[24] << 8 ) + get_timing[25];
    core_mp->nodp.tp_to_dp = (get_timing[26] << 8 ) + get_timing[27];
    core_mp->nodp.phase_adc = get_timing[28];
    core_mp->nodp.r2d_pw = get_timing[29];
    core_mp->nodp.rst_pw = get_timing[30];
    core_mp->nodp.rst_pw_back = get_timing[31];
    core_mp->nodp.dac_td = get_timing[32];
    core_mp->nodp.qsh_pw = get_timing[33];
    core_mp->nodp.qsh_td = get_timing[34];
    core_mp->nodp.drop_nodp = get_timing[35];

    ilitek_info("60HZ = %d\n",core_mp->nodp.is60HZ);
    ilitek_info("DDI Mode = %d\n",core_mp->nodp.isLongV);
    ilitek_info("TSHD = %d\n",core_mp->nodp.tshd);
    ilitek_info("Multi Term Num (120Hz) = %d\n",core_mp->nodp.multi_term_num_120);
    ilitek_info("Multi Term Num (60Hz) = %d\n",core_mp->nodp.multi_term_num_60);
    ilitek_info("TSVD to TSHD = %d\n",core_mp->nodp.tsvd_to_tshd);
    ilitek_info("QSH TDF = %d\n",core_mp->nodp.qsh_tdf);
    ilitek_info("AutoTrim Variation = %d\n",core_mp->nodp.auto_trim);
    ilitek_info("TP TSHD Wait (120Hz) = %d\n",core_mp->nodp.tp_tshd_wait_120);
    ilitek_info("DDI Width (120Hz) = %d\n",core_mp->nodp.ddi_width_120);
    ilitek_info("TP TSHD Wait (60Hz) = %d\n",core_mp->nodp.tp_tshd_wait_60);
    ilitek_info("DDI Width (60Hz) = %d\n",core_mp->nodp.ddi_width_60);
    ilitek_info("DP to TP = %d\n",core_mp->nodp.dp_to_tp);
    ilitek_info("TX Wait Const = %d\n",core_mp->nodp.tx_wait_const);
    ilitek_info("TX Wait Const Multi = %d\n",core_mp->nodp.tx_wait_const_multi);
    ilitek_info("TP to DP = %d\n",core_mp->nodp.tp_to_dp);
    ilitek_info("Phase ADC = %d\n",core_mp->nodp.phase_adc);
    ilitek_info("R2D PW = %d\n",core_mp->nodp.r2d_pw);
    ilitek_info("RST PW = %d\n",core_mp->nodp.rst_pw);
    ilitek_info("RST PW Back = %d\n",core_mp->nodp.rst_pw_back);
    ilitek_info("DAC TD = %d\n",core_mp->nodp.dac_td);
    ilitek_info("QSH PW = %d\n",core_mp->nodp.qsh_pw);
    ilitek_info("QSH TD = %d\n",core_mp->nodp.qsh_td);
    ilitek_info("Drop NODP Num = %d\n",core_mp->nodp.drop_nodp);

    mp_calc_nodp(core_mp->nodp.isLongV);

out:
    return ret;
}

static int mp_cdc_get_pv5_4_command(u8 *cmd, int len, int index)
{
    int ret = 0;
    char str[128] = {0};
    char tmp[128] = {0};
    char *key = tItems[index].desp;

    if (strncmp(key, "Raw Data_TD(LCM OFF)", strlen(key)) == 0)
        key = "Doze Raw Data";
    else if (strncmp(key, "Peak To Peak_TD(LCM OFF)", strlen(key)) == 0)
        key = "Doze Peak To Peak";
    else if (strncmp(key, "Raw Data(No BK)(LCM OFF)", strlen(key)) == 0)
        key = "Raw Data(No BK)";
    else if (strncmp(key, "Raw Data(Have BK)(LCM OFF)", strlen(key)) == 0)
        key = "Raw Data(Have BK)";

    /* Calculate NODP */
    mp_calc_timing_nodp();

    ilitek_info("%s gets %s command from INI.\n", tItems[index].desp, key);

    ret = core_parser_get_int_data("PV5_4 Command", key, str);
    if (ret < 0) {
        ilitek_err("Failed to parse PV54 command, ret = %d\n",ret);
        goto out;
    }

    strncpy(tmp, str, ret);
    core_parser_get_u8_array(tmp, cmd);

out:
    return ret;
}

static int mp_cdc_init_cmd_common(u8 *cmd, int len, int index)
{
    int ret = 0;
    struct ilitek_protocol *p_pro = g_ilitek_ts->pro;
    struct protocol_test *p_test = p_pro->test;

    if (p_pro->ver->data[0] >= 5 && p_pro->ver->data[1] >= 4) {
        ilitek_info("Get CDC command with protocol v5.4\n");
        return mp_cdc_get_pv5_4_command(cmd, len, index);
    }

    cmd[0] = p_test->cmd_cdc;
    cmd[1] = tItems[index].cmd;
    cmd[2] = 0;

    if (strcmp(tItems[index].name, "open_integration") == 0)
        cmd[2] = 0x2;
    if (strcmp(tItems[index].name, "open_cap") == 0)
        cmd[2] = 0x3;

    if(tItems[index].catalog == PEAK_TO_PEAK_TEST) {
        cmd[2] = ((tItems[index].frame_count & 0xff00) >> 8);
        cmd[3] = tItems[index].frame_count & 0xff;
        cmd[4] = 0;

        if (strcmp(tItems[index].name, "noise_peak_to_peak_cut") == 0)
            cmd[4] = 0x1;

        ilitek_debug(DEBUG_MP_TEST, "P2P CMD: %d,%d,%d,%d,%d\n",
                cmd[0],cmd[1],cmd[2],cmd[3],cmd[4]);
    }

    return ret;
}

static int allnode_mutual_cdc_data(int index)
{
    static int i = 0, res = 0, len = 0;
    int inDACp = 0, inDACn = 0, first_node = 0;
    static u8 cmd[15] = {0};
    u8 *ori = NULL;
    struct ilitek_config *p_cfg = g_ilitek_ts->cfg;
    struct ilitek_protocol *p_pro = g_ilitek_ts->pro;
    struct protocol_test *p_test = g_ilitek_ts->pro->test;

    /* Multipling by 2 is due to the 16 bit in each node */
    len = (core_mp->xch_len * core_mp->ych_len * 2) + 2;

    ilitek_debug(DEBUG_MP_TEST, "Read X/Y Channel length = %d\n", len);
    ilitek_debug(DEBUG_MP_TEST, "core_mp->frame_len = %d\n", core_mp->frame_len);

    if (len <= 2) {
        ilitek_err("Length is invalid\n");
        res = -1;
        goto out;
    }

    memset(cmd, 0xFF, p_test->cdc_len);

    /* CDC init */
    mp_cdc_init_cmd_common(cmd, p_test->cdc_len, index);

    dump_data(cmd, 8, p_test->cdc_len, 0, "Mutual CDC command");

    res = ilitek_i2c_write(cmd, p_test->cdc_len);
    if (res < 0) {
        ilitek_err("I2C Write Error while initialising cdc\n");
        goto out;
    }

    /* Check busy */
    ilitek_info("Check busy method = %d\n",core_mp->busy_cdc);
    if (core_mp->busy_cdc == POLL_CHECK) {
        /* retry 50 times, each time delay 50ms */
        res = ilitek_config_check_cdc_busy(50, 50);
    } else if (core_mp->busy_cdc == INT_CHECK) {
        res = ilitek_config_check_int_status(false);
    } else if (core_mp->busy_cdc == DELAY_CHECK) {
        mdelay(600);
    }

    if (res < 0) {
        ilitek_err("Check busy timeout !\n");
        res = -1;
        goto out;
    }

    /* Prepare to get cdc data */
    cmd[0] = p_pro->cmd_read_ctrl;
    cmd[1] = p_test->cmd_get_cdc;

    res = ilitek_i2c_write(cmd, 2);
    if (res < 0) {
        ilitek_err("I2C Write Error\n");
        goto out;
    }

    mdelay(1);

    res = ilitek_i2c_write(&cmd[1], 1);
    if (res < 0) {
        ilitek_err("I2C Write Error\n");
        goto out;
    }

    mdelay(1);

    /* Allocate a buffer for the original */
    ori = kcalloc(len, sizeof(u8), GFP_KERNEL);
    if (ERR_ALLOC_MEM(ori)) {
        ilitek_err("Failed to allocate ori mem (%ld)\n", PTR_ERR(ori));
        goto out;
    }

    /* Get original frame(cdc) data */
    res = ilitek_i2c_read(ori, len);
    if (res < 0) {
        ilitek_err("I2C Read Error while getting original cdc data\n");
        goto out;
    }

    dump_data(ori, 8, len, 0, "Mutual CDC original");

    if (frame_buf == NULL) {
        frame_buf = kcalloc(core_mp->frame_len, sizeof(s32), GFP_KERNEL);
        if (ERR_ALLOC_MEM(frame_buf)) {
            ilitek_err("Failed to allocate FrameBuffer mem (%ld)\n", PTR_ERR(frame_buf));
            goto out;
        }
    } else {
        memset(frame_buf, 0x0, core_mp->frame_len);
    }

    /* Convert original data to the physical one in each node */
    for (i = 0; i < core_mp->frame_len; i++) {
        if (tItems[index].cmd == p_test->mutual_dac) {
            /* DAC - P */
            if (((ori[(2 * i) + 1] & 0x80) >> 7) == 1) {
                /* Negative */
                inDACp = 0 - (int)(ori[(2 * i) + 1] & 0x7F);
            } else {
                inDACp = ori[(2 * i) + 1] & 0x7F;
            }

            /* DAC - N */
            if (((ori[(1 + (2 * i)) + 1] & 0x80) >> 7) == 1) {
                /* Negative */
                inDACn = 0 - (int)(ori[(1 + (2 * i)) + 1] & 0x7F);
            } else {
                inDACn = ori[(1 + (2 * i)) + 1] & 0x7F;
            }

            frame_buf[i] = (inDACp + inDACn) / 2;
        } else {
            /* H byte + L byte */
            s32 tmp = (ori[(2 * i) + 1] << 8) + ori[(1 + (2 * i)) + 1];

            if ((tmp & 0x8000) == 0x8000)
                frame_buf[i] = tmp - 65536;
            else
                frame_buf[i] = tmp;

            if(strcmp(tItems[index].name, "mutual_no_bk") == 0)
            {
                if(p_cfg->chip_info->chip_id == ILITEK_ILI9881)
                {
                    if(p_cfg->chip_info->chip_type == ILI9881_TYPE_H)
                        frame_buf[i] -= RAWDATA_NO_BK_DATA_SHIFT_9881H;
                    if(p_cfg->chip_info->chip_type == ILI9881_TYPE_F)
                        frame_buf[i] -= RAWDATA_NO_BK_DATA_SHIFT_9881F;
                }
            }
        }
    }

    first_node = frame_buf[0];
    combined_flag = false;

    for (i = 0; i < core_mp->frame_len; i++) {
        if (first_node == frame_buf[i]) {
            combined_flag = true;
        } else {
            combined_flag = false;
            break;
        }
    }

    ilitek_info("combined flag = %d\n", combined_flag);
    dump_data(frame_buf, 32, core_mp->frame_len,  core_mp->xch_len, "Mutual CDC combined");

out:
    ipio_kfree((void **)&ori);
    return res;
}

static void run_pixel_test(int index)
{
    int i, x, y;
    s32 *p_comb = frame_buf;

    for (y = 0; y < core_mp->ych_len; y++) {
        for (x = 0; x < core_mp->xch_len; x++) {
            int tmp[4] = { 0 }, max = 0;
            int shift = y * core_mp->xch_len;
            int centre = p_comb[shift + x];

            /* if its position is in corner, the number of point
               we have to minus is around 2 to 3.  */
            if (y == 0 && x == 0) {
                tmp[0] = Mathabs(centre - p_comb[(shift + 1) + x]);    /* down */
                tmp[1] = Mathabs(centre - p_comb[shift + (x + 1)]);    /* right */
            } else if (y == (core_mp->ych_len - 1) && x == 0) {
                tmp[0] = Mathabs(centre - p_comb[(shift - 1) + x]);    /* up */
                tmp[1] = Mathabs(centre - p_comb[shift + (x + 1)]);    /* right */
            } else if (y == 0 && x == (core_mp->xch_len - 1)) {
                tmp[0] = Mathabs(centre - p_comb[(shift + 1) + x]);    /* down */
                tmp[1] = Mathabs(centre - p_comb[shift + (x - 1)]);    /* left */
            } else if (y == (core_mp->ych_len - 1) && x == (core_mp->xch_len - 1)) {
                tmp[0] = Mathabs(centre - p_comb[(shift - 1) + x]);    /* up */
                tmp[1] = Mathabs(centre - p_comb[shift + (x - 1)]);    /* left */
            } else if (y == 0 && x != 0) {
                tmp[0] = Mathabs(centre - p_comb[(shift + 1) + x]);    /* down */
                tmp[1] = Mathabs(centre - p_comb[shift + (x - 1)]);    /* left */
                tmp[2] = Mathabs(centre - p_comb[shift + (x + 1)]);    /* right */
            } else if (y != 0 && x == 0) {
                tmp[0] = Mathabs(centre - p_comb[(shift - 1) + x]);    /* up */
                tmp[1] = Mathabs(centre - p_comb[shift + (x + 1)]);    /* right */
                tmp[2] = Mathabs(centre - p_comb[(shift + 1) + x]);    /* down */

            } else if (y == (core_mp->ych_len - 1) && x != 0) {
                tmp[0] = Mathabs(centre - p_comb[(shift - 1) + x]);    /* up */
                tmp[1] = Mathabs(centre - p_comb[shift + (x - 1)]);    /* left */
                tmp[2] = Mathabs(centre - p_comb[shift + (x + 1)]);    /* right */
            } else if (y != 0 && x == (core_mp->xch_len - 1)) {
                tmp[0] = Mathabs(centre - p_comb[(shift - 1) + x]);    /* up */
                tmp[1] = Mathabs(centre - p_comb[shift + (x - 1)]);    /* left */
                tmp[2] = Mathabs(centre - p_comb[(shift + 1) + x]);    /* down */
            } else {
                /* middle minus four directions */
                tmp[0] = Mathabs(centre - p_comb[(shift - 1) + x]);    /* up */
                tmp[1] = Mathabs(centre - p_comb[(shift + 1) + x]);    /* down */
                tmp[2] = Mathabs(centre - p_comb[shift + (x - 1)]);    /* left */
                tmp[3] = Mathabs(centre - p_comb[shift + (x + 1)]);    /* right */
            }

            max = tmp[0];

            for (i = 0; i < 4; i++) {
                if (tmp[i] > max)
                    max = tmp[i];
            }

            tItems[index].buf[shift + x] = max;
        }
    }
}

static int run_open_test(int index)
{
    int i, x, y, k, res = 0;
    int border_x[] = {-1, 0, 1, 1, 1, 0, -1, -1};
    int border_y[] = {-1, -1, -1, 0, 1, 1, 1, 0};
    s32 *p_comb = frame_buf;

    if (strcmp(tItems[index].name, "open_integration") == 0) {
        for (i = 0; i < core_mp->frame_len; i++)
            tItems[index].buf[i] = p_comb[i];
    } else if (strcmp(tItems[index].name, "open_cap") == 0) {
        /* Each result is getting from a 3 by 3 grid depending on where the centre location is.
           So if the centre is at corner, the number of node grabbed from a grid will be different. */
        for (y = 0; y < core_mp->ych_len; y++) {
            for (x = 0; x < core_mp->xch_len; x++) {
                int sum = 0, avg = 0, count = 0;
                int shift = y * core_mp->xch_len;
                int centre = p_comb[shift + x];

                for (k = 0; k < 8; k++) {
                    if (((y + border_y[k] >= 0) && (y + border_y[k] < core_mp->ych_len)) &&
                                ((x + border_x[k] >= 0) && (x + border_x[k] < core_mp->xch_len))) {
                        count++;
                        sum += p_comb[(y + border_y[k]) * core_mp->xch_len + (x + border_x[k])];
                    }
                }

                avg = (sum + centre) / (count + 1);    /* plus 1 because of centre */
                tItems[index].buf[shift + x] = (centre * 100) / avg;
            }
        }
    }
    return res;
}

static void run_tx_rx_delta_test(int index)
{
    int x, y;
    s32 *p_comb = frame_buf;

    for (y = 0; y < core_mp->ych_len; y++) {
        for (x = 0; x < core_mp->xch_len; x++) {
            int shift = y * core_mp->xch_len;

            /* Tx Delta */
            if (y != (core_mp->ych_len - 1)) {
                core_mp->tx_delta_buf[shift + x] = Mathabs(p_comb[shift + x] - p_comb[(shift + 1) + x]);
            }

            /* Rx Delta */
            if (x != (core_mp->xch_len - 1)) {
                core_mp->rx_delta_buf[shift + x] = Mathabs(p_comb[shift + x] - p_comb[shift + (x + 1)]);
            }
        }
    }
}

static void run_untouch_p2p_test(int index)
{
    int x, y;
    s32 *p_comb = frame_buf;

    for (y = 0; y < core_mp->ych_len; y++) {
        for (x = 0; x < core_mp->xch_len; x++) {
            int shift = y * core_mp->xch_len;

            if (p_comb[shift + x] > tItems[index].max_buf[shift + x]) {
                tItems[index].max_buf[shift + x] = p_comb[shift + x];
            }

            if (p_comb[shift + x] < tItems[index].min_buf[shift + x]) {
                tItems[index].min_buf[shift + x] = p_comb[shift + x];
            }

            tItems[index].buf[shift + x] =
                tItems[index].max_buf[shift + x] - tItems[index].min_buf[shift + x];
        }
    }
}

static void compare_MaxMin_result(int index, s32 *data)
{
    int x, y;

    for (y = 0; y < core_mp->ych_len; y++) {
        for (x = 0; x < core_mp->xch_len; x++) {
            int shift = y * core_mp->xch_len;

            if (tItems[index].catalog == UNTOUCH_P2P)
                return;
            else if (tItems[index].catalog == TX_RX_DELTA) {
                /* Tx max/min comparison */
                if (core_mp->tx_delta_buf[shift + x] < data[shift + x]) {
                    core_mp->tx_max_buf[shift + x] = data[shift + x];
                }

                if (core_mp->tx_delta_buf[shift + x] > data[shift + x]) {
                    core_mp->tx_min_buf[shift + x] = data[shift + x];
                }

                /* Rx max/min comparison */
                if (core_mp->rx_delta_buf[shift + x] < data[shift + x]) {
                    core_mp->rx_max_buf[shift + x] = data[shift + x];
                }

                if (core_mp->rx_delta_buf[shift + x] > data[shift + x]) {
                    core_mp->rx_min_buf[shift + x] = data[shift + x];
                }
            } else {
                if (tItems[index].max_buf[shift + x] < data[shift + x]) {
                    tItems[index].max_buf[shift + x] = data[shift + x];
                }

                if (tItems[index].min_buf[shift + x] > data[shift + x]) {
                    tItems[index].min_buf[shift + x] = data[shift + x];
                }
            }
        }
    }
}


#define ABS(a,b) ( (a>b) ? (a-b) : (b-a) )
#define ADDR(x,y) ((y*core_mp->xch_len)+(x))

int full_open_rate_compare(s32* full_open, s32* cbk, int x, int y, s32 inNodeType, int full_open_rate)
{
    int ret = true;

    if ((inNodeType == NO_COMPARE) || ((inNodeType & Round_Corner) == Round_Corner))
    {
        return true;
    }

    if(full_open[ADDR(x,y)] < (cbk[ADDR(x,y)] * full_open_rate / 100))
        ret = false;

    return ret;
}


int compare_charge(s32* charge_rate, int x, int y, s32* inNodeType, int Charge_AA, int Charge_Border, int Charge_Notch)
{
    int OpenThreadhold,tempY, tempX, ret, k;
    int sx[8] = { -1, 0, 1, -1, 1, -1, 0, 1 };
    int sy[8] = { -1, -1, -1, 0, 0, 1, 1, 1 };

    ret = charge_rate[ADDR(x,y)];

    //Setting Threadhold from node type
    if ((inNodeType[ADDR(x,y)] & AA_Area) == AA_Area)
        OpenThreadhold = Charge_AA;
    else if ((inNodeType[ADDR(x,y)] & Border_Area) == Border_Area)
        OpenThreadhold = Charge_Border;
    else if ((inNodeType[ADDR(x,y)] & Notch) == Notch)
        OpenThreadhold = Charge_Notch;
    else if ((inNodeType[ADDR(x,y)] & Round_Corner) == Round_Corner)
        return ret;
    else
        return ret;


    //compare carge rate with 3*3 node
    for (k = 0; k < 8; k++)
    {
        tempX = x + sx[k];
        tempY = y + sy[k];

        if ((tempX < 0) || (tempX >= core_mp->xch_len) || (tempY < 0) || (tempY >= core_mp->ych_len)) //out of range
            continue;

        if ((inNodeType[ADDR(tempX, tempY)] == NO_COMPARE) || ((inNodeType[ADDR(tempX, tempY)] & Round_Corner) == Round_Corner))
            continue;

        if (ABS(charge_rate[ADDR(tempX, tempY)],charge_rate[ADDR(x, y)])> OpenThreadhold)
            return OpenThreadhold;
    }
    return ret;
}

/* This will be merged to allnode_mutual_cdc_data in next version */
int allnode_open_cdc_data(int mode, int *buf, int *dac)
{
    int i = 0, res = 0, len = 0;
    int inDACp = 0, inDACn = 0;
    u8 cmd[15] = {0};
    u8 *ori = NULL;
    char str[128] = {0};
    char tmp[128] = {0};
    char *key[] = {"OPEN DAC", "OPEN Raw1", "OPEN Raw2", "OPEN Raw3"};
    struct ilitek_protocol *p_pro = g_ilitek_ts->pro;
    struct protocol_test *p_test = g_ilitek_ts->pro->test;

    /* Multipling by 2 is due to the 16 bit in each node */
    len = (core_mp->xch_len * core_mp->ych_len * 2) + 2;

    ilitek_debug(DEBUG_MP_TEST, "Read X/Y Channel length = %d\n", len);
    ilitek_debug(DEBUG_MP_TEST, "core_mp->frame_len = %d, mode= %d\n", core_mp->frame_len, mode);

    if (len <= 2) {
        ilitek_err("Length is invalid\n");
        res = -1;
        goto out;
    }

    /* Calculate NODP */
    mp_calc_timing_nodp();

    /* CDC init. Read command from ini file */
    res = core_parser_get_int_data("PV5_4 Command", key[mode], str);
    if (res < 0) {
        ilitek_err("Failed to parse PV54 command, res = %d\n",res);
        goto out;
    }

    strncpy(tmp, str, res);
    core_parser_get_u8_array(tmp, cmd);

    dump_data(cmd, 8, sizeof(cmd), 0, "Open SP command");

    res = ilitek_i2c_write(cmd, 15);
    if (res < 0) {
        ilitek_err("I2C Write Error while initialising cdc\n");
        goto out;
    }

    /* Check busy */
    ilitek_info("Check busy method = %d\n",core_mp->busy_cdc);
    if (core_mp->busy_cdc == POLL_CHECK) {
        /* retry 50 times, each time delay 50ms */
        res = ilitek_config_check_cdc_busy(50, 50);
    } else if (core_mp->busy_cdc == INT_CHECK) {
        res = ilitek_config_check_int_status(false);
    } else if (core_mp->busy_cdc == DELAY_CHECK) {
        mdelay(600);
    }

    if (res < 0) {
        ilitek_err("Check busy timeout !\n");
        res = -1;
        goto out;
    }

    /* Prepare to get cdc data */
    cmd[0] = p_pro->cmd_read_ctrl;
    cmd[1] = p_test->cmd_get_cdc;

    res = ilitek_i2c_write(cmd, 2);
    if (res < 0) {
        ilitek_err("I2C Write Error\n");
        goto out;
    }

    mdelay(1);

    res = ilitek_i2c_write(&cmd[1], 1);
    if (res < 0) {
        ilitek_err("I2C Write Error\n");
        goto out;
    }

    mdelay(1);

    /* Allocate a buffer for the original */
    ori = kcalloc(len, sizeof(u8), GFP_KERNEL);
    if (ERR_ALLOC_MEM(ori)) {
        ilitek_err("Failed to allocate ori mem (%ld)\n", PTR_ERR(ori));
        goto out;
    }

    /* Get original frame(cdc) data */
    res = ilitek_i2c_read(ori, len);
    if (res < 0) {
        ilitek_err("I2C Read Error while getting original cdc data\n");
        goto out;
    }

    dump_data(ori, 8, len, 0, "Open SP CDC original");

    /* Convert original data to the physical one in each node */
    for (i = 0; i < core_mp->frame_len; i++) {
        if (mode == 0) {
            /* DAC - P */
            if (((ori[(2 * i) + 1] & 0x80) >> 7) == 1) {
                /* Negative */
                inDACp = 0 - (int)(ori[(2 * i) + 1] & 0x7F);
            } else {
                inDACp = ori[(2 * i) + 1] & 0x7F;
            }

            /* DAC - N */
            if (((ori[(1 + (2 * i)) + 1] & 0x80) >> 7) == 1) {
                /* Negative */
                inDACn = 0 - (int)(ori[(1 + (2 * i)) + 1] & 0x7F);
            } else {
                inDACn = ori[(1 + (2 * i)) + 1] & 0x7F;
            }

            buf[i] = (inDACp + inDACn) / 2;
        } else {
            /* H byte + L byte */
            s32 tmp = (ori[(2 * i) + 1] << 8) + ori[(1 + (2 * i)) + 1];
            if ((tmp & 0x8000) == 0x8000)
                buf[i] = tmp - 65536;
            else
                buf[i] = tmp;
            buf[i] = (int)((int)(dac[i] * 2 * 10000 * 161 / 100) - (int)(16384 / 2 - (int)buf[i]) * 20000 * 7 / 16384 * 36 / 10) / 31 / 2;
        }
    }
    dump_data(buf, 10, core_mp->frame_len,  core_mp->xch_len, "Open SP CDC combined");
out:
    ipio_kfree((void **)&ori);

    return res;
}

static int ilitek_test_get_raw_data(int index, u32 offset)
{
    int ret = NO_ERR;
    int num = 0, i = 0;
    s32 *dest_buf  = NULL;
    struct ilitek_test *p_test = g_ilitek_ts->test;

    /* ts kit config test items*/
    switch(index) {
    case MP_TEST_NOISE_PEAK_TO_PEAK_PANEL:
    case MP_TEST_NOISE_PEAK_TO_PEAK_IC:
    case MP_TEST_RX_SHORT:
    case MP_TEST_OPEN_INTERGRATION_SP:
    case MP_TEST_MUTUAL_BK:
    //case MP_TEST_MUTUAL_BK_LCM_OFF:
    case MP_TEST_MUTUAL_DAC:
    case MP_TEST_MUTUAL_NO_BK:
    //case MP_TEST_MUTUAL_NO_BK_LCM_OFF:
    //case MP_TEST_NOISE_PEAK_TO_PEAK_PANEL_LCM_OFF:
    //case MP_TEST_NOISE_PEAK_TO_PEAK_IC_LCM_OFF:
    //case MP_TEST_DOZE_RAW_TD_LCM_OFF:
    //case MP_TEST_DOZE_P2P_TD_LCM_OFF:
    //case MP_TEST_DOZE_RAW:
    //case MP_TEST_DOZE_P2P:
        num = ilitek_test_hash(index);
        if (num >= 0 && num < ILITEK_TEST_ITEMS) {
            dest_buf = p_test->orignal_data[num];
            for (i = 0; i < core_mp->frame_len; i++)
                dest_buf[i + offset] = tItems[index].buf[i];
        } else {
            ilitek_err("check hash failed\n");
            ret = -EINVAL;
        }
        break;
    default:
        ilitek_err("item = %s, ts kit mp test not support\n", tItems[index].desp);
        ret = -EINVAL;
        break;
    }

    return ret;
}

static int open_test_sp(int index)
{
    struct mp_test_P540_open open[tItems[index].frame_count];
    int i = 0, x = 0, y = 0, res = 0 ,get_frame_cont = tItems[index].frame_count, addr;
    int Charge_AA = 0, Charge_Border = 0, Charge_Notch =0, full_open_rate = 0;
    char str[512] = { 0 };

    ilitek_debug(DEBUG_MP_TEST, "index = %d, name = %s, CMD = 0x%x, Frame Count = %d\n",
        index, tItems[index].name, tItems[index].cmd, tItems[index].frame_count);

    /*
     * We assume that users who are calling the test forget to config frame count
     * as 1, so we just help them to set it up.
     */
    if (tItems[index].frame_count <= 0) {
        ilitek_err("Frame count is zero, which is at least set as 1\n");
        tItems[index].frame_count = 1;
    }

    res = create_mp_test_frame_buffer(index, tItems[index].frame_count);
    if (res < 0)
        goto out;

    /* Allocate node type buffer only for open test SP */
    tItems[index].node_type = kcalloc(core_mp->frame_len, sizeof(s32), GFP_KERNEL);
    if (ERR_ALLOC_MEM(tItems[index].node_type)){
        ilitek_err("Failed to allocate node_type FRAME buffer\n");
        return -ENOMEM;
    }

    /* Init Max/Min buffer */
    for (y = 0; y < core_mp->ych_len; y++) {
        for (x = 0; x < core_mp->xch_len; x++) {
                tItems[index].max_buf[y * core_mp->xch_len + x] = INT_MIN;
                tItems[index].min_buf[y * core_mp->xch_len + x] = INT_MAX;
        }
    }

    if (tItems[index].spec_option == BENCHMARK) {
        core_parser_benchmark(tItems[index].bench_mark_max, tItems[index].bench_mark_min, tItems[index].type_option,tItems[index].desp);
        if (ilitek_debug_level & DEBUG_PARSER > 0)
            dump_benchmark_data(tItems[index].bench_mark_max , tItems[index].bench_mark_min);
    }

    core_parser_nodetype(tItems[index].node_type,"Node Type");
    if (ilitek_debug_level & DEBUG_PARSER > 0)
        dump_node_type_buffer(tItems[index].node_type, "node type");

    res = core_parser_get_int_data(tItems[index].desp, "Charge_AA", str);
    if (res || res == 0)
        Charge_AA = katoi(str);

    res = core_parser_get_int_data(tItems[index].desp, "Charge_Border", str);
    if (res || res == 0)
        Charge_Border = katoi(str);

    res = core_parser_get_int_data(tItems[index].desp, "Charge_Notch", str);
    if (res || res == 0)
        Charge_Notch = katoi(str);

    res = core_parser_get_int_data(tItems[index].desp, "Full Open", str);
    if (res || res == 0)
        full_open_rate = katoi(str);

    if (res < 0) {
        ilitek_err("Failed to get parameters from ini file\n");
        goto out;
    }

    ilitek_debug(DEBUG_MP_TEST,"pen test frame_cont %d, AA %d,Border %d, Notch %d, full_open_rate %d \n",
            get_frame_cont,Charge_AA,Charge_Border,Charge_Notch,full_open_rate);

    for(i = 0; i < tItems[index].frame_count; i++) {
        open[i].cbk_700 = kcalloc(core_mp->frame_len, sizeof(s32), GFP_KERNEL);
        open[i].cbk_250 = kcalloc(core_mp->frame_len, sizeof(s32), GFP_KERNEL);
        open[i].cbk_200 = kcalloc(core_mp->frame_len, sizeof(s32), GFP_KERNEL);
        open[i].charg_rate = kcalloc(core_mp->frame_len, sizeof(s32), GFP_KERNEL);
        open[i].full_Open = kcalloc(core_mp->frame_len, sizeof(s32), GFP_KERNEL);
        open[i].dac = kcalloc(core_mp->frame_len, sizeof(s32), GFP_KERNEL);
        //open[i].cdc = kcalloc(core_mp->frame_len, sizeof(s32), GFP_KERNEL);
    }

    for (i = 0; i < get_frame_cont; i++) {
        res = allnode_open_cdc_data(0, open[i].dac, open[i].dac);
        if (res < 0) {
            ilitek_err("Failed to get Open SP DAC data, %d\n", res);
            goto out;
        }
        res = allnode_open_cdc_data(1, open[i].cbk_700, open[i].dac);
        if (res < 0) {
            ilitek_err("Failed to get Open SP Raw1 data, %d\n", res);
            goto out;
        }
        res = allnode_open_cdc_data(2, open[i].cbk_250, open[i].dac);
        if (res < 0) {
            ilitek_err("Failed to get Open SP Raw2 data, %d\n", res);
            goto out;
        }
        res = allnode_open_cdc_data(3, open[i].cbk_200, open[i].dac);
        if (res < 0) {
            ilitek_err("Failed to get Open SP Raw3 data, %d\n", res);
            goto out;
        }
        addr = 0;
        for(y = 0; y < core_mp->ych_len; y++){
            for(x = 0; x < core_mp->xch_len; x++){
                open[i].charg_rate[addr] = open[i].cbk_250[addr] * 100 / open[i].cbk_700[addr];
                open[i].full_Open[addr] = open[i].cbk_700[addr] - open[i].cbk_200[addr];
                addr++;
            }
        }
        if (ilitek_debug_level & DEBUG_MP_TEST)
        {
            dump_data(open[i].charg_rate, 10, core_mp->frame_len, core_mp->xch_len, "origin charge rate");
            dump_data(open[i].full_Open, 10, core_mp->frame_len, core_mp->xch_len, "origin full open");
        }

        addr = 0;
        for(y = 0; y < core_mp->ych_len; y++){
            for(x = 0; x < core_mp->xch_len; x++){
                tItems[index].buf[(i * core_mp->frame_len) + addr] = compare_charge(open[i].charg_rate, x, y, tItems[index].node_type, Charge_AA, Charge_Border, Charge_Notch);
                addr++;
            }
        }
        if (ilitek_debug_level & DEBUG_MP_TEST)
            dump_data(&tItems[index].buf[(i * core_mp->frame_len)], 10, core_mp->frame_len, core_mp->xch_len, "after compare charge rate");

        addr = 0;
        for(y = 0; y < core_mp->ych_len; y++){
            for(x = 0; x < core_mp->xch_len; x++){
                if(full_open_rate_compare(open[i].full_Open, open[i].cbk_700, x, y, tItems[index].node_type[addr], full_open_rate) == false)
                    tItems[index].buf[(i * core_mp->frame_len) + addr] = 0;
                addr++;
            }
        }
        if (ilitek_debug_level & DEBUG_MP_TEST)
            dump_data(&tItems[index].buf[(i * core_mp->frame_len)], 10, core_mp->frame_len, core_mp->xch_len, "after full_open_rate_compare");

        ilitek_test_get_raw_data(index, 0);

        compare_MaxMin_result(index, &tItems[index].buf[(i * core_mp->frame_len)]);
    }

    for(i = 0; i < tItems[index].frame_count; i++) {
        ipio_kfree((void **)&open[i].cbk_700);
        ipio_kfree((void **)&open[i].cbk_250);
        ipio_kfree((void **)&open[i].cbk_200);
        ipio_kfree((void **)&open[i].charg_rate);
        ipio_kfree((void **)&open[i].full_Open);
        ipio_kfree((void **)&open[i].dac);
    }

out:
    return res;
}

int codeToOhm(s32 Code)
{
    int douTDF1 = 0;
    int douTDF2 = 0;
    int douTVCH = 24;
    int douTVCL = 8;
    int douCint = 7;
    int douVariation = 64;
    int douRinternal = 930;
    s32 temp = 0;

    if (core_mp->nodp.isLongV) {
        douTDF1 = 300;
        douTDF2 = 100;
    } else {
        douTDF1 = 219;
        douTDF2 = 100;
    }

    if (Code == 0) {
        ilitek_info("This node is zero,set 1 to calculate\n");
        Code = 1;
    }

    temp = ((douTVCH - douTVCL) * douVariation * (douTDF1 - douTDF2) * (1<<14) / (36 * Code * douCint)) * 100;
    /* Unit = M Ohm */
    temp = (temp - douRinternal) / 1000;
    return temp;
}

static int short_test(int index,int frame_index)
{
    int j = 0, res = 0;
    struct ilitek_protocol *p_pro = g_ilitek_ts->pro;

    if(p_pro->ver->data[0] >= 5 && p_pro->ver->data[1] >= 4) {
        /* Calculate code to ohm and save to tItems[index].buf */
        for (j = 0; j < core_mp->frame_len; j++)
            tItems[index].buf[frame_index * core_mp->frame_len + j] = codeToOhm(frame_buf[j]);
    } else {
        for (j = 0; j < core_mp->frame_len; j++)
            tItems[index].buf[frame_index * core_mp->frame_len + j] = frame_buf[j];
    }

    return res;
}

static int mutual_test(int index)
{
    int i = 0, j = 0, x = 0, y = 0, res = 0 ,get_frame_cont =1 ;

    ilitek_debug(DEBUG_MP_TEST, "index = %d, name = %s, CMD = 0x%x, Frame Count = %d\n",
        index, tItems[index].name, tItems[index].cmd, tItems[index].frame_count);

    /*
     * We assume that users who are calling the test forget to config frame count
     * as 1, so we just help them to set it up.
     */
    if (tItems[index].frame_count <= 0) {
        ilitek_err("Frame count is zero, which is at least set as 1\n");
        tItems[index].frame_count = 1;
    }

    res = create_mp_test_frame_buffer(index, tItems[index].frame_count);
    if (res < 0)
        goto out;

    /* Init Max/Min buffer */
    for (y = 0; y < core_mp->ych_len; y++) {
        for (x = 0; x < core_mp->xch_len; x++) {
            if (tItems[i].catalog == TX_RX_DELTA) {
                core_mp->tx_max_buf[y * core_mp->xch_len + x] = INT_MIN;
                core_mp->rx_max_buf[y * core_mp->xch_len + x] = INT_MIN;
                core_mp->tx_min_buf[y * core_mp->xch_len + x] = INT_MAX;
                core_mp->rx_min_buf[y * core_mp->xch_len + x] = INT_MAX;
            } else {
                tItems[index].max_buf[y * core_mp->xch_len + x] = INT_MIN;
                tItems[index].min_buf[y * core_mp->xch_len + x] = INT_MAX;
            }
        }
    }
    if (tItems[index].catalog != PEAK_TO_PEAK_TEST)
        get_frame_cont = tItems[index].frame_count;

    if (tItems[index].spec_option == BENCHMARK) {
        core_parser_benchmark(tItems[index].bench_mark_max, tItems[index].bench_mark_min, tItems[index].type_option,tItems[index].desp);
        if (ilitek_debug_level & DEBUG_PARSER > 0)
            dump_benchmark_data(tItems[index].bench_mark_max , tItems[index].bench_mark_min);
    }

    for (i = 0; i < get_frame_cont; i++) {
        res = allnode_mutual_cdc_data(index);
        if (res < 0) {
            ilitek_err("Failed to initialise CDC data, %d\n", res);
            goto out;
        }
        switch (tItems[index].catalog) {
        case PIXEL:
            run_pixel_test(index);
            break;
        case UNTOUCH_P2P:
            run_untouch_p2p_test(index);
            break;
        case OPEN_TEST:
            run_open_test(index);
            break;
        case TX_RX_DELTA:
            run_tx_rx_delta_test(index);
            break;
        case SHORT_TEST:
            short_test(index , i);
            break;
        default:
            for (j = 0; j < core_mp->frame_len; j++)
                tItems[index].buf[i * core_mp->frame_len + j] = frame_buf[j];
            break;
        }

        compare_MaxMin_result(index, &tItems[index].buf[i * core_mp->frame_len]);

        ilitek_test_get_raw_data(index, 0);
    }

out:
    return res;
}

static int key_test(int index)
{
    int i, j = 0, res = 0;

    ilitek_debug(DEBUG_MP_TEST, "Item = %s, CMD = 0x%x, Frame Count = %d\n",
        tItems[index].name, tItems[index].cmd, tItems[index].frame_count);

    if (tItems[index].frame_count == 0) {
        ilitek_err("Frame count is zero, which at least sets as 1\n");
        res = -EINVAL;
        goto out;
    }

    res = create_mp_test_frame_buffer(index, tItems[index].frame_count);
    if (res < 0)
        goto out;

    for (i = 0; i < tItems[index].frame_count; i++) {
        res = allnode_key_cdc_data(index);
        if (res < 0) {
            ilitek_err("Failed to initialise CDC data, %d\n", res);
            goto out;
        }

        for (j = 0; j < core_mp->key_len; j++)
            tItems[index].buf[j] = key_buf[j];
    }

    compare_MaxMin_result(index, tItems[index].buf);

out:
    return res;
}

static int self_test(int index)
{
    ilitek_err("TDDI has no self to be tested currently\n");
    return -1;
}

static int st_test(int index)
{
    ilitek_err("ST Test is not supported by the driver\n");
    return -1;
}

int mp_test_data_sort_average(int *u32buff_data,int index)
{
    int i,j,k,x,y,len = 5;
    s32 u32temp;
    int u32up_frame,u32down_frame;
    s32* u32sum_raw_data;

    if (ERR_ALLOC_MEM(u32buff_data)){
        ilitek_err("Input wrong adress\n");
            return -ENOMEM;
    }

    u32sum_raw_data = kcalloc(core_mp->frame_len, sizeof(s32), GFP_KERNEL);
    if (ERR_ALLOC_MEM(u32sum_raw_data)){
        ilitek_err("Failed to allocate u32sum_raw_data FRAME buffer\n");
        return -ENOMEM;
    }

    if(tItems[index].frame_count <= 1)
        return 0;

    u32up_frame = tItems[index].frame_count * tItems[index].highest_percentage / 100;
    u32down_frame = tItems[index].frame_count * tItems[index].lowest_percentage / 100;
    ilitek_debug(DEBUG_MP_TEST,"Up=%d,Down=%d -%s\n",u32up_frame,u32down_frame,tItems[index].desp);

    if (ilitek_debug_level & DEBUG_MP_TEST) {
        printk("\n[Show Original frist%d and last%d node data]\n",len,len);
        for(i = 0 ; i < core_mp->frame_len ; i++){
            for(j = 0 ; j < tItems[index].frame_count ; j++){
                if((i<len) || (i >= (core_mp->frame_len-len)))
                    printk("%d,",u32buff_data[j* core_mp->frame_len + i]);
            }
            if((i<len) || (i >=(core_mp->frame_len-len)))
                printk("\n");
        }
    }

    for(i = 0 ; i < core_mp->frame_len ; i++){
        for(j = 0 ; j < tItems[index].frame_count-1 ; j++){
            for(k = 0 ; k < (tItems[index].frame_count-1-j) ; k++){
                x=i+k*core_mp->frame_len;
                y=i+(k+1)*core_mp->frame_len;
                if (*(u32buff_data+x) > *(u32buff_data+y))
                {
                    u32temp = *(u32buff_data+x);
                    *(u32buff_data+x) = *(u32buff_data+y);
                    *(u32buff_data+y) = u32temp;
                }
            }
        }
    }

    if (ilitek_debug_level & DEBUG_MP_TEST) {
        printk("\n[After sorting frist%d and last%d node data]\n",len,len);
        for(i = 0 ; i < core_mp->frame_len ; i++){
            for(j = u32down_frame ; j < tItems[index].frame_count - u32up_frame ; j++){
                if((i<len) || (i >= (core_mp->frame_len-len)))
                    printk("%d,",u32buff_data[i + j * core_mp->frame_len]);
            }
            if((i<len) || (i >= (core_mp->frame_len-len)))
                printk("\n");
        }
    }

    for(i = 0 ; i < core_mp->frame_len ; i++){
        u32sum_raw_data[i]=0;
        for(j = u32down_frame ; j < tItems[index].frame_count - u32up_frame ; j++)
            u32sum_raw_data[i] += u32buff_data[i + j * core_mp->frame_len];

        u32buff_data[i] = u32sum_raw_data[i] / (tItems[index].frame_count - u32down_frame - u32up_frame);
    }

    if (ilitek_debug_level & DEBUG_MP_TEST) {
        printk("\n[Average result frist%d and last%d node data]\n",len,len);
        for(i = 0 ; i < core_mp->frame_len ; i++){
            if((i<len) || (i >= (core_mp->frame_len-len)))
                printk("%d,",u32buff_data[i]);
        }
        if((i<len) || (i >= (core_mp->frame_len-len)))
            printk("\n");
    }

    ipio_kfree((void **)&u32sum_raw_data);
    return 0;
}

int core_mp_compare_retry_cdc_result(int index)
{
    int x, y, shift;
    int max_ts, min_ts, retry_result = MP_PASS;
    s32 *bench = NULL, *tmp_max = NULL, *tmp_min = NULL;

    ilitek_info("index = %d, item = %s\n",index,tItems[index].desp);

    if (tItems[index].spec_option == BENCHMARK) {

        if (ERR_ALLOC_MEM(tItems[index].buf)) {
            ilitek_err("The buffer in NULL, retry failed\n");
            retry_result = MP_FAIL;
            goto out;
        }

        bench = tItems[index].buf;

        for (y = 0; y < core_mp->ych_len; y++) {
            for (x = 0; x < core_mp->xch_len; x++) {
                shift = y * core_mp->xch_len + x;
                if (bench[shift] > tItems[index].bench_mark_max[shift] ||
                    bench[shift] < tItems[index].bench_mark_min[shift]) {
                        retry_result = MP_FAIL;
                        break;
                }
            }
        }
    } else {

        if (ERR_ALLOC_MEM(tItems[index].max_buf) || ERR_ALLOC_MEM(tItems[index].min_buf)) {
            ilitek_err("The buffer in NULL, retry failed\n");
            retry_result = MP_FAIL;
            goto out;
        }

        tmp_max = tItems[index].max_buf;
        tmp_min = tItems[index].min_buf;

        max_ts = tItems[index].max;
        min_ts = tItems[index].min;

        for (y = 0; y < core_mp->ych_len; y++) {
            for (x = 0; x < core_mp->xch_len; x++) {
                shift = y * core_mp->xch_len + x;
                if (tmp_max[shift] > max_ts || tmp_min[shift] < min_ts) {
                    retry_result = MP_FAIL;
                    break;
                }
            }
        }
    }

    if (combined_flag) {
        ilitek_info("Combined data is same, doing retry\n");
        retry_result = MP_FAIL;
    }

out:
    ilitek_info("Result = %s\n", ((retry_result == MP_PASS) ? "MP PASS" : "MP FAIL"));

    if (retry_result == MP_PASS) {
        tItems[index].max_res = MP_PASS;
        tItems[index].min_res = MP_PASS;
    } else {
        tItems[index].max_res = MP_FAIL;
        tItems[index].min_res = MP_FAIL;
    }

    return retry_result;
}

void core_mp_retry(int index, int count)
{
    if (count == 0) {
        ilitek_info("Finish retry action\n");
        return;
    }

    /* hw reset avoid i2c error */
    ilitek_chip_reset();

    /* Switch to Demo mode */
    ilitek_config_mode_ctrl(ILITEK_DEMO_MODE, NULL);

    /* Switch to test mode */
    ilitek_config_mode_ctrl(ILITEK_TEST_MODE, NULL);

    ilitek_info("retry = %d, item = %s\n", count, tItems[index].desp);

    tItems[index].do_test(index);

    if (core_mp_compare_retry_cdc_result(index) == MP_FAIL) {
        return core_mp_retry(index, count - 1);
    }
}

void core_mp_show_result(void)
{
    int i, x, y, csv_len = 0, pass_item_count = 0;
    char *csv = NULL;
    char csv_name[128] = { 0 };
    char *line_breaker = "\n";
    struct file *f = NULL;
    mm_segment_t fs;
    loff_t pos;
    struct ilitek_config *p_cfg = g_ilitek_ts->cfg;

    csv = vmalloc(CSV_FILE_SIZE);
    if (ERR_ALLOC_MEM(csv)) {
        ilitek_err("Failed to allocate CSV mem\n");
        goto fail_open;
    }

    csv_len += sprintf(csv + csv_len, "%x\n", p_cfg->chip_info->chip_pid);

    for (i = 0; i < ARRAY_SIZE(tItems); i++) {
        if (tItems[i].run) {
            if (tItems[i].trimmed_mean && tItems[i].catalog != PEAK_TO_PEAK_TEST)
            {
                mp_test_data_sort_average(tItems[i].buf,i);
                if ( tItems[i].spec_option != BENCHMARK)
                {
                    mp_show_sorting_result(i, true, false, csv, &csv_len);
                    continue;
                }
            }
            if ( tItems[i].spec_option == BENCHMARK) {
                mp_compare_benchmark_result(i, true, false, csv, &csv_len);
                continue;
            }

            pr_info("%s\n", tItems[i].desp);
            csv_len += sprintf(csv + csv_len, " %s ", tItems[i].desp);

            pr_info("\n");
            csv_len += sprintf(csv + csv_len, "%s", line_breaker);

            pr_info("Max = %d\n",tItems[i].max);
            csv_len += sprintf(csv + csv_len, "Max = %d", tItems[i].max);

            pr_info("\n");
            csv_len += sprintf(csv + csv_len, "%s", line_breaker);

            pr_info("Min = %d\n",tItems[i].min);
            csv_len += sprintf(csv + csv_len, "Min = %d", tItems[i].min);

            pr_info("\n");
            csv_len += sprintf(csv + csv_len, "%s", line_breaker);

            pr_info("Frame count = %d\n",tItems[i].frame_count);
            csv_len += sprintf(csv + csv_len, "Frame count = %d", tItems[i].frame_count);

            pr_info("\n");
            csv_len += sprintf(csv + csv_len, "%s", line_breaker);

            if (tItems[i].catalog == TX_RX_DELTA) {
                if (ERR_ALLOC_MEM(core_mp->rx_delta_buf) || ERR_ALLOC_MEM(core_mp->tx_delta_buf)) {
                    ilitek_err("This test item (%s) has no data inside its buffer\n", tItems[i].desp);
                    continue;
                }
            } else {
                if (ERR_ALLOC_MEM(tItems[i].buf) || ERR_ALLOC_MEM(tItems[i].max_buf) ||
                        ERR_ALLOC_MEM(tItems[i].min_buf)) {
                    ilitek_err("This test item (%s) has no data inside its buffer\n", tItems[i].desp);
                    continue;
                }
            }

            if (tItems[i].catalog == KEY_TEST) {
                for (x = 0; x < core_mp->key_len; x++) {
                    DUMP(DEBUG_MP_TEST, "KEY_%02d ", x);
                    csv_len += sprintf(csv + csv_len, "KEY_%02d,", x);
                }

                DUMP(DEBUG_MP_TEST, "\n");
                csv_len += sprintf(csv + csv_len, "%s", line_breaker);

                for (y = 0; y < core_mp->key_len; y++) {
                    DUMP(DEBUG_MP_TEST, " %3d   ", tItems[i].buf[y]);
                    csv_len += sprintf(csv + csv_len, " %3d, ", tItems[i].buf[y]);
                }

                DUMP(DEBUG_MP_TEST, "\n");
                csv_len += sprintf(csv + csv_len, "%s", line_breaker);
            } else if (tItems[i].catalog == TX_RX_DELTA) {
                DUMP(DEBUG_MP_TEST, " %s ", "TX Max Hold");
                csv_len += sprintf(csv + csv_len, "  %s ", "TX Max Hold");

                DUMP(DEBUG_MP_TEST, "\n");
                csv_len += sprintf(csv + csv_len, "%s", line_breaker);

                mp_compare_cdc_result(i, true, true, csv, &csv_len);

                DUMP(DEBUG_MP_TEST, " %s ", "TX Min Hold");
                csv_len += sprintf(csv + csv_len, "  %s ", "TX Min Hold");

                DUMP(DEBUG_MP_TEST, "\n");
                csv_len += sprintf(csv + csv_len, "%s", line_breaker);

                mp_compare_cdc_result(i, false, true, csv, &csv_len);

                DUMP(DEBUG_MP_TEST, " %s ", "RX Max Hold");
                csv_len += sprintf(csv + csv_len, "  %s ", "RX Max Hold");

                DUMP(DEBUG_MP_TEST, "\n");
                csv_len += sprintf(csv + csv_len, "%s", line_breaker);

                mp_compare_cdc_result(i, true, false, csv, &csv_len);

                DUMP(DEBUG_MP_TEST, " %s ", "RX Min Hold");
                csv_len += sprintf(csv + csv_len, "  %s ", "RX Min Hold");

                DUMP(DEBUG_MP_TEST, "\n");
                csv_len += sprintf(csv + csv_len, "%s", line_breaker);

                mp_compare_cdc_result(i, false, false, csv, &csv_len);
            } else {
                DUMP(DEBUG_MP_TEST, " %s ", "Max Hold");
                csv_len += sprintf(csv + csv_len, "  %s ", "Max Hold");

                DUMP(DEBUG_MP_TEST, "\n");
                csv_len += sprintf(csv + csv_len, "%s", line_breaker);

                mp_compare_cdc_result(i, true, false, csv, &csv_len);

                DUMP(DEBUG_MP_TEST, " %s ", "Min Hold");
                csv_len += sprintf(csv + csv_len, "  %s ", "Min Hold");

                DUMP(DEBUG_MP_TEST, "\n");
                csv_len += sprintf(csv + csv_len, "%s", line_breaker);

                mp_compare_cdc_result(i, false, false, csv, &csv_len);
            }

            if (tItems[i].max_res == MP_PASS && tItems[i].min_res == MP_PASS)
                sprintf(tItems[i].result, "%s", "PASS");
            else
                sprintf(tItems[i].result, "%s", "FAIL");

            pr_info("\n Result : %s\n", tItems[i].result);
            csv_len += sprintf(csv + csv_len, " Result : %s ", tItems[i].result);

            pr_info("\n");
            csv_len += sprintf(csv + csv_len, "%s", line_breaker);
            csv_len += sprintf(csv + csv_len, "%s", line_breaker);
            csv_len += sprintf(csv + csv_len, "%s", line_breaker);
        }
    }

    memset(csv_name, 0, 128 * sizeof(char));

    for (i = 0; i < ARRAY_SIZE(tItems); i++) {
        if (tItems[i].run) {
            if (strcmp(tItems[i].result, "FAIL") == 0)
            {
                pass_item_count = 0;
                break;
            }
            pass_item_count++;
        }
    }

    if(pass_item_count == 0)
        core_mp->final_result = MP_FAIL;
    else
        core_mp->final_result = MP_PASS;

    if (core_mp->final_result == MP_PASS)
        sprintf(csv_name, "%s/%s.csv", CSV_PATH, "mp_pass");
    else
        sprintf(csv_name, "%s/%s.csv", CSV_PATH, "mp_fail");

    ilitek_info("Open CSV : %s\n", csv_name);

    if (f == NULL)
        f = filp_open(csv_name, O_WRONLY | O_CREAT | O_TRUNC, 644);

    if (ERR_ALLOC_MEM(f)) {
        ilitek_err("Failed to open CSV file\n");
        goto fail_open;
    }

    ilitek_info("Open CSV succeed, its length = %d\n ", csv_len);

    if (csv_len >= CSV_FILE_SIZE) {
        ilitek_err("The length saved to CSV is too long !\n");
        goto fail_open;
    }

    fs = get_fs();
    set_fs(KERNEL_DS);
    pos = 0;
    vfs_write(f, csv, csv_len, &pos);
    set_fs(fs);
    filp_close(f, NULL);

    ilitek_info("Writing Data into CSV succeed\n");

fail_open:
    if(csv != NULL)
        vfree(csv);
}

bool ilitek_check_result(int index)
{
    int x, y;
    s32 *max_buf = NULL, *min_buf = NULL;
    s32 *bk_max_buf = NULL, *bk_min_buf = NULL;
    s32 *data = NULL;
    int max_ts, min_ts, offset;
    bool mp_result = true;

    max_buf = tItems[index].max_buf;
    min_buf = tItems[index].min_buf;
    bk_max_buf = tItems[index].bench_mark_max;
    bk_min_buf = tItems[index].bench_mark_min;
    max_ts = tItems[index].max;
    min_ts = tItems[index].min;

    data = tItems[index].buf;

    ilitek_debug(DEBUG_MP_TEST, "item = %s\n", tItems[index].desp);
    if (tItems[index].spec_option == BENCHMARK) {
        if (bk_max_buf != NULL && bk_min_buf!= NULL) {
            ilitek_debug(DEBUG_MP_TEST, "bk item = %s\n", tItems[index].desp);
            for (y = 0; y < core_mp->ych_len; y++) {
                for (x = 0; x < core_mp->xch_len; x++) {
                    offset = y * core_mp->xch_len + x;
                    if (data[offset] > bk_max_buf[offset] ||
                        data[offset] < bk_min_buf[offset]) {
                        mp_result = false;
                        ilitek_err("%s [%2d, %2d] val is [%5d], BK limit is [%5d, %5d]\n",
                            tItems[index].desp,
                            y,
                            x,
                            data[offset],
                            bk_max_buf[offset],
                            bk_min_buf[offset]);
                    }
                }
            }
        }
    } else {
        if (max_buf != NULL && min_buf != NULL) {
            ilitek_debug(DEBUG_MP_TEST, "no_bk item = %s\n", tItems[index].desp);
            for (y = 0; y < core_mp->ych_len; y++) {
                for (x = 0; x < core_mp->xch_len; x++) {
                    offset = y * core_mp->xch_len + x;
                    if (max_buf[offset] > max_ts ||
                        max_buf[offset] < min_ts ||
                        min_buf[offset] > max_ts ||
                        min_buf[offset] < min_ts) {
                        mp_result = false;
                        ilitek_err("%s [%2d, %2d] min_max val is [%5d, %5d], limit is [%5d, %5d]\n",
                            tItems[index].desp,
                            y,
                            x,
                            min_buf[offset],
                            max_buf[offset],
                            min_ts,
                            max_ts);
                    }
                }
            }
        }
    }

    return mp_result;
}

void core_mp_run_test(char *item, bool ini)
{
    int i = 0;
    char str[512] = { 0 };
    struct ilitek_config *p_cfg = g_ilitek_ts->cfg;

    /* update X/Y channel length if they're changed */
    core_mp->xch_len = p_cfg->tp_info.nXChannelNum;
    core_mp->ych_len = p_cfg->tp_info.nYChannelNum;

    /* update key's length if they're changed */
    core_mp->key_len = p_cfg->tp_info.nKeyCount;

    /* compute the total length in one frame */
    core_mp->frame_len = core_mp->xch_len * core_mp->ych_len;

    if (item == NULL || strncmp(item, " ", strlen(item)) == 0 || core_mp->frame_len == 0 ) {
        core_mp->final_result = MP_FAIL;
        ilitek_err("Invaild string or length\n");
        return;
    }

    ilitek_debug(DEBUG_MP_TEST, "item = %s\n", item);

    for (i = 0; i < core_mp->mp_items; i++) {
        if (strncmp(item, tItems[i].desp, strlen(item)) == 0) {
            if (ini) {
                core_parser_get_int_data(item, "Enable", str);
                tItems[i].run = katoi(str);
                core_parser_get_int_data(item, "SPEC Option", str);
                tItems[i].spec_option= katoi(str);
                core_parser_get_int_data(item, "Type Option", str);
                tItems[i].type_option= katoi(str);
                 core_parser_get_int_data(item, "Frame Count", str);
                tItems[i].frame_count= katoi(str);
                core_parser_get_int_data(item, "Trimmed Mean", str);
                tItems[i].trimmed_mean= katoi(str);
                core_parser_get_int_data(item, "Lowest Percentage", str);
                tItems[i].lowest_percentage= katoi(str);
                 core_parser_get_int_data(item, "Highest Percentage", str);
                tItems[i].highest_percentage= katoi(str);

                /* Get threshold from ini structure in parser */
                if (strcmp(item, "Tx/Rx Delta") == 0) {
                    core_parser_get_int_data(item, "Tx Max", str);
                    core_mp->TxDeltaMax = katoi(str);
                    core_parser_get_int_data(item, "Tx Min", str);
                    core_mp->TxDeltaMin = katoi(str);
                    core_parser_get_int_data(item, "Rx Max", str);
                    core_mp->RxDeltaMax = katoi(str);
                    core_parser_get_int_data(item, "Rx Min", str);
                    core_mp->RxDeltaMin = katoi(str);
                    ilitek_debug(DEBUG_MP_TEST, "%s: Tx Max = %d, Tx Min = %d, Rx Max = %d,  Rx Min = %d\n",
                            tItems[i].desp, core_mp->TxDeltaMax, core_mp->TxDeltaMin,
                            core_mp->RxDeltaMax, core_mp->RxDeltaMin);
                } else {
                    core_parser_get_int_data(item, "Max", str);
                    tItems[i].max = katoi(str);
                    core_parser_get_int_data(item, "Min", str);
                    tItems[i].min = katoi(str);
                }

                core_parser_get_int_data(item, "Frame Count", str);
                tItems[i].frame_count = katoi(str);

                ilitek_debug(DEBUG_MP_TEST, "%s: run = %d, max = %d, min = %d, frame_count = %d\n", tItems[i].desp,
                        tItems[i].run, tItems[i].max, tItems[i].min, tItems[i].frame_count);
            }

            if (tItems[i].run) {
                /* LCM off */
                if (strnstr(tItems[i].desp, "LCM", strlen(tItems[i].desp)) != NULL)
                    mp_ctrl_lcm_status(false);

                core_mp->run = true;
                ilitek_info("Running Test Item : %s\n", tItems[i].desp);
                tItems[i].do_test(i);

                /* To see if this item needs to do retry  */
                if (core_mp_compare_retry_cdc_result(i) == MP_FAIL)
                    core_mp_retry(i, RETRY_COUNT);

                core_mp->run = false;

                /* LCM on */
                if (strnstr(tItems[i].desp, "LCM", strlen(tItems[i].desp)) != NULL)
                    mp_ctrl_lcm_status(false);
            }
            break;
        }
    }
}

/* debug: dump crc result, hw check mp fw status, after copy from flash to iram */
static int host_download_dma_check(void)
{
    int retry = ILITEK_DEBUG_CRC_RETRYS;
    u32 start_addr = 0, block_size = ILITEK_DEBUG_MP_FW_SIZE;
    u32 busy = 0 , crc_data = 0;

    ilitek_info("start_addr = %d, block_size = %d\n", start_addr, block_size);

    /* dma1 src1 adress */
    ilitek_config_ice_mode_write(0x072104, start_addr, 4);
    /* dma1 src1 format */
    ilitek_config_ice_mode_write(0x072108, 0x80000001, 4);
    /* dma1 dest address */
    ilitek_config_ice_mode_write(0x072114, 0x00030000, 4);
    /* dma1 dest format */
    ilitek_config_ice_mode_write(0x072118, 0x80000000, 4);
    /* Block size*/
    ilitek_config_ice_mode_write(0x07211C, block_size, 4);
    /* crc off */
    ilitek_config_ice_mode_write(0x041014, 0x00000000, 4);
    /* dma crc */
    ilitek_config_ice_mode_write(0x041048, 0x00000001, 4);
    /* crc on */
    ilitek_config_ice_mode_write(0x041014, 0x00010000, 4);
    /* Dma1 stop */
    ilitek_config_ice_mode_write(0x072100, 0x00000000, 4);
    /* clr int */
    ilitek_config_ice_mode_write(0x048006, 0x1, 1);
    /* Dma1 start */
    ilitek_config_ice_mode_write(0x072100, 0x01000000, 4);

    /* Polling BIT0 */
    while (retry > 0) {
        mdelay(1);
        /* check iram crc status */
        busy = ilitek_config_read_write_onebyte(0x048006);

        if ((busy & 0x01) == 1)
            break;

        retry--;
    }

    if (retry <= 0) {
        ilitek_err("BIT0 is busy\n");
        return -EIO;
    }

    /* read iram crc result */
    crc_data =  ilitek_config_ice_mode_read(0x04101C);
    ilitek_info("read crc[0x04101c], data = 0x%x\n" , crc_data);
}

int core_mp_move_code(void)
{
    ilitek_info("Prepaing to enter Test Mode\n");

    if (ilitek_config_check_cdc_busy(50, 10) < 0) {
        ilitek_err("Check busy is timout ! Enter Test Mode failed\n");
        return -1;
    }

    if (ilitek_config_ice_mode_enable() < 0) {
        ilitek_err("Failed to enter ICE mode\n");
        return -1;
    }

    /* DMA Trigger */
    ilitek_config_ice_mode_write(0x41010, 0xFF, 1);

    mdelay(30);

    /* CS High */
    ilitek_config_ice_mode_write(0x041000, 0x1, 1);

    host_download_dma_check();

    mdelay(60);

    /* Code reset */
    ilitek_config_ice_mode_write(0x40040, 0xAE, 1);

    ilitek_config_ice_mode_disable();

    /* retry 15 times, each time delay 50ms */
    if (ilitek_config_check_cdc_busy(15, 50) < 0) {
        ilitek_err("Check busy is timout ! Enter Test Mode failed\n");
        return -1;
    }

    ilitek_info("FW Test Mode ready\n");
    return 0;
}

void core_mp_test_free(void)
{
    int i;

    ilitek_info("Free all allocated mem\n");

    core_mp->final_result = MP_FAIL;

    for (i = 0; i < ARRAY_SIZE(tItems); i++) {
        tItems[i].run = false;
        tItems[i].max_res = false;
        tItems[i].min_res = false;
        sprintf(tItems[i].result, "%s", "FAIL");

        if (tItems[i].catalog == TX_RX_DELTA) {
            ipio_kfree((void **)&core_mp->rx_delta_buf);
            ipio_kfree((void **)&core_mp->tx_delta_buf);
            ipio_kfree((void **)&core_mp->tx_max_buf);
            ipio_kfree((void **)&core_mp->tx_min_buf);
            ipio_kfree((void **)&core_mp->rx_max_buf);
            ipio_kfree((void **)&core_mp->rx_min_buf);
        } else {
            if (tItems[i].spec_option == BENCHMARK) {
                ipio_kfree((void **)&tItems[i].bench_mark_max);
                ipio_kfree((void **)&tItems[i].bench_mark_min);
            }
            ipio_kfree((void **)&tItems[i].buf);
            ipio_kfree((void **)&tItems[i].max_buf);
            ipio_kfree((void **)&tItems[i].min_buf);
        }
    }

    ipio_kfree((void **)&frame_buf);
    ipio_kfree((void **)&key_buf);
    ipio_kfree((void **)&core_mp);
}

static void mp_test_init_item(void)
{
    int i;
    struct protocol_test *p_test = g_ilitek_ts->pro->test;

    core_mp->mp_items = ARRAY_SIZE(tItems);

    /* assign test functions run on MP flow according to their catalog */
    for (i = 0; i < ARRAY_SIZE(tItems); i++) {

        tItems[i].spec_option = 0;
        tItems[i].type_option = 0;
        tItems[i].run = false;
        tItems[i].max = 0;
        tItems[i].max_res = MP_FAIL;
        tItems[i].min = 0;
        tItems[i].min_res = MP_FAIL;
        tItems[i].frame_count = 0;
        tItems[i].trimmed_mean = 0;
        tItems[i].lowest_percentage = 0;
        tItems[i].highest_percentage = 0;
        tItems[i].buf = NULL;
        tItems[i].max_buf = NULL;
        tItems[i].min_buf = NULL;
        tItems[i].bench_mark_max = NULL;
        tItems[i].bench_mark_min = NULL;
        tItems[i].node_type = NULL;

        if (tItems[i].catalog == MUTUAL_TEST) {
            tItems[i].do_test = mutual_test;
        } else if (tItems[i].catalog == TX_RX_DELTA) {
            tItems[i].do_test = mutual_test;
        } else if (tItems[i].catalog == UNTOUCH_P2P) {
            tItems[i].do_test = mutual_test;
        } else if (tItems[i].catalog == PIXEL) {
            tItems[i].do_test = mutual_test;
        } else if (tItems[i].catalog == OPEN_TEST){
            if (strcmp(tItems[i].name, "open_integration_sp") == 0)
                tItems[i].do_test = open_test_sp;
            else
                tItems[i].do_test = mutual_test;
        } else if (tItems[i].catalog == KEY_TEST) {
            tItems[i].do_test = key_test;
        } else if (tItems[i].catalog == SELF_TEST) {
            tItems[i].do_test = self_test;
        } else if (tItems[i].catalog == ST_TEST) {
            tItems[i].do_test = st_test;
        } else if (tItems[i].catalog == PEAK_TO_PEAK_TEST) {
            tItems[i].do_test = mutual_test;
        } else if (tItems[i].catalog == SHORT_TEST) {
            tItems[i].do_test = mutual_test;
        }

        tItems[i].result = kmalloc(16, GFP_KERNEL);
        sprintf(tItems[i].result, "%s", "FAIL");
    }

    /*
     * assign protocol command written into firmware via I2C,
     * which might be differnet if the version of protocol was changed.
     */
    tItems[0].cmd = p_test->mutual_dac;
    tItems[1].cmd = p_test->mutual_bg;
    tItems[2].cmd = p_test->mutual_signal;
    tItems[3].cmd = p_test->mutual_no_bk;
    tItems[4].cmd = p_test->mutual_has_bk;
    tItems[5].cmd = p_test->mutual_bk_dac;
    tItems[6].cmd = p_test->self_dac;
    tItems[7].cmd = p_test->self_bg;
    tItems[8].cmd = p_test->self_signal;
    tItems[9].cmd = p_test->self_no_bk;
    tItems[10].cmd = p_test->self_has_bk;
    tItems[11].cmd = p_test->self_bk_dac;
    tItems[12].cmd = p_test->key_dac;
    tItems[13].cmd = p_test->key_bg;
    tItems[14].cmd = p_test->key_no_bk;
    tItems[15].cmd = p_test->key_has_bk;
    tItems[16].cmd = p_test->key_open;
    tItems[17].cmd = p_test->key_short;
    tItems[18].cmd = p_test->st_dac;
    tItems[19].cmd = p_test->st_bg;
    tItems[20].cmd = p_test->st_no_bk;
    tItems[21].cmd = p_test->st_has_bk;
    tItems[22].cmd = p_test->st_open;
    tItems[23].cmd = p_test->tx_short;
    tItems[24].cmd = p_test->rx_short;
    tItems[25].cmd = p_test->rx_open;
    tItems[26].cmd = p_test->cm_data;
    tItems[27].cmd = p_test->cs_data;
    tItems[28].cmd = p_test->tx_rx_delta;
    tItems[29].cmd = p_test->mutual_signal;
    tItems[30].cmd = p_test->mutual_no_bk;
    tItems[31].cmd = p_test->mutual_has_bk;
    tItems[32].cmd = p_test->rx_open;
    tItems[33].cmd = p_test->rx_open;
    tItems[34].cmd = p_test->peak_to_peak;
}

int core_mp_init(void)
{
    int res = 0;
    struct ilitek_config *p_cfg = g_ilitek_ts->cfg;

    if (p_cfg->tp_info.nXChannelNum == 0 ||
        p_cfg->tp_info.nYChannelNum == 0 ||
        p_cfg->tp_info.self_tx_channel_num == 0 ||
        p_cfg->tp_info.self_rx_channel_num == 0) {
        ilitek_err("tp_info is invaild\n");
        return -EINVAL;
    }

    if (core_mp == NULL) {
        core_mp = kzalloc(sizeof(*core_mp), GFP_KERNEL);
        if (ERR_ALLOC_MEM(core_mp)) {
            ilitek_err("Failed to init core_mp, %ld\n", PTR_ERR(core_mp));
            return -ENOMEM;
        }

        core_mp->xch_len = p_cfg->tp_info.nXChannelNum;
        core_mp->ych_len = p_cfg->tp_info.nYChannelNum;

        core_mp->stx_len = p_cfg->tp_info.self_tx_channel_num;
        core_mp->srx_len = p_cfg->tp_info.self_rx_channel_num;

        core_mp->key_len = p_cfg->tp_info.nKeyCount;
        core_mp->st_len = p_cfg->tp_info.side_touch_type;

        core_mp->tdf = 240;
        core_mp->busy_cdc = INT_CHECK;

        core_mp->run = false;
        core_mp->final_result = MP_FAIL;

        mp_test_init_item();
    }

    return res;
}
