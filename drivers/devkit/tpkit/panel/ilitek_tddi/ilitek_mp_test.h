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
#ifndef __ILITEK_MP_TEST_H__
#define __ILITEK_MP_TEST_H__

#define MP_PASS                                1
#define MP_FAIL                                -1
#define BENCHMARK                              1
#define NODETYPE                               1
#define RAWDATA_NO_BK_DATA_SHIFT_9881H         8192
#define RAWDATA_NO_BK_DATA_SHIFT_9881F         4096

#define ILITEK_DEBUG_CRC_RETRYS                 50
/* last 4 bytes are crc in fw, others are mp fw size */
#define ILITEK_DEBUG_MP_FW_SIZE                 (36 * 1024 - 4)

/* must match with tItems */
typedef enum mp_tests {
    MP_TEST_MUTUAL_DAC,
    MP_TEST_MUTUAL_BG,
    MP_TEST_MUTUAL_SIGNAL,
    MP_TEST_MUTUAL_NO_BK,
    MP_TEST_MUTUAL_NO_BK_LCM_OFF,
    MP_TEST_MUTUAL_BK,                         /* 5 */
    MP_TEST_MUTUAL_BK_LCM_OFF,
    MP_TEST_MUTUAL_BK_DAC,

    MP_TEST_SELF_DAC,
    MP_TEST_SELF_BG,
    MP_TEST_SELF_SIGNAL,                       /* 10 */
    MP_TEST_SELF_NO_BK,
    MP_TEST_SELF_BK,
    MP_TEST_SELF_BK_DAC,

    MP_TEST_KEY_DAC,
    MP_TEST_KEY_BG,                            /* 15 */
    MP_TEST_KEY_NO_BK,
    MP_TEST_KEY_BK,
    MP_TEST_KEY_OPEN,
    MP_TEST_KEY_SHORT,

    MP_TEST_ST_DAC,                            /* 20 */
    MP_TEST_ST_BG,
    MP_TEST_ST_NO_BK,
    MP_TEST_ST_BK,
    MP_TEST_ST_OPEN,

    MP_TEST_TX_SHORT,                          /* 25 */
    MP_TEST_RX_SHORT,
    MP_TEST_RX_OPEN,

    MP_TEST_CM_DATA,
    MP_TEST_CS_DATA,

    MP_TEST_TX_RX_DELTA,                       /* 30 */

    MP_TEST_P2P,

    MP_TEST_PIXEL_NO_BK,
    MP_TEST_PIXEL_BK,

    MP_TEST_OPEN_INTERGRATION,
    MP_TEST_OPEN_INTERGRATION_SP,              /* 35 */
    MP_TEST_OPEN_CAP,

    MP_TEST_NOISE_PEAK_TO_PEAK_IC,
    MP_TEST_NOISE_PEAK_TO_PEAK_IC_LCM_OFF,
    MP_TEST_NOISE_PEAK_TO_PEAK_PANEL,
    MP_TEST_NOISE_PEAK_TO_PEAK_PANEL_LCM_OFF,  /* 40 */

    MP_TEST_DOZE_RAW,
    MP_TEST_DOZE_P2P,
    MP_TEST_DOZE_RAW_TD_LCM_OFF,
    MP_TEST_DOZE_P2P_TD_LCM_OFF,
    MP_TEST_NUMS,
}mp_tests;

struct mp_test_P540_open {
    s32 *cbk_700;
    s32 *cbk_250;
    s32 *cbk_200;
    s32 *charg_rate;
    s32 *full_Open;
    s32 *dac;
    s32 *cdc;
};

struct mp_test_items {
    char *name;
    /* The description must be the same as ini's section name */
    char *desp;
    char *result;
    int catalog;
    u8 cmd;
    u8 spec_option;
    u8 type_option;
    bool run;
    int max;
    int max_res;
    int min;
    int min_res;
    int frame_count;
    int trimmed_mean;
    int lowest_percentage;
    int highest_percentage;
    s32 *buf;
    s32 *max_buf;
    s32 *min_buf;
    s32 *bench_mark_max;
    s32 *bench_mark_min;
    s32 *node_type;
    int (*do_test)(int index);
};

struct mp_nodp_calc {
    bool is60HZ;
    bool isLongV;

    /* Input */
    u16 tshd;
    u8 multi_term_num_120;
    u8 multi_term_num_60;
    u16 tsvd_to_tshd;
    u16 qsh_tdf;

    /* Settings */
    u8 auto_trim;
    u16 tp_tshd_wait_120;
    u16 ddi_width_120;
    u16 tp_tshd_wait_60;
    u16 ddi_width_60;
    u16 dp_to_tp;
    u16 tx_wait_const;
    u16 tx_wait_const_multi;
    u16 tp_to_dp;
    u8 phase_adc;
    u8 r2d_pw;
    u8 rst_pw;
    u8 rst_pw_back;
    u8 dac_td;
    u8 qsh_pw;
    u8 qsh_td;
    u8 drop_nodp;

    /* Output */
    u32 first_tp_width;
    u32 tp_width;
    u32 txpw;
    u32 long_tsdh_wait;
    u32 nodp;
};

struct core_mp_test_data {
    struct mp_nodp_calc nodp;
    /* A flag shows a test run in particular */
    bool run;
    bool m_signal;
    bool m_dac;
    bool s_signal;
    bool s_dac;
    bool key_dac;
    bool st_dac;
    bool p_no_bk;
    bool p_has_bk;
    bool open_integ;
    bool open_cap;

    int xch_len;
    int ych_len;
    int stx_len;
    int srx_len;
    int key_len;
    int st_len;
    int frame_len;
    int mp_items;
    int final_result;

    /* Tx/Rx threshold & buffer */
    int TxDeltaMax;
    int TxDeltaMin;
    int RxDeltaMax;
    int RxDeltaMin;
    s32 *tx_delta_buf;
    s32 *rx_delta_buf;
    s32 *tx_max_buf;
    s32 *tx_min_buf;
    s32 *rx_max_buf;
    s32 *rx_min_buf;

    int tdf;
    bool busy_cdc;
    bool ctrl_lcm;
};

extern struct core_mp_test_data *core_mp;
extern struct mp_test_items tItems[];

bool ilitek_check_result(int index);
void dump_data(void *data, int type, int len, int row_len, const char *name);
void core_mp_test_free(void);
void core_mp_show_result(void);
void core_mp_run_test(char *item, bool ini);
int core_mp_move_code(void);
int core_mp_init(void);

#endif

