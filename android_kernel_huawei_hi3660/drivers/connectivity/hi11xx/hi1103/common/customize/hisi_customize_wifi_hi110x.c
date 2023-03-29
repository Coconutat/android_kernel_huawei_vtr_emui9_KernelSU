

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#if defined(_PRE_PRODUCT_ID_HI110X_HOST)

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
/*
 * 1 Header File Including
 */

#include <linux/kernel.h>
#include <linux/time.h>
#include "hisi_customize_wifi_hi110x.h"
#include "hisi_ini.h"
#include "plat_type.h"
#include "oam_ext_if.h"
#include "wlan_spec.h"
#include "mac_vap.h"
#include "oal_sdio_comm.h"
#include "oal_hcc_host_if.h"
#include "oal_main.h"
#include "dmac_ext_if.h"
#include "plat_pm_wlan.h"
#include "plat_firmware.h"

/* 终端头文件 */
#include <linux/mtd/hisi_nve_interface.h>
#include <linux/etherdevice.h>

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HISI_CUSTOMIZE_WIFI_HI110X_C

/*
 * 2 Global Variable Definition
 */
int32 g_al_host_init_params_etc[WLAN_CFG_INIT_BUTT] = {0};      /* ini定制化参数数组 */
int32 g_al_dts_params_etc[WLAN_CFG_DTS_BUTT] = {0};             /* dts定制化参数数组 */
int8  g_ac_country_code_etc[COUNTRY_CODE_LEN] = "00";
uint8 g_auc_wifimac_etc[MAC_LEN] = {0x00,0x00,0x00,0x00,0x00,0x00};
int32 g_al_nvram_init_params[NVRAM_PARAMS_PWR_INDEX_BUTT] = {0};    /* ini文件中NV参数数组 */
wlan_customize_private_stru g_al_priv_cust_params[WLAN_CFG_PRIV_BUTT] = {{0,0}};  /* 私有定制化参数数组 */
wlan_cust_country_code_ingore_flag_stru g_st_cust_country_code_ignore_flag = {0}; /* 定制化国家码配置 */
wlan_cust_nvram_params g_st_cust_nv_params  = {{0}};  /* 最大发送功率定制化数组 */
wlan_customize_pwr_fit_para_stru g_ast_pro_line_params[WLAN_RF_CHANNEL_NUMS][DY_CALI_PARAMS_NUM] = {{{0}}};    /* 产测定制化参数数组 */
uint8 g_auc_cust_nvram_info[WLAN_CFG_DTS_NVRAM_END][CUS_PARAMS_LEN_MAX] = {{0}};  /* NVRAM数组 */
oal_bool_enum_uint8 g_en_nv_dp_init_is_null = OAL_TRUE;      /* NVRAM中dp init置空标志 */
oal_int16 gs_extre_point_vals[WLAN_RF_CHANNEL_NUMS][DY_CALI_NUM_5G_BAND] = {{0}};
uint8 g_uc_wlan_open_cnt = 0;
#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH
oal_bool_enum_uint8 g_en_tas_switch_en = OAL_FALSE;
#endif
oal_bool_enum_uint8 g_en_fact_cali_completed = OAL_FALSE;

/*
 * 定制化结构体
 * default values as follows:
 * ampdu_tx_max_num:            WLAN_AMPDU_TX_MAX_NUM               = 64
 * switch:                      ON                                  = 1
 * scan_orthogonal:             ROAM_SCAN_CHANNEL_ORG_BUTT          = 4
 */
wlan_customize_stru g_st_wlan_customize_etc = {
            64,             /* addba_buffer_size */
            1,              /* roam switch */
            4,              /* roam scan org */
            -70,            /* roam trigger 2G */
            -70,            /* roam trigger 5G */
            10,             /* roam delta 2G */
            10,             /* roam delta 5G */
            0,              /* random mac addr scan */
            0,              /* disable_capab_2ght40 */
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST)
            0,              /*lte_gpio_check_switch*/
            0,              /*ism_priority*/
            0,              /*lte_rx*/
            0,              /*lte_tx*/
            0,              /*lte_inact*/
            0,              /*ism_rx_act*/
            0,              /*bant_pri*/
            0,              /*bant_status*/
            0,              /*want_pri*/
            0,              /*want_status*/
#endif
};

/**
 *  regdomain <-> country code map table
 *  max support country num: MAX_COUNTRY_COUNT
 *
 **/
OAL_STATIC countryinfo_stru g_ast_country_info_table[] =
{
    /*Note:too few initializers for unsigned char [3]*/
    /*lint -e785*/
    {REGDOMAIN_COMMON, {'0', '0'}}, // WORLD DOMAIN
    {REGDOMAIN_FCC, {'A', 'D'}}, // ANDORRA
    {REGDOMAIN_ETSI, {'A', 'E'}}, //UAE
    {REGDOMAIN_ETSI, {'A', 'L'}}, //ALBANIA
    {REGDOMAIN_ETSI, {'A', 'M'}}, //ARMENIA
    {REGDOMAIN_ETSI, {'A', 'N'}}, //NETHERLANDS ANTILLES
    {REGDOMAIN_FCC, {'A', 'R'}}, //ARGENTINA
    {REGDOMAIN_FCC, {'A', 'S'}}, //AMERICAN SOMOA
    {REGDOMAIN_ETSI, {'A', 'T'}}, //AUSTRIA
    {REGDOMAIN_FCC, {'A', 'U'}}, //AUSTRALIA
    {REGDOMAIN_ETSI , {'A', 'W'}}, //ARUBA
    {REGDOMAIN_ETSI,  {'A', 'Z'}}, //AZERBAIJAN
    {REGDOMAIN_ETSI, {'B', 'A'}}, //BOSNIA AND HERZEGOVINA
    {REGDOMAIN_FCC, {'B', 'B'}}, //BARBADOS
    {REGDOMAIN_ETSI, {'B', 'D'}}, //BANGLADESH
    {REGDOMAIN_ETSI, { 'B', 'E'}}, //BELGIUM
    {REGDOMAIN_ETSI, {'B', 'G'}}, //BULGARIA
    {REGDOMAIN_ETSI, {'B', 'H'}}, //BAHRAIN
    {REGDOMAIN_ETSI, {'B', 'L'}}, //
    {REGDOMAIN_FCC, {'B', 'M'}}, //BERMUDA
    {REGDOMAIN_ETSI, {'B', 'N'}}, //BRUNEI DARUSSALAM
    {REGDOMAIN_ETSI, {'B', 'O'}}, //BOLIVIA
    {REGDOMAIN_ETSI, {'B', 'R'}}, //BRAZIL
    {REGDOMAIN_FCC, {'B', 'S'}}, //BAHAMAS
    {REGDOMAIN_ETSI, {'B', 'Y'}}, //BELARUS
    {REGDOMAIN_ETSI, {'B', 'Z'}}, //BELIZE
    {REGDOMAIN_FCC, {'C', 'A'}}, //CANADA
    {REGDOMAIN_ETSI, {'C', 'H'}}, //SWITZERLAND
    {REGDOMAIN_ETSI, {'C', 'L'}}, //CHILE
    {REGDOMAIN_COMMON, {'C', 'N'}}, //CHINA
    {REGDOMAIN_FCC, {'C', 'O'}}, //COLOMBIA
    {REGDOMAIN_ETSI, {'C', 'R'}}, //COSTA RICA
    {REGDOMAIN_ETSI, {'C', 'S'}},
    {REGDOMAIN_ETSI, {'C', 'Y'}}, //CYPRUS
    {REGDOMAIN_ETSI, {'C', 'Z'}}, //CZECH REPUBLIC
    {REGDOMAIN_ETSI, {'D', 'E'}}, //GERMANY
    {REGDOMAIN_ETSI, {'D', 'K'}}, //DENMARK
    {REGDOMAIN_FCC, {'D', 'O'}}, //DOMINICAN REPUBLIC
    {REGDOMAIN_ETSI, {'D', 'Z'}}, //ALGERIA
    {REGDOMAIN_ETSI, {'E', 'C'}}, //ECUADOR
    {REGDOMAIN_ETSI, {'E', 'E'}}, //ESTONIA
    {REGDOMAIN_ETSI, {'E', 'G'}}, //EGYPT
    {REGDOMAIN_ETSI, {'E', 'S'}}, //SPAIN
    {REGDOMAIN_ETSI, {'F', 'I'}}, //FINLAND
    {REGDOMAIN_ETSI, {'F', 'R'}}, //FRANCE
    {REGDOMAIN_ETSI, {'G', 'B'}}, //UNITED KINGDOM
    {REGDOMAIN_FCC, {'G', 'D'}},  //GRENADA
    {REGDOMAIN_ETSI, {'G', 'E'}}, //GEORGIA
    {REGDOMAIN_ETSI, {'G', 'F'}}, //FRENCH GUIANA
    {REGDOMAIN_ETSI, {'G', 'L'}}, //GREENLAND
    {REGDOMAIN_ETSI, {'G', 'P'}}, //GUADELOUPE
    {REGDOMAIN_ETSI, {'G', 'R'}}, //GREECE
    {REGDOMAIN_FCC, {'G', 'T'}},  //GUATEMALA
    {REGDOMAIN_FCC, {'G', 'U'}},  //GUAM
    {REGDOMAIN_ETSI, {'H', 'R'}}, //Croatia
    {REGDOMAIN_ETSI, {'H', 'U'}}, //HUNGARY
    {REGDOMAIN_FCC, {'I', 'D'}},  //INDONESIA
    {REGDOMAIN_ETSI, {'I', 'E'}}, //IRELAND
    {REGDOMAIN_ETSI, {'I', 'L'}}, //ISRAEL
    {REGDOMAIN_ETSI, {'I', 'N'}}, //INDIA
    {REGDOMAIN_ETSI, {'I', 'R'}}, //IRAN, ISLAMIC REPUBLIC OF
    {REGDOMAIN_ETSI, {'I', 'S'}}, //ICELNAD
    {REGDOMAIN_ETSI, {'I', 'T'}}, //ITALY
    {REGDOMAIN_FCC, {'J', 'M'}},  //JAMAICA
    {REGDOMAIN_JAPAN, {'J', 'P'}}, //JAPAN
    {REGDOMAIN_ETSI, {'J', 'O'}}, //JORDAN
    {REGDOMAIN_ETSI, {'K', 'E'}}, //KENYA
    {REGDOMAIN_ETSI, {'K', 'H'}}, //CAMBODIA
    {REGDOMAIN_ETSI, {'K', 'P'}}, //KOREA, DEMOCRATIC PEOPLE's REPUBLIC OF
    {REGDOMAIN_ETSI, {'K', 'R'}}, //KOREA, REPUBLIC OF
    {REGDOMAIN_ETSI, {'K', 'W'}}, //KUWAIT
    {REGDOMAIN_ETSI, {'K', 'Z'}}, //KAZAKHSTAN
    {REGDOMAIN_ETSI, {'L', 'B'}}, //LEBANON
    {REGDOMAIN_ETSI, {'L', 'I'}}, //LIECHTENSTEIN
    {REGDOMAIN_ETSI, {'L', 'K'}}, //SRI-LANKA
    {REGDOMAIN_ETSI, {'L', 'T'}}, //LITHUANIA
    {REGDOMAIN_ETSI, {'L', 'U'}}, //LUXEMBOURG
    {REGDOMAIN_ETSI, {'L','V'}},  //LATVIA
    {REGDOMAIN_ETSI, {'M', 'A'}}, //MOROCCO
    {REGDOMAIN_ETSI, {'M', 'C'}}, //MONACO
    {REGDOMAIN_ETSI, {'M', 'E'}}, //Montenegro
    {REGDOMAIN_ETSI, {'M', 'K'}}, //MACEDONIA, THE FORMER YUGOSLAV REPUBLIC OF
    {REGDOMAIN_ETSI, {'M', 'M'}}, //MYANMAR
    {REGDOMAIN_FCC, {'M','N'}}, //MONGOLIA
    {REGDOMAIN_FCC, {'M', 'O'}}, //MACAO
    {REGDOMAIN_FCC, {'M', 'P'}}, //NORTHERN MARIANA ISLANDS
    {REGDOMAIN_ETSI, {'M', 'Q'}}, //MARTINIQUE
    {REGDOMAIN_FCC, {'M', 'T'}}, //MALTA
    {REGDOMAIN_ETSI, {'M', 'U'}}, //MAURITIUS
    {REGDOMAIN_ETSI, {'M', 'W'}}, //MALAWI
    {REGDOMAIN_FCC, {'M', 'X'}}, //MEXICO
    {REGDOMAIN_ETSI, {'M', 'Y'}}, //MALAYSIA
    {REGDOMAIN_ETSI, {'N', 'G'}}, //NIGERIA
    {REGDOMAIN_FCC, {'N', 'I'}}, //NICARAGUA
    {REGDOMAIN_ETSI, {'N', 'L'}}, //NETHERLANDS
    {REGDOMAIN_ETSI, {'N', 'O'}}, //NORWAY
    {REGDOMAIN_ETSI, {'N', 'P'}}, //NEPAL
    {REGDOMAIN_FCC, {'N', 'Z'}}, //NEW-ZEALAND
    {REGDOMAIN_FCC, {'O', 'M'}}, //OMAN
    {REGDOMAIN_FCC, {'P', 'A'}}, //PANAMA
    {REGDOMAIN_ETSI, {'P', 'E'}}, //PERU
    {REGDOMAIN_ETSI, {'P', 'F'}}, //FRENCH POLYNESIA
    {REGDOMAIN_ETSI, {'P', 'G'}}, //PAPUA NEW GUINEA
    {REGDOMAIN_FCC, {'P', 'H'}}, //PHILIPPINES
    {REGDOMAIN_ETSI, {'P', 'K'}}, //PAKISTAN
    {REGDOMAIN_ETSI, {'P', 'L'}}, //POLAND
    {REGDOMAIN_FCC, {'P', 'R'}}, //PUERTO RICO
    {REGDOMAIN_FCC, {'P', 'S'}}, //PALESTINIAN TERRITORY, OCCUPIED
    {REGDOMAIN_ETSI, {'P', 'T'}}, //PORTUGAL
    {REGDOMAIN_FCC, {'P', 'Y'}}, //PARAGUAY
    {REGDOMAIN_ETSI, {'Q', 'A'}}, //QATAR
    {REGDOMAIN_ETSI, {'R', 'E'}}, //REUNION
    {REGDOMAIN_ETSI, {'R', 'O'}}, //ROMAINIA
    {REGDOMAIN_ETSI, {'R', 'S'}}, //SERBIA
    {REGDOMAIN_ETSI, {'R', 'U'}}, //RUSSIA
    {REGDOMAIN_FCC, {'R', 'W'}}, //RWANDA
    {REGDOMAIN_ETSI, {'S', 'A'}}, //SAUDI ARABIA
    {REGDOMAIN_ETSI, {'S', 'D'}}, //SUDAN ,REPUBLIC OF THE
    {REGDOMAIN_ETSI, {'S', 'E'}}, //SWEDEN
    {REGDOMAIN_ETSI, {'S', 'G'}}, //SINGAPORE
    {REGDOMAIN_ETSI, {'S', 'I'}}, //SLOVENNIA
    {REGDOMAIN_ETSI, {'S', 'K'}}, //SLOVAKIA
    {REGDOMAIN_ETSI, {'S', 'V'}}, //EL SALVADOR
    {REGDOMAIN_ETSI, {'S', 'Y'}}, //SYRIAN ARAB REPUBLIC
    {REGDOMAIN_ETSI, {'T', 'H'}}, //THAILAND
    {REGDOMAIN_ETSI, {'T', 'N'}}, //TUNISIA
    {REGDOMAIN_ETSI, {'T', 'R'}}, //TURKEY
    {REGDOMAIN_ETSI, {'T', 'T'}}, //TRINIDAD AND TOBAGO
    {REGDOMAIN_FCC, {'T', 'W'}}, //TAIWAN, PRIVINCE OF CHINA
    {REGDOMAIN_FCC, {'T', 'Z'}}, //TANZANIA, UNITED REPUBLIC OF
    {REGDOMAIN_ETSI, {'U', 'A'}}, //UKRAINE
    {REGDOMAIN_ETSI, {'U', 'G'}}, //UGANDA
    {REGDOMAIN_FCC, {'U', 'S'}}, //USA
    {REGDOMAIN_ETSI, {'U', 'Y'}}, //URUGUAY
    {REGDOMAIN_FCC, {'U', 'Z'}}, //UZBEKISTAN
    {REGDOMAIN_ETSI, {'V', 'E'}}, //VENEZUELA
    {REGDOMAIN_FCC, {'V', 'I'}}, //VIRGIN ISLANDS, US
    {REGDOMAIN_ETSI, {'V', 'N'}}, //VIETNAM
    {REGDOMAIN_ETSI, {'Y', 'E'}}, //YEMEN
    {REGDOMAIN_ETSI, {'Y', 'T'}}, //MAYOTTE
    {REGDOMAIN_ETSI, {'Z', 'A'}}, //SOUTH AFRICA
    {REGDOMAIN_ETSI, {'Z', 'W'}}, //ZIMBABWE

    {REGDOMAIN_COUNT,{'9','9'}}
    /*lint +e785*/
};

#if 0
/**
 * regdomain <-> plat_tag mapping table
 *
 **/
OAL_STATIC regdomain_plat_tag_map_stru g_ast_plat_tag_mapping_table[] =
{
        {REGDOMAIN_FCC,     INI_MODU_POWER_FCC},        //FCC
        {REGDOMAIN_ETSI,    INI_MODU_POWER_ETSI},       //ETSI
        {REGDOMAIN_JAPAN,   INI_MODU_POWER_JP},         //JP
        {REGDOMAIN_COMMON,  INI_MODU_WIFI},             //COMMON

        {REGDOMAIN_COUNT,   INI_MODU_INVALID}
};
#endif

OAL_STATIC wlan_cfg_cmd g_ast_wifi_config_dts[] =
{
    /* 校准 */
    {"cali_txpwr_pa_dc_ref_2g_val_chan1",                    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN1},
    {"cali_txpwr_pa_dc_ref_2g_val_chan2",                    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN2},
    {"cali_txpwr_pa_dc_ref_2g_val_chan3",                    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN3},
    {"cali_txpwr_pa_dc_ref_2g_val_chan4",                    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN4},
    {"cali_txpwr_pa_dc_ref_2g_val_chan5",                    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN5},
    {"cali_txpwr_pa_dc_ref_2g_val_chan6",                    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN6},
    {"cali_txpwr_pa_dc_ref_2g_val_chan7",                    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN7},
    {"cali_txpwr_pa_dc_ref_2g_val_chan8",                    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN8},
    {"cali_txpwr_pa_dc_ref_2g_val_chan9",                    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN9},
    {"cali_txpwr_pa_dc_ref_2g_val_chan10",                   WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN10},
    {"cali_txpwr_pa_dc_ref_2g_val_chan11",                   WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN11},
    {"cali_txpwr_pa_dc_ref_2g_val_chan12",                   WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN12},
    {"cali_txpwr_pa_dc_ref_2g_val_chan13",                   WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN13},

    {"cali_txpwr_pa_dc_ref_5g_val_band1",                    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND1},
    {"cali_txpwr_pa_dc_ref_5g_val_band2",                    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND2},
    {"cali_txpwr_pa_dc_ref_5g_val_band3",                    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND3},
    {"cali_txpwr_pa_dc_ref_5g_val_band4",                    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND4},
    {"cali_txpwr_pa_dc_ref_5g_val_band5",                    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND5},
    {"cali_txpwr_pa_dc_ref_5g_val_band6",                    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND6},
    {"cali_txpwr_pa_dc_ref_5g_val_band7",                    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND7},
    {"cali_tone_amp_grade",                                  WLAN_CFG_DTS_CALI_TONE_AMP_GRADE},
    /* DPD校准 */
    {"dpd_cali_ch_core0",                        WLAN_CFG_DTS_DPD_CALI_CH_CORE0},
    {"dpd_use_cail_ch_idx0_core0",               WLAN_CFG_DTS_DPD_USE_CALI_CH_IDX0_CORE0},
    {"dpd_use_cail_ch_idx1_core0",               WLAN_CFG_DTS_DPD_USE_CALI_CH_IDX1_CORE0},
    {"dpd_use_cail_ch_idx2_core0",               WLAN_CFG_DTS_DPD_USE_CALI_CH_IDX2_CORE0},
    {"dpd_use_cail_ch_idx3_core0",               WLAN_CFG_DTS_DPD_USE_CALI_CH_IDX3_CORE0},
    {"dpd_cali_ch_core1",                        WLAN_CFG_DTS_DPD_CALI_CH_CORE1},
    {"dpd_use_cail_ch_idx0_core1",               WLAN_CFG_DTS_DPD_USE_CALI_CH_IDX0_CORE1},
    {"dpd_use_cail_ch_idx1_core1",               WLAN_CFG_DTS_DPD_USE_CALI_CH_IDX1_CORE1},
    {"dpd_use_cail_ch_idx2_core1",               WLAN_CFG_DTS_DPD_USE_CALI_CH_IDX2_CORE1},
    {"dpd_use_cail_ch_idx3_core1",               WLAN_CFG_DTS_DPD_USE_CALI_CH_IDX3_CORE1},
    /* 动态校准 */
    {"dyn_cali_dscr_interval",     WLAN_CFG_DTS_DYN_CALI_DSCR_ITERVL},
    {"dyn_cali_opt_switch",        WLAN_CFG_DTS_DYN_CALI_OPT_SWITCH},
    {"gm0_dB10_amend",             WLAN_CFG_DTS_DYN_CALI_GM0_DB10_AMEND},
    /* DPN 40M 20M 11b */
    {"dpn24g_ch1_core0",           WLAN_CFG_DTS_2G_CORE0_DPN_CH1},
    {"dpn24g_ch2_core0",           WLAN_CFG_DTS_2G_CORE0_DPN_CH2},
    {"dpn24g_ch3_core0",           WLAN_CFG_DTS_2G_CORE0_DPN_CH3},
    {"dpn24g_ch4_core0",           WLAN_CFG_DTS_2G_CORE0_DPN_CH4},
    {"dpn24g_ch5_core0",           WLAN_CFG_DTS_2G_CORE0_DPN_CH5},
    {"dpn24g_ch6_core0",           WLAN_CFG_DTS_2G_CORE0_DPN_CH6},
    {"dpn24g_ch7_core0",           WLAN_CFG_DTS_2G_CORE0_DPN_CH7},
    {"dpn24g_ch8_core0",           WLAN_CFG_DTS_2G_CORE0_DPN_CH8},
    {"dpn24g_ch9_core0",           WLAN_CFG_DTS_2G_CORE0_DPN_CH9},
    {"dpn24g_ch10_core0",          WLAN_CFG_DTS_2G_CORE0_DPN_CH10},
    {"dpn24g_ch11_core0",          WLAN_CFG_DTS_2G_CORE0_DPN_CH11},
    {"dpn24g_ch12_core0",          WLAN_CFG_DTS_2G_CORE0_DPN_CH12},
    {"dpn24g_ch13_core0",          WLAN_CFG_DTS_2G_CORE0_DPN_CH13},
    {"dpn5g_core0_b0",             WLAN_CFG_DTS_5G_CORE0_DPN_B0},
    {"dpn5g_core0_b1",             WLAN_CFG_DTS_5G_CORE0_DPN_B1},
    {"dpn5g_core0_b2",             WLAN_CFG_DTS_5G_CORE0_DPN_B2},
    {"dpn5g_core0_b3",             WLAN_CFG_DTS_5G_CORE0_DPN_B3},
    {"dpn5g_core0_b4",             WLAN_CFG_DTS_5G_CORE0_DPN_B4},
    {"dpn24g_ch1_core1",           WLAN_CFG_DTS_2G_CORE1_DPN_CH1},
    {"dpn24g_ch2_core1",           WLAN_CFG_DTS_2G_CORE1_DPN_CH2},
    {"dpn24g_ch3_core1",           WLAN_CFG_DTS_2G_CORE1_DPN_CH3},
    {"dpn24g_ch4_core1",           WLAN_CFG_DTS_2G_CORE1_DPN_CH4},
    {"dpn24g_ch5_core1",           WLAN_CFG_DTS_2G_CORE1_DPN_CH5},
    {"dpn24g_ch6_core1",           WLAN_CFG_DTS_2G_CORE1_DPN_CH6},
    {"dpn24g_ch7_core1",           WLAN_CFG_DTS_2G_CORE1_DPN_CH7},
    {"dpn24g_ch8_core1",           WLAN_CFG_DTS_2G_CORE1_DPN_CH8},
    {"dpn24g_ch9_core1",           WLAN_CFG_DTS_2G_CORE1_DPN_CH9},
    {"dpn24g_ch10_core1",          WLAN_CFG_DTS_2G_CORE1_DPN_CH10},
    {"dpn24g_ch11_core1",          WLAN_CFG_DTS_2G_CORE1_DPN_CH11},
    {"dpn24g_ch12_core1",          WLAN_CFG_DTS_2G_CORE1_DPN_CH12},
    {"dpn24g_ch13_core1",          WLAN_CFG_DTS_2G_CORE1_DPN_CH13},
    {"dpn5g_core1_b0",             WLAN_CFG_DTS_5G_CORE1_DPN_B0},
    {"dpn5g_core1_b1",             WLAN_CFG_DTS_5G_CORE1_DPN_B1},
    {"dpn5g_core1_b2",             WLAN_CFG_DTS_5G_CORE1_DPN_B2},
    {"dpn5g_core1_b3",             WLAN_CFG_DTS_5G_CORE1_DPN_B3},
    {"dpn5g_core1_b4",             WLAN_CFG_DTS_5G_CORE1_DPN_B4},
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_HOST)
    /* RF REGISTER */
    {"rf_reg117",                                           WLAN_CFG_DTS_RF_REG117},
    {"rf_reg123",                                           WLAN_CFG_DTS_RF_REG123},
    {"rf_reg124",                                           WLAN_CFG_DTS_RF_REG124},
    {"rf_reg125",                                           WLAN_CFG_DTS_RF_REG125},
    {"rf_reg126",                                           WLAN_CFG_DTS_RF_REG126},
    /* bt 校准 */
    {"cali_txpwr_pa_ref_band1",                              WLAN_CFG_DTS_BT_CALI_TXPWR_PA_REF_BAND1},
    {"cali_txpwr_pa_ref_band2",                              WLAN_CFG_DTS_BT_CALI_TXPWR_PA_REF_BAND2},
    {"cali_txpwr_pa_ref_band3",                              WLAN_CFG_DTS_BT_CALI_TXPWR_PA_REF_BAND3},
    {"cali_txpwr_pa_ref_band4",                              WLAN_CFG_DTS_BT_CALI_TXPWR_PA_REF_BAND4},
    {"cali_txpwr_pa_ref_band5",                              WLAN_CFG_DTS_BT_CALI_TXPWR_PA_REF_BAND5},
    {"cali_txpwr_pa_ref_band6",                              WLAN_CFG_DTS_BT_CALI_TXPWR_PA_REF_BAND6},
    {"cali_txpwr_pa_ref_band7",                              WLAN_CFG_DTS_BT_CALI_TXPWR_PA_REF_BAND7},
    {"cali_txpwr_pa_ref_band8",                              WLAN_CFG_DTS_BT_CALI_TXPWR_PA_REF_BAND8},
    {"cali_txpwr_pa_ref_num",                                WLAN_CFG_DTS_BT_CALI_TXPWR_PA_NUM},
    {"cali_txpwr_pa_fre1",                                   WLAN_CFG_DTS_BT_CALI_TXPWR_PA_FRE1},
    {"cali_txpwr_pa_fre2",                                   WLAN_CFG_DTS_BT_CALI_TXPWR_PA_FRE2},
    {"cali_txpwr_pa_fre3",                                   WLAN_CFG_DTS_BT_CALI_TXPWR_PA_FRE3},
    {"cali_txpwr_pa_fre4",                                   WLAN_CFG_DTS_BT_CALI_TXPWR_PA_FRE4},
    {"cali_txpwr_pa_fre5",                                   WLAN_CFG_DTS_BT_CALI_TXPWR_PA_FRE5},
    {"cali_txpwr_pa_fre6",                                   WLAN_CFG_DTS_BT_CALI_TXPWR_PA_FRE6},
    {"cali_txpwr_pa_fre7",                                   WLAN_CFG_DTS_BT_CALI_TXPWR_PA_FRE7},
    {"cali_txpwr_pa_fre8",                                   WLAN_CFG_DTS_BT_CALI_TXPWR_PA_FRE8},
    {"cali_bt_tone_amp_grade",                               WLAN_CFG_DTS_BT_CALI_TONE_AMP_GRADE},
#endif //#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_HOST)

    {OAL_PTR_NULL, 0}
};

OAL_STATIC wlan_cfg_cmd g_ast_wifi_config_priv[] =
{
    /* 校准开关 */
    {"cali_mask",                   WLAN_CFG_PRIV_CALI_MASK},
    /* #bit0:开wifi重新校准 bit1:开wifi重新上传 bit2:开机校准 bit3:动态校准调平Debug
       #bit4:不读取NV区域的数据(1:不读取 0：读取) */
    {"cali_data_mask",              WLAN_CFG_PRIV_CALI_DATA_MASK},
    {"cali_auto_cali_mask",         WLAN_CFG_PRIV_CALI_AUTOCALI_MASK},
    /* TBD:hal_cfg_customize_info_stru/mac_device_capability_stru */
    {"bw_max_width",                WLAN_CFG_PRIV_BW_MAX_WITH},
    {"ldpc_coding",                 WLAN_CFG_PRIV_LDPC_CODING},
    {"rx_stbc",                     WLAN_CFG_PRIV_RX_STBC},
    {"tx_stbc",                     WLAN_CFG_PRIV_TX_STBC},
    {"su_bfer",                     WLAN_CFG_PRIV_SU_BFER},
    {"su_bfee",                     WLAN_CFG_PRIV_SU_BFEE},
    {"mu_bfer",                     WLAN_CFG_PRIV_MU_BFER},
    {"mu_bfee",                     WLAN_CFG_PRIV_MU_BFEE},

    {"11n_txbf",                    WLAN_CFG_PRIV_11N_TXBF},

    {"1024qam_en",                  WLAN_CFG_PRIV_1024_QAM},
    /* DBDC */
    {"radio_cap_0",                 WLAN_CFG_PRIV_DBDC_RADIO_0},
    {"radio_cap_1",                 WLAN_CFG_PRIV_DBDC_RADIO_1},
    {"fastscan_switch",             WLAN_CFG_PRIV_FASTSCAN_SWITCH},

    /* RSSI天线切换 */
    {"rssi_ant_switch",             WLAN_CFG_ANT_SWITCH},

    {"m2s_function_mask",           WLAN_CFG_PRIV_M2S_FUNCTION_MASK},

#ifdef _PRE_WLAN_DOWNLOAD_PM
    {"download_rate_limit_pps",     WLAN_CFG_PRIV_DOWNLOAD_RATE_LIMIT_PPS},
#endif
#ifdef _PRE_WLAN_FEATURE_TXOPPS
    {"txopps_switch",               WLAN_CFG_PRIV_TXOPPS_SWITCH},
#endif
    {"over_temper_protect_threshold",  WLAN_CFG_PRIV_OVER_TEMPER_PROTECT_THRESHOLD},
    {"over_temp_pro_enable",           WLAN_CFG_PRIV_OVER_TEMP_PRO_ENABLE},
    {"over_temp_pro_reduce_pwr_enable",WLAN_CFG_PRIV_OVER_TEMP_PRO_REDUCE_PWR_ENABLE},
    {"over_temp_pro_safe_th",          WLAN_CFG_PRIV_OVER_TEMP_PRO_SAFE_TH},
    {"over_temp_pro_over_th",          WLAN_CFG_PRIV_OVER_TEMP_PRO_OVER_TH},
    {"over_temp_pro_pa_off_th",        WLAN_CFG_PRIV_OVER_TEMP_PRO_PA_OFF_TH},

    {"dsss2ofdm_dbb_pwr_bo_val",       WLAN_DSSS2OFDM_DBB_PWR_BO_VAL},
    {"evm_fail_pll_reg_fix",           WLAN_CFG_PRIV_EVM_PLL_REG_FIX},
    {"voe_switch_mask",                WLAN_CFG_PRIV_VOE_SWITCH},
    {"11ax_switch_mask",               WLAN_CFG_PRIV_11AX_SWITCH},
    {"dyn_bypass_extlna_enable",       WLAN_CFG_PRIV_DYN_BYPASS_EXTLNA},
    {"ctrl_frame_tx_chain",            WLAN_CFG_PRIV_CTRL_FRAME_TX_CHAIN},
    {"upc_cali_code_for_18dBm_c0",     WLAN_CFG_PRIV_CTRL_UPC_FOR_18DBM_CO},
    {"upc_cali_code_for_18dBm_c1",     WLAN_CFG_PRIV_CTRL_UPC_FOR_18DBM_CO},
    {OAL_PTR_NULL, 0}
};


OAL_STATIC wlan_cfg_nv_map_handler g_ast_wifi_nvram_cfg_handler[WLAN_CFG_DTS_NVRAM_END]=
{
#ifdef _PRE_WLAN_DPINIT_CALI
    {"WITXBW0",  "dp2ginit0",   NULL,      WLAN_CFG_NVRAM_DP2G_INIT0,          {0}},
    {"WITXBW1",  "dp2ginit1",   NULL,      WLAN_CFG_NVRAM_DP2G_INIT1,          {0}},
#endif
    {"WITXCCK",  "pa2gccka0",   HWIFI_CFG_NV_WITXNVCCK_NUMBER,       WLAN_CFG_DTS_NVRAM_RATIO_PA2GCCKA0, {0}},
    {"WINVRAM",  "pa2ga0",      HWIFI_CFG_NV_WINVRAM_NUMBER,         WLAN_CFG_NVRAM_RATIO_PA2GA0,        {0}},
    {"WITXLC0" , "pa2g40a0",    HWIFI_CFG_NV_WITXL2G5G0_NUMBER,      WLAN_CFG_DTS_NVRAM_RATIO_PA2G40A0,  {0}},
    {"WINVRAM",  "pa5ga0",      HWIFI_CFG_NV_WINVRAM_NUMBER,         WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0,    {0}},
    {"WITXCCK",  "pa2gccka1",   HWIFI_CFG_NV_WITXNVCCK_NUMBER,       WLAN_CFG_DTS_NVRAM_RATIO_PA2GCCKA1, {0}},
    {"WITXRF1",  "pa2ga1",      HWIFI_CFG_NV_WITXNVC1_NUMBER,        WLAN_CFG_DTS_NVRAM_RATIO_PA2GA1,    {0}},
    {"WITXLC1" , "pa2g40a1",    HWIFI_CFG_NV_WITXL2G5G1_NUMBER,      WLAN_CFG_DTS_NVRAM_RATIO_PA2G40A1,  {0}},
    {"WITXRF1",  "pa5ga1",      HWIFI_CFG_NV_WITXNVC1_NUMBER,        WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1,    {0}},
    {"WIC0_5GLOW",  "pa5glowa0",  HWIFI_CFG_NV_WITXNVBWC0_NUMBER,    WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0_LOW,  {0}},
    {"WIC1_5GLOW",  "pa5glowa1",  HWIFI_CFG_NV_WITXNVBWC1_NUMBER,    WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1_LOW,  {0}},
    //DPN
    {"WIC0CCK",  "mf2gccka0",   HWIFI_CFG_NV_MUFREQ_CCK_C0_NUMBER,   WLAN_CFG_DTS_NVRAM_MUFREQ_2GCCK_C0,  {0}},
    {"WC0OFDM",  "mf2ga0",      HWIFI_CFG_NV_MUFREQ_2G20_C0_NUMBER,  WLAN_CFG_DTS_NVRAM_MUFREQ_2G20_C0,   {0}},
    {"C02G40M",  "mf2g40a0",    HWIFI_CFG_NV_MUFREQ_2G40_C0_NUMBER,  WLAN_CFG_DTS_NVRAM_MUFREQ_2G40_C0,   {0}},
    {"WIC1CCK",  "mf2gccka1",   HWIFI_CFG_NV_MUFREQ_CCK_C1_NUMBER,   WLAN_CFG_DTS_NVRAM_MUFREQ_2GCCK_C1,  {0}},
    {"WC1OFDM",  "mf2ga1",      HWIFI_CFG_NV_MUFREQ_2G20_C1_NUMBER,  WLAN_CFG_DTS_NVRAM_MUFREQ_2G20_C1,   {0}},
    {"C12G40M",  "mf2g40a1",    HWIFI_CFG_NV_MUFREQ_2G40_C1_NUMBER,  WLAN_CFG_DTS_NVRAM_MUFREQ_2G40_C1,   {0}},
    {"WIC0_5G160M",  "dpn160c0",    HWIFI_CFG_NV_MUFREQ_5G160_C0_NUMBER,  WLAN_CFG_DTS_NVRAM_MUFREQ_5G160_C0,  {0}},
    {"WIC1_5G160M",  "dpn160c1",    HWIFI_CFG_NV_MUFREQ_5G160_C1_NUMBER,  WLAN_CFG_DTS_NVRAM_MUFREQ_5G160_C1,  {0}},
};


OAL_STATIC wlan_cfg_cmd g_ast_wifi_config_cmds[] =
{
    /* ROAM */
    {"roam_switch",                     WLAN_CFG_INIT_ROAM_SWITCH},
    {"scan_orthogonal",                 WLAN_CFG_INIT_SCAN_ORTHOGONAL},
    {"trigger_b",                       WLAN_CFG_INIT_TRIGGER_B},
    {"trigger_a",                       WLAN_CFG_INIT_TRIGGER_A},
    {"delta_b",                         WLAN_CFG_INIT_DELTA_B},
    {"delta_a",                         WLAN_CFG_INIT_DELTA_A},
    {"dense_env_trigger_b",             WLAN_CFG_INIT_DENSE_ENV_TRIGGER_B},
    {"dense_env_trigger_a",             WLAN_CFG_INIT_DENSE_ENV_TRIGGER_A},
    {"scenario_enable",                 WLAN_CFG_INIT_SCENARIO_ENABLE},
    {"candidate_good_rssi",             WLAN_CFG_INIT_CANDIDATE_GOOD_RSSI},
    {"candidate_good_num",              WLAN_CFG_INIT_CANDIDATE_GOOD_NUM},
    {"candidate_weak_num",              WLAN_CFG_INIT_CANDIDATE_WEAK_NUM},

    /* 性能 */
    {"ampdu_tx_max_num",                WLAN_CFG_INIT_AMPDU_TX_MAX_NUM},
    {"used_mem_for_start",              WLAN_CFG_INIT_USED_MEM_FOR_START},
    {"used_mem_for_stop",               WLAN_CFG_INIT_USED_MEM_FOR_STOP},
    {"rx_ack_limit",                    WLAN_CFG_INIT_RX_ACK_LIMIT},
    {"sdio_d2h_assemble_count",         WLAN_CFG_INIT_SDIO_D2H_ASSEMBLE_COUNT},
    {"sdio_h2d_assemble_count",         WLAN_CFG_INIT_SDIO_H2D_ASSEMBLE_COUNT},
    /* LINKLOSS */
    {"link_loss_threshold_bt",          WLAN_CFG_INIT_LINK_LOSS_THRESHOLD_BT},
    {"link_loss_threshold_dbac",        WLAN_CFG_INIT_LINK_LOSS_THRESHOLD_DBAC},
    {"link_loss_threshold_normal",      WLAN_CFG_INIT_LINK_LOSS_THRESHOLD_NORMAL},
    /* 自动调频 */
#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
    {"pss_threshold_level_0",           WLAN_CFG_INIT_PSS_THRESHOLD_LEVEL_0},
    {"cpu_freq_limit_level_0",          WLAN_CFG_INIT_CPU_FREQ_LIMIT_LEVEL_0},
    {"ddr_freq_limit_level_0",          WLAN_CFG_INIT_DDR_FREQ_LIMIT_LEVEL_0},
    {"pss_threshold_level_1",           WLAN_CFG_INIT_PSS_THRESHOLD_LEVEL_1},
    {"cpu_freq_limit_level_1",          WLAN_CFG_INIT_CPU_FREQ_LIMIT_LEVEL_1},
    {"ddr_freq_limit_level_1",          WLAN_CFG_INIT_DDR_FREQ_LIMIT_LEVEL_1},
    {"pss_threshold_level_2",           WLAN_CFG_INIT_PSS_THRESHOLD_LEVEL_2},
    {"cpu_freq_limit_level_2",          WLAN_CFG_INIT_CPU_FREQ_LIMIT_LEVEL_2},
    {"ddr_freq_limit_level_2",          WLAN_CFG_INIT_DDR_FREQ_LIMIT_LEVEL_2},
    {"pss_threshold_level_3",           WLAN_CFG_INIT_PSS_THRESHOLD_LEVEL_3},
    {"cpu_freq_limit_level_3",          WLAN_CFG_INIT_CPU_FREQ_LIMIT_LEVEL_3},
    {"ddr_freq_limit_level_3",          WLAN_CFG_INIT_DDR_FREQ_LIMIT_LEVEL_3},
    {"device_type_level_0",             WLAN_CFG_INIT_DEVICE_TYPE_LEVEL_0},
    {"device_type_level_1",             WLAN_CFG_INIT_DEVICE_TYPE_LEVEL_1},
    {"device_type_level_2",             WLAN_CFG_INIT_DEVICE_TYPE_LEVEL_2},
    {"device_type_level_3",             WLAN_CFG_INIT_DEVICE_TYPE_LEVEL_3},
#endif
    /* 收发中断动态绑核 */
    {"irq_affinity",                    WLAN_CFG_INIT_IRQ_AFFINITY},
    {"cpu_id_th_low",                   WLAN_CFG_INIT_IRQ_TH_LOW},
    {"cpu_id_th_high",                  WLAN_CFG_INIT_IRQ_TH_HIGH},
    {"cpu_id_pps_th_low",               WLAN_CFG_INIT_IRQ_PPS_TH_LOW},
    {"cpu_id_pps_th_high",              WLAN_CFG_INIT_IRQ_PPS_TH_HIGH},
#ifdef _PRE_WLAN_FEATURE_AMPDU_TX_HW
    /* 硬件聚合使能 */
    {"hw_ampdu",                        WLAN_CFG_INIT_HW_AMPDU},
    {"hw_ampdu_th_l",                   WLAN_CFG_INIT_HW_AMPDU_TH_LOW},
    {"hw_ampdu_th_h",                   WLAN_CFG_INIT_HW_AMPDU_TH_HIGH},
#endif
#ifdef _PRE_WLAN_FEATURE_MULTI_NETBUF_AMSDU
    {"tx_amsdu_ampdu",                  WLAN_CFG_INIT_AMPDU_AMSDU_SKB},
    {"tx_amsdu_ampdu_th_l",             WLAN_CFG_INIT_AMSDU_AMPDU_TH_LOW},
    {"tx_amsdu_ampdu_th_h",             WLAN_CFG_INIT_AMSDU_AMPDU_TH_HIGH},
#endif
#ifdef _PRE_WLAN_TCP_OPT
    {"en_tcp_ack_filter",               WLAN_CFG_INIT_TCP_ACK_FILTER},
    {"rx_tcp_ack_filter_th_l",          WLAN_CFG_INIT_TCP_ACK_FILTER_TH_LOW},
    {"rx_tcp_ack_filter_th_h",          WLAN_CFG_INIT_TCP_ACK_FILTER_TH_HIGH},
#endif

    {"small_amsdu_switch",              WLAN_CFG_INIT_TX_SMALL_AMSDU},
    {"small_amsdu_th_h",                WLAN_CFG_INIT_SMALL_AMSDU_HIGH},
    {"small_amsdu_th_l",                WLAN_CFG_INIT_SMALL_AMSDU_LOW},
    {"small_amsdu_pps_th_h",            WLAN_CFG_INIT_SMALL_AMSDU_PPS_HIGH},
    {"small_amsdu_pps_th_l",            WLAN_CFG_INIT_SMALL_AMSDU_PPS_LOW},

    {"tcp_ack_buf_switch",              WLAN_CFG_INIT_TX_TCP_ACK_BUF},
    {"tcp_ack_buf_th_h",                WLAN_CFG_INIT_TCP_ACK_BUF_HIGH},
    {"tcp_ack_buf_th_l",                WLAN_CFG_INIT_TCP_ACK_BUF_LOW},
    {"tcp_ack_buf_th_h_40M",            WLAN_CFG_INIT_TCP_ACK_BUF_HIGH_40M},
    {"tcp_ack_buf_th_l_40M",            WLAN_CFG_INIT_TCP_ACK_BUF_LOW_40M},
    {"tcp_ack_buf_th_h_80M",            WLAN_CFG_INIT_TCP_ACK_BUF_HIGH_80M},
    {"tcp_ack_buf_th_l_80M",            WLAN_CFG_INIT_TCP_ACK_BUF_LOW_80M},
    {"tcp_ack_buf_th_h_160M",           WLAN_CFG_INIT_TCP_ACK_BUF_HIGH_160M},
    {"tcp_ack_buf_th_l_160M",           WLAN_CFG_INIT_TCP_ACK_BUF_LOW_160M},

    {"dyn_bypass_extlna_th_switch",     WLAN_CFG_INIT_RX_DYN_BYPASS_EXTLNA},
    {"dyn_bypass_extlna_th_h",          WLAN_CFG_INIT_RX_DYN_BYPASS_EXTLNA_HIGH},
    {"dyn_bypass_extlna_th_l",          WLAN_CFG_INIT_RX_DYN_BYPASS_EXTLNA_LOW},

    {"rx_ampdu_amsdu",                  WLAN_CFG_INIT_RX_AMPDU_AMSDU_SKB},

    /* 低功耗 */
    {"powermgmt_switch",                WLAN_CFG_INIT_POWERMGMT_SWITCH},

    {"ps_mode",                         WLAN_CFG_INIT_PS_MODE},
    {"ps_fast_check_cnt",               WLAN_CFG_INIT_FAST_CHECK_CNT},
    /* 可维可测 */
    {"loglevel",                        WLAN_CFG_INIT_LOGLEVEL},
    /* 2G RF前端插损 */
    {"rf_rx_insertion_loss_2g_b1",     WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_2G_BAND1},
    {"rf_rx_insertion_loss_2g_b2",     WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_2G_BAND2},
    {"rf_rx_insertion_loss_2g_b3",     WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_2G_BAND3},
    /* 5G RF前端插损 */
    {"rf_rx_insertion_loss_5g_b1",     WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND1},
    {"rf_rx_insertion_loss_5g_b2",     WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND2},
    {"rf_rx_insertion_loss_5g_b3",     WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND3},
    {"rf_rx_insertion_loss_5g_b4",     WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND4},
    {"rf_rx_insertion_loss_5g_b5",     WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND5},
    {"rf_rx_insertion_loss_5g_b6",     WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND6},
    {"rf_rx_insertion_loss_5g_b7",     WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND7},

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST)
    /* 用于定制化计算PWR RF值的偏差 */
    {"rf_line_rf_pwr_ref_rssi_db_2g_c0_mult4",     WLAN_CFG_INIT_RF_PWR_REF_RSSI_2G_C0_MULT4},
    {"rf_line_rf_pwr_ref_rssi_db_2g_c1_mult4",     WLAN_CFG_INIT_RF_PWR_REF_RSSI_2G_C1_MULT4},
    {"rf_line_rf_pwr_ref_rssi_db_5g_c0_mult4",     WLAN_CFG_INIT_RF_PWR_REF_RSSI_5G_C0_MULT4},
    {"rf_line_rf_pwr_ref_rssi_db_5g_c1_mult4",     WLAN_CFG_INIT_RF_PWR_REF_RSSI_5G_C1_MULT4},
#endif
    /* fem */
    {"rf_lna_bypass_gain_db_2g",        WLAN_CFG_INIT_RF_LNA_BYPASS_GAIN_DB_2G},
    {"rf_lna_gain_db_2g",               WLAN_CFG_INIT_RF_LNA_GAIN_DB_2G},
    {"rf_pa_db_b0_2g",                  WLAN_CFG_INIT_RF_PA_GAIN_DB_B0_2G},
    {"rf_pa_db_b1_2g",                  WLAN_CFG_INIT_RF_PA_GAIN_DB_B1_2G},
    {"rf_pa_db_lvl_2g",                 WLAN_CFG_INIT_RF_PA_GAIN_LVL_2G},
    {"ext_switch_isexist_2g",           WLAN_CFG_INIT_EXT_SWITCH_ISEXIST_2G},
    {"ext_pa_isexist_2g",               WLAN_CFG_INIT_EXT_PA_ISEXIST_2G},
    {"ext_lna_isexist_2g",              WLAN_CFG_INIT_EXT_LNA_ISEXIST_2G},
    {"lna_on2off_time_ns_2g",           WLAN_CFG_INIT_LNA_ON2OFF_TIME_NS_2G},
    {"lna_off2on_time_ns_2g",           WLAN_CFG_INIT_LNA_OFF2ON_TIME_NS_2G},
    {"rf_lna_bypass_gain_db_5g",        WLAN_CFG_INIT_RF_LNA_BYPASS_GAIN_DB_5G},
    {"rf_lna_gain_db_5g",               WLAN_CFG_INIT_RF_LNA_GAIN_DB_5G},
    {"rf_pa_db_b0_5g",                  WLAN_CFG_INIT_RF_PA_GAIN_DB_B0_5G},
    {"rf_pa_db_b1_5g",                  WLAN_CFG_INIT_RF_PA_GAIN_DB_B1_5G},
    {"rf_pa_db_lvl_5g",                 WLAN_CFG_INIT_RF_PA_GAIN_LVL_5G},
    {"ext_switch_isexist_5g",           WLAN_CFG_INIT_EXT_SWITCH_ISEXIST_5G},
    {"ext_pa_isexist_5g",               WLAN_CFG_INIT_EXT_PA_ISEXIST_5G},
    {"ext_lna_isexist_5g",              WLAN_CFG_INIT_EXT_LNA_ISEXIST_5G},
    {"lna_on2off_time_ns_5g",           WLAN_CFG_INIT_LNA_ON2OFF_TIME_NS_5G},
    {"lna_off2on_time_ns_5g",           WLAN_CFG_INIT_LNA_OFF2ON_TIME_NS_5G},
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_HOST)
    /* 温度上升导致发射功率下降过多的功率补偿 */
    {"tx_ratio_level_0",                WLAN_CFG_INIT_TX_RATIO_LEVEL_0},
    {"tx_pwr_comp_val_level_0",         WLAN_CFG_INIT_TX_PWR_COMP_VAL_LEVEL_0},
    {"tx_ratio_level_1",                WLAN_CFG_INIT_TX_RATIO_LEVEL_1},
    {"tx_pwr_comp_val_level_1",         WLAN_CFG_INIT_TX_PWR_COMP_VAL_LEVEL_1},
    {"tx_ratio_level_2",                WLAN_CFG_INIT_TX_RATIO_LEVEL_2},
    {"tx_pwr_comp_val_level_2",         WLAN_CFG_INIT_TX_PWR_COMP_VAL_LEVEL_2},
    {"more_pwr",                        WLAN_CFG_INIT_MORE_PWR},
#endif //#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_HOST)
    /* SCAN */
    {"random_mac_addr_scan",            WLAN_CFG_INIT_RANDOM_MAC_ADDR_SCAN},
    /* 11AC2G */
    {"11ac2g_enable",                   WLAN_CFG_INIT_11AC2G_ENABLE},
    {"disable_capab_2ght40",            WLAN_CFG_INIT_DISABLE_CAPAB_2GHT40},
    {"dual_antenna_enable",             WLAN_CFG_INIT_DUAL_ANTENNA_ENABLE}, /* 双天线开关 */
    /* sta keepalive cnt th*/
    {"sta_keepalive_cnt_th",            WLAN_CFG_INIT_STA_KEEPALIVE_CNT_TH}, /* 动态功率校准 */

    {"far_dist_pow_gain_switch",            WLAN_CFG_INIT_FAR_DIST_POW_GAIN_SWITCH},
    {"far_dist_dsss_scale_promote_switch",  WLAN_CFG_INIT_FAR_DIST_DSSS_SCALE_PROMOTE_SWITCH},
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST)
    {"chann_radio_cap",                 WLAN_CFG_INIT_CHANN_RADIO_CAP},

    {"lte_gpio_check_switch",           WLAN_CFG_LTE_GPIO_CHECK_SWITCH},/* lte?????? */
    {"ism_priority",                    WLAN_ATCMDSRV_ISM_PRIORITY},
    {"lte_rx",                          WLAN_ATCMDSRV_LTE_RX},
    {"lte_tx",                          WLAN_ATCMDSRV_LTE_TX},
    {"lte_inact",                       WLAN_ATCMDSRV_LTE_INACT},
    {"ism_rx_act",                      WLAN_ATCMDSRV_ISM_RX_ACT},
    {"bant_pri",                        WLAN_ATCMDSRV_BANT_PRI},
    {"bant_status",                     WLAN_ATCMDSRV_BANT_STATUS},
    {"want_pri",                        WLAN_ATCMDSRV_WANT_PRI},
    {"want_status",                     WLAN_ATCMDSRV_WANT_STATUS},
    {"tx5g_upc_mix_gain_ctrl_1",        WLAN_TX5G_UPC_MIX_GAIN_CTRL_1},
    {"tx5g_upc_mix_gain_ctrl_2",        WLAN_TX5G_UPC_MIX_GAIN_CTRL_2},
    {"tx5g_upc_mix_gain_ctrl_3",        WLAN_TX5G_UPC_MIX_GAIN_CTRL_3},
    {"tx5g_upc_mix_gain_ctrl_4",        WLAN_TX5G_UPC_MIX_GAIN_CTRL_4},
    {"tx5g_upc_mix_gain_ctrl_5",        WLAN_TX5G_UPC_MIX_GAIN_CTRL_5},
    {"tx5g_upc_mix_gain_ctrl_6",        WLAN_TX5G_UPC_MIX_GAIN_CTRL_6},
    {"tx5g_upc_mix_gain_ctrl_7",        WLAN_TX5G_UPC_MIX_GAIN_CTRL_7},
    /* 定制化RF部分PA偏置寄存器 */
    {"tx2g_pa_gate_236",                WLAN_TX2G_PA_GATE_VCTL_REG236},
    {"tx2g_pa_gate_237",                WLAN_TX2G_PA_GATE_VCTL_REG237},
    {"tx2g_pa_gate_238",                WLAN_TX2G_PA_GATE_VCTL_REG238},
    {"tx2g_pa_gate_239",                WLAN_TX2G_PA_GATE_VCTL_REG239},
    {"tx2g_pa_gate_240",                WLAN_TX2G_PA_GATE_VCTL_REG240},
    {"tx2g_pa_gate_241",                WLAN_TX2G_PA_GATE_VCTL_REG241},
    {"tx2g_pa_gate_242",                WLAN_TX2G_PA_GATE_VCTL_REG242},
    {"tx2g_pa_gate_243",                WLAN_TX2G_PA_GATE_VCTL_REG243},
    {"tx2g_pa_gate_244",                WLAN_TX2G_PA_GATE_VCTL_REG244},

    {"tx2g_pa_gate_253",         WLAN_TX2G_PA_VRECT_GATE_THIN_REG253},
    {"tx2g_pa_gate_254",         WLAN_TX2G_PA_VRECT_GATE_THIN_REG254},
    {"tx2g_pa_gate_255",         WLAN_TX2G_PA_VRECT_GATE_THIN_REG255},
    {"tx2g_pa_gate_256",         WLAN_TX2G_PA_VRECT_GATE_THIN_REG256},
    {"tx2g_pa_gate_257",         WLAN_TX2G_PA_VRECT_GATE_THIN_REG257},
    {"tx2g_pa_gate_258",         WLAN_TX2G_PA_VRECT_GATE_THIN_REG258},
    {"tx2g_pa_gate_259",         WLAN_TX2G_PA_VRECT_GATE_THIN_REG259},
    {"tx2g_pa_gate_260",         WLAN_TX2G_PA_VRECT_GATE_THIN_REG260},
    {"tx2g_pa_gate_261",         WLAN_TX2G_PA_VRECT_GATE_THIN_REG261},
    {"tx2g_pa_gate_262",         WLAN_TX2G_PA_VRECT_GATE_THIN_REG262},
    {"tx2g_pa_gate_263",         WLAN_TX2G_PA_VRECT_GATE_THIN_REG263},
    {"tx2g_pa_gate_264",         WLAN_TX2G_PA_VRECT_GATE_THIN_REG264},
    {"tx2g_pa_gate_265",         WLAN_TX2G_PA_VRECT_GATE_THIN_REG265},
    {"tx2g_pa_gate_266",         WLAN_TX2G_PA_VRECT_GATE_THIN_REG266},
    {"tx2g_pa_gate_267",         WLAN_TX2G_PA_VRECT_GATE_THIN_REG267},
    {"tx2g_pa_gate_268",         WLAN_TX2G_PA_VRECT_GATE_THIN_REG268},
    {"tx2g_pa_gate_269",         WLAN_TX2G_PA_VRECT_GATE_THIN_REG269},
    {"tx2g_pa_gate_270",         WLAN_TX2G_PA_VRECT_GATE_THIN_REG270},
    {"tx2g_pa_gate_271",         WLAN_TX2G_PA_VRECT_GATE_THIN_REG271},
    {"tx2g_pa_gate_272",         WLAN_TX2G_PA_VRECT_GATE_THIN_REG272},
    {"tx2g_pa_gate_273",         WLAN_TX2G_PA_VRECT_GATE_THIN_REG273},
    {"tx2g_pa_gate_274",         WLAN_TX2G_PA_VRECT_GATE_THIN_REG274},
    {"tx2g_pa_gate_275",         WLAN_TX2G_PA_VRECT_GATE_THIN_REG275},
    {"tx2g_pa_gate_276",         WLAN_TX2G_PA_VRECT_GATE_THIN_REG276},
    {"tx2g_pa_gate_277",                WLAN_TX2G_PA_VRECT_GATE_THIN_REG277},
    {"tx2g_pa_gate_278",                WLAN_TX2G_PA_VRECT_GATE_THIN_REG278},
    {"tx2g_pa_gate_279",                WLAN_TX2G_PA_VRECT_GATE_THIN_REG279},
    {"tx2g_pa_gate_280_band1",          WLAN_TX2G_PA_VRECT_GATE_THIN_REG280_BAND1},
    {"tx2g_pa_gate_281",                WLAN_TX2G_PA_VRECT_GATE_THIN_REG281},
    {"tx2g_pa_gate_282",                WLAN_TX2G_PA_VRECT_GATE_THIN_REG282},
    {"tx2g_pa_gate_283",                WLAN_TX2G_PA_VRECT_GATE_THIN_REG283},
    {"tx2g_pa_gate_284",                WLAN_TX2G_PA_VRECT_GATE_THIN_REG284},
    {"tx2g_pa_gate_280_band2",          WLAN_TX2G_PA_VRECT_GATE_THIN_REG280_BAND2},
    {"tx2g_pa_gate_280_band3",          WLAN_TX2G_PA_VRECT_GATE_THIN_REG280_BAND3},
#endif
    {"delta_cca_ed_high_20th_2g",       WLAN_CFG_INIT_DELTA_CCA_ED_HIGH_20TH_2G},
    {"delta_cca_ed_high_40th_2g",       WLAN_CFG_INIT_DELTA_CCA_ED_HIGH_40TH_2G},
    {"delta_cca_ed_high_20th_5g",       WLAN_CFG_INIT_DELTA_CCA_ED_HIGH_20TH_5G},
    {"delta_cca_ed_high_40th_5g",       WLAN_CFG_INIT_DELTA_CCA_ED_HIGH_40TH_5G},
    {"voe_switch_mask",                 WLAN_CFG_INIT_VOE_SWITCH},
    {"11ax_switch_mask",                WLAN_CFG_INIT_11AX_SWITCH},
    /* ldac m2s rssi */
    {"ldac_threshold_m2s",          WLAN_CFG_INIT_LDAC_THRESHOLD_M2S},
    {"ldac_threshold_s2m",          WLAN_CFG_INIT_LDAC_THRESHOLD_S2M},
    {OAL_PTR_NULL, 0}
};

OAL_STATIC wlan_cfg_cmd g_ast_nvram_config_ini[NVRAM_PARAMS_PWR_INDEX_BUTT] =
{
    {"nvram_params0",                     NVRAM_PARAMS_INDEX_0},
    {"nvram_params1",                     NVRAM_PARAMS_INDEX_1},
    {"nvram_params2",                     NVRAM_PARAMS_INDEX_2},
    {"nvram_params3",                     NVRAM_PARAMS_INDEX_3},
    {"nvram_params4",                     NVRAM_PARAMS_INDEX_4},
    {"nvram_params5",                     NVRAM_PARAMS_INDEX_5},
    {"nvram_params6",                     NVRAM_PARAMS_INDEX_6},
    {"nvram_params7",                     NVRAM_PARAMS_INDEX_7},
    {"nvram_params8",                     NVRAM_PARAMS_INDEX_8},
    {"nvram_params9",                     NVRAM_PARAMS_INDEX_9},
    {"nvram_params10",                    NVRAM_PARAMS_INDEX_10},
    {"nvram_params11",                    NVRAM_PARAMS_INDEX_11},
    {"nvram_params12",                    NVRAM_PARAMS_INDEX_12},
    {"nvram_params13",                    NVRAM_PARAMS_INDEX_13},
    {"nvram_params14",                    NVRAM_PARAMS_INDEX_14},
    {"nvram_params15",                    NVRAM_PARAMS_INDEX_15},
    {"nvram_params16",                    NVRAM_PARAMS_INDEX_16},
    {"nvram_params17",                    NVRAM_PARAMS_INDEX_17},
    {"nvram_params59",                    NVRAM_PARAMS_INDEX_DPD_0},
    {"nvram_params60",                    NVRAM_PARAMS_INDEX_DPD_1},
    {"nvram_params61",                    NVRAM_PARAMS_INDEX_DPD_2},
    /* 11B & OFDM delta power */
    {"nvram_params62",                    NVRAM_PARAMS_INDEX_62},
    /* 5G cali upper upc limit */
    {"nvram_params63",                    NVRAM_PARAMS_INDEX_63},
    {OAL_PTR_NULL,                        NVRAM_PARAMS_TXPWR_INDEX_BUTT},
    {"nvram_max_txpwr_base_2p4g",         NVRAM_PARAMS_INDEX_19},
    {"nvram_max_txpwr_base_5g",           NVRAM_PARAMS_INDEX_20},
    {"nvram_max_txpwr_base_2p4g_slave",   NVRAM_PARAMS_INDEX_21},
    {"nvram_max_txpwr_base_5g_slave",     NVRAM_PARAMS_INDEX_22},
    {OAL_PTR_NULL,                        NVRAM_PARAMS_BASE_INDEX_BUTT},
    {OAL_PTR_NULL,  NVRAM_PARAMS_INDEX_23_RSV},
    {OAL_PTR_NULL,  NVRAM_PARAMS_INDEX_24_RSV},
    /* FCC & SAR */
    {"side_band_txpwr_limit_5g_20m_0",  NVRAM_PARAMS_INDEX_25},
    {"side_band_txpwr_limit_5g_20m_1",  NVRAM_PARAMS_INDEX_26},
    {"side_band_txpwr_limit_5g_40m_0",  NVRAM_PARAMS_INDEX_27},
    {"side_band_txpwr_limit_5g_40m_1",  NVRAM_PARAMS_INDEX_28},
    {"side_band_txpwr_limit_5g_80m_0",  NVRAM_PARAMS_INDEX_29},
    {"side_band_txpwr_limit_5g_80m_1",  NVRAM_PARAMS_INDEX_30},
    {"side_band_txpwr_limit_5g_160m",   NVRAM_PARAMS_INDEX_31},
    {"side_band_txpwr_limit_24g_ch1",   NVRAM_PARAMS_INDEX_32},
    {"side_band_txpwr_limit_24g_ch2",   NVRAM_PARAMS_INDEX_33},
    {"side_band_txpwr_limit_24g_ch3",   NVRAM_PARAMS_INDEX_34},
    {"side_band_txpwr_limit_24g_ch4",   NVRAM_PARAMS_INDEX_35},
    {"side_band_txpwr_limit_24g_ch5",   NVRAM_PARAMS_INDEX_36},
    {"side_band_txpwr_limit_24g_ch6",   NVRAM_PARAMS_INDEX_37},
    {"side_band_txpwr_limit_24g_ch7",   NVRAM_PARAMS_INDEX_38},
    {"side_band_txpwr_limit_24g_ch8",   NVRAM_PARAMS_INDEX_39},
    {"side_band_txpwr_limit_24g_ch9",   NVRAM_PARAMS_INDEX_40},
    {"side_band_txpwr_limit_24g_ch10",  NVRAM_PARAMS_INDEX_41},
    {"side_band_txpwr_limit_24g_ch11",  NVRAM_PARAMS_INDEX_42},
    {"side_band_txpwr_limit_24g_ch12",  NVRAM_PARAMS_INDEX_43},
    {"side_band_txpwr_limit_24g_ch13",  NVRAM_PARAMS_INDEX_44},
    {OAL_PTR_NULL,                      NVRAM_PARAMS_FCC_END_INDEX_BUTT},
    {"sar_txpwr_ctrl_5g_band1",   NVRAM_PARAMS_INDEX_45},
    {"sar_txpwr_ctrl_5g_band2",   NVRAM_PARAMS_INDEX_46},
    {"sar_txpwr_ctrl_5g_band3",   NVRAM_PARAMS_INDEX_47},
    {"sar_txpwr_ctrl_5g_band4",   NVRAM_PARAMS_INDEX_48},
    {"sar_txpwr_ctrl_5g_band5",   NVRAM_PARAMS_INDEX_49},
    {"sar_txpwr_ctrl_5g_band6",   NVRAM_PARAMS_INDEX_50},
    {"sar_txpwr_ctrl_5g_band7",   NVRAM_PARAMS_INDEX_51},
    {"sar_txpwr_ctrl_2g",         NVRAM_PARAMS_INDEX_52},
    {OAL_PTR_NULL,                NVRAM_PARAMS_SAR_END_INDEX_BUTT},
#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH
    {"tas_ant_switch_en",         NVRAM_PARAMS_TAS_ANT_SWITCH_EN},
    {"tas_txpwr_ctrl_params",     NVRAM_PARAMS_TAS_PWR_CTRL},
#endif
    {"5g_max_pow_high_band_ce",   NVRAM_PARAMS_5G_CE_HIGH_BAND_MAX_PWR},
};

OAL_STATIC wlan_cfg_cmd g_ast_nvram_pro_line_config_ini[] =
{
#ifdef _PRE_WLAN_DPINIT_CALI
    {OAL_PTR_NULL,           WLAN_CFG_NVRAM_DP2G_INIT0},
    {OAL_PTR_NULL,           WLAN_CFG_NVRAM_DP2G_INIT1},
#endif
    /* 产侧nvram参数 */
    {"nvram_pa2gccka0",      WLAN_CFG_DTS_NVRAM_RATIO_PA2GCCKA0},
    {"nvram_pa2ga0",         WLAN_CFG_NVRAM_RATIO_PA2GA0},
    {"nvram_pa2g40a0",       WLAN_CFG_DTS_NVRAM_RATIO_PA2G40A0},
    {"nvram_pa5ga0",         WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0},
    {"nvram_pa2gccka1",      WLAN_CFG_DTS_NVRAM_RATIO_PA2GCCKA1},
    {"nvram_pa2ga1",         WLAN_CFG_DTS_NVRAM_RATIO_PA2GA1},
    {"nvram_pa2g40a1",       WLAN_CFG_DTS_NVRAM_RATIO_PA2G40A1},
    {"nvram_pa5ga1",         WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1},
    {"nvram_pa5ga0_low",     WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0_LOW},
    {"nvram_pa5ga1_low",     WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1_LOW},

    {OAL_PTR_NULL,           WLAN_CFG_DTS_NVRAM_MUFREQ_2GCCK_C0},
    {OAL_PTR_NULL,           WLAN_CFG_DTS_NVRAM_MUFREQ_2G20_C0},
    {OAL_PTR_NULL,           WLAN_CFG_DTS_NVRAM_MUFREQ_2G40_C0},
    {OAL_PTR_NULL,           WLAN_CFG_DTS_NVRAM_MUFREQ_2GCCK_C1},
    {OAL_PTR_NULL,           WLAN_CFG_DTS_NVRAM_MUFREQ_2G20_C1},
    {OAL_PTR_NULL,           WLAN_CFG_DTS_NVRAM_MUFREQ_2G40_C1},
    {OAL_PTR_NULL,           WLAN_CFG_DTS_NVRAM_MUFREQ_5G160_C0},
    {OAL_PTR_NULL,           WLAN_CFG_DTS_NVRAM_MUFREQ_5G160_C1},

    {"nvram_pa5ga0_band1",      WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0_BAND1},
    {"nvram_pa5ga1_band1",      WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1_BAND1},
    {"nvram_pa2gcwa0",          WLAN_CFG_DTS_NVRAM_RATIO_PA2GCWA0},
    {"nvram_pa2gcwa1",          WLAN_CFG_DTS_NVRAM_RATIO_PA2GCWA1},
    {"nvram_pa5ga0_band1_low",  WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0_BAND1_LOW},
    {"nvram_pa5ga1_band1_low",  WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1_BAND1_LOW},

    {"nvram_ppa2gcwa0",         WLAN_CFG_DTS_NVRAM_RATIO_PPA2GCWA0},
    {"nvram_ppa2gcwa1",         WLAN_CFG_DTS_NVRAM_RATIO_PPA2GCWA1},

    {OAL_PTR_NULL,              WLAN_CFG_DTS_NVRAM_PARAMS_BUTT},
};


OAL_STATIC oal_void original_value_for_nvram_params(oal_void)
{
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_0]  = 0x0000F6F6;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_1]  = 0xFBE7F1FB;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_2]  = 0xE7F1F1FB;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_3]  = 0xECF6F6D8;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_4]  = 0xD8D8E2EC;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_5]  = 0x000000E2;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_6]  = 0x0000F1F6;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_7]  = 0xE2ECF600;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_8]  = 0xF1FBFBFB;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_9]  = 0x00F1D3EA;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_10] = 0xE7EC0000;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_11] = 0xC9CED3CE;
    /*  2.4g 5g 20M mcs9 */
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_12] = 0xD8DDCED3;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_13] = 0xC9C9CED3;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_14] = 0x000000C4;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_15] = 0xEC000000;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_16] = 0xC9CECEE7;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_17] = 0x000000C4;
    /* DPD 打开时高阶速率功率 */
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_DPD_0] = 0xE2ECEC00;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_DPD_1] = 0xE2E200E2;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_DPD_2] = 0x0000C4C4;
    /* 11B和OFDM功率差 */
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_62] = 0x00000000;
    /* 5G功率和IQ校准UPC上限值 */
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_63] = 0xD8D83030;
    /* FCC功率认证 */
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_25] = 0xFFFFFFFF;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_26] = 0xFFFFFFFF;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_27] = 0xFFFFFFFF;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_28] = 0xFFFFFFFF;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_29] = 0xFFFFFFFF;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_30] = 0xFFFFFFFF;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_31] = 0xFFFFFFFF;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_32] = 0xFFFFFFFF;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_33] = 0xFFFFFFFF;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_34] = 0xFFFFFFFF;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_35] = 0xFFFFFFFF;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_36] = 0xFFFFFFFF;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_37] = 0xFFFFFFFF;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_38] = 0xFFFFFFFF;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_39] = 0xFFFFFFFF;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_40] = 0xFFFFFFFF;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_41] = 0xFFFFFFFF;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_42] = 0xFFFFFFFF;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_43] = 0xFFFFFFFF;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_44] = 0xFFFFFFFF;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_45] = 0xFFFFFF;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_46] = 0xFFFFFF;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_47] = 0xFFFFFF;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_48] = 0xFFFFFF;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_49] = 0xFFFFFF;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_50] = 0xFFFFFF;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_51] = 0xFFFFFF;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_52] = 0xFFFFFF;
#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH
    g_al_nvram_init_params[NVRAM_PARAMS_TAS_ANT_SWITCH_EN] = 0x0;
    g_al_nvram_init_params[NVRAM_PARAMS_TAS_PWR_CTRL]      = 0x0;
#endif
    g_al_nvram_init_params[NVRAM_PARAMS_5G_CE_HIGH_BAND_MAX_PWR] = 0xFF;
}


OAL_STATIC oal_void original_value_for_dts_params(oal_void)
{
    /* 校准 */
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN1]     = 6250;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN2]     = 5362;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN3]     = 4720;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN4]     = 4480;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN5]     = 4470;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN6]     = 4775;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN7]     = 5200;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN8]     = 5450;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN9]     = 5600;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN10]    = 6100;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN11]    = 6170;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN12]    = 6350;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN13]    = 6530;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND1]     = 2500;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND2]     = 2800;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND3]     = 3100;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND4]     = 3600;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND5]     = 3600;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND6]     = 3700;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND7]     = 3800;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TONE_AMP_GRADE]                   = 1;
    /* DPD校准 */
    g_al_dts_params_etc[WLAN_CFG_DTS_DPD_CALI_CH_CORE0]                     =0x641DA71,
    g_al_dts_params_etc[WLAN_CFG_DTS_DPD_USE_CALI_CH_IDX0_CORE0]            =0x11110000,
    g_al_dts_params_etc[WLAN_CFG_DTS_DPD_USE_CALI_CH_IDX1_CORE0]            =0x33222,
    g_al_dts_params_etc[WLAN_CFG_DTS_DPD_USE_CALI_CH_IDX2_CORE0]            =0x22211000,
    g_al_dts_params_etc[WLAN_CFG_DTS_DPD_USE_CALI_CH_IDX3_CORE0]            =0x2,
    g_al_dts_params_etc[WLAN_CFG_DTS_DPD_CALI_CH_CORE1]                     =0x641DA71,
    g_al_dts_params_etc[WLAN_CFG_DTS_DPD_USE_CALI_CH_IDX0_CORE1]            =0x11110000,
    g_al_dts_params_etc[WLAN_CFG_DTS_DPD_USE_CALI_CH_IDX1_CORE1]            =0x33222,
    g_al_dts_params_etc[WLAN_CFG_DTS_DPD_USE_CALI_CH_IDX2_CORE1]            =0x22211000,
    g_al_dts_params_etc[WLAN_CFG_DTS_DPD_USE_CALI_CH_IDX3_CORE1]            =0x2,
    g_al_dts_params_etc[WLAN_CFG_DTS_DYN_CALI_DSCR_ITERVL]                  = 0x0;
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_HOST)
    /* rf register */
    g_al_dts_params_etc[WLAN_CFG_DTS_RF_REG117]                                 = 0x0505;
    g_al_dts_params_etc[WLAN_CFG_DTS_RF_REG123]                                 = 0x9d01;
    g_al_dts_params_etc[WLAN_CFG_DTS_RF_REG124]                                 = 0x9d01;
    g_al_dts_params_etc[WLAN_CFG_DTS_RF_REG125]                                 = 0x9d01;
    g_al_dts_params_etc[WLAN_CFG_DTS_RF_REG126]                                 = 0x9d01;
    /* bt tmp */
    g_al_dts_params_etc[WLAN_CFG_DTS_BT_CALI_TXPWR_PA_REF_BAND1]            = 11000;
    g_al_dts_params_etc[WLAN_CFG_DTS_BT_CALI_TXPWR_PA_REF_BAND2]            = 10000;
    g_al_dts_params_etc[WLAN_CFG_DTS_BT_CALI_TXPWR_PA_REF_BAND3]            = 7000;
    g_al_dts_params_etc[WLAN_CFG_DTS_BT_CALI_TXPWR_PA_REF_BAND4]            = 8000;
    g_al_dts_params_etc[WLAN_CFG_DTS_BT_CALI_TXPWR_PA_REF_BAND5]            = 7000;
    g_al_dts_params_etc[WLAN_CFG_DTS_BT_CALI_TXPWR_PA_REF_BAND6]            = 7000;
    g_al_dts_params_etc[WLAN_CFG_DTS_BT_CALI_TXPWR_PA_REF_BAND7]            = 12000;
    g_al_dts_params_etc[WLAN_CFG_DTS_BT_CALI_TXPWR_PA_REF_BAND8]            = 12000;
    g_al_dts_params_etc[WLAN_CFG_DTS_BT_CALI_TXPWR_PA_NUM]                  = 7;
    g_al_dts_params_etc[WLAN_CFG_DTS_BT_CALI_TXPWR_PA_FRE1]                 = 0;
    g_al_dts_params_etc[WLAN_CFG_DTS_BT_CALI_TXPWR_PA_FRE2]                 = 10;
    g_al_dts_params_etc[WLAN_CFG_DTS_BT_CALI_TXPWR_PA_FRE3]                 = 28;
    g_al_dts_params_etc[WLAN_CFG_DTS_BT_CALI_TXPWR_PA_FRE4]                 = 45;
    g_al_dts_params_etc[WLAN_CFG_DTS_BT_CALI_TXPWR_PA_FRE5]                 = 53;
    g_al_dts_params_etc[WLAN_CFG_DTS_BT_CALI_TXPWR_PA_FRE6]                 = 63;
    g_al_dts_params_etc[WLAN_CFG_DTS_BT_CALI_TXPWR_PA_FRE7]                 = 76;
    g_al_dts_params_etc[WLAN_CFG_DTS_BT_CALI_TXPWR_PA_FRE8]                 = 78;
    g_al_dts_params_etc[WLAN_CFG_DTS_BT_CALI_TONE_AMP_GRADE]                = 2;
#endif //#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_HOST)
}


OAL_STATIC oal_void host_params_init_first(oal_void)
{
    /* ROAM */
    g_al_host_init_params_etc[WLAN_CFG_INIT_ROAM_SWITCH]                       = 1;
    g_al_host_init_params_etc[WLAN_CFG_INIT_SCAN_ORTHOGONAL]                   = 4;
    g_al_host_init_params_etc[WLAN_CFG_INIT_TRIGGER_B]                         = -70;
    g_al_host_init_params_etc[WLAN_CFG_INIT_TRIGGER_A]                         = -70;
    g_al_host_init_params_etc[WLAN_CFG_INIT_DELTA_B]                           = 10;
    g_al_host_init_params_etc[WLAN_CFG_INIT_DELTA_A]                           = 10;

    /* 性能 */
    g_al_host_init_params_etc[WLAN_CFG_INIT_AMPDU_TX_MAX_NUM]                  = WLAN_AMPDU_TX_MAX_BUF_SIZE;
    g_al_host_init_params_etc[WLAN_CFG_INIT_USED_MEM_FOR_START]                = 45;
    g_al_host_init_params_etc[WLAN_CFG_INIT_USED_MEM_FOR_STOP]                 = 25;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RX_ACK_LIMIT]                      = 10;
    g_al_host_init_params_etc[WLAN_CFG_INIT_SDIO_D2H_ASSEMBLE_COUNT]           = HISDIO_DEV2HOST_SCATT_MAX;
    g_al_host_init_params_etc[WLAN_CFG_INIT_SDIO_H2D_ASSEMBLE_COUNT]           = 8;
    /* LINKLOSS */
    g_al_host_init_params_etc[WLAN_CFG_INIT_LINK_LOSS_THRESHOLD_BT]            = 80;
    g_al_host_init_params_etc[WLAN_CFG_INIT_LINK_LOSS_THRESHOLD_DBAC]          = 80;
    g_al_host_init_params_etc[WLAN_CFG_INIT_LINK_LOSS_THRESHOLD_NORMAL]        = 40;
    /* 自动调频 */
#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
    g_al_host_init_params_etc[WLAN_CFG_INIT_PSS_THRESHOLD_LEVEL_0]             = PPS_VALUE_0;
    g_al_host_init_params_etc[WLAN_CFG_INIT_CPU_FREQ_LIMIT_LEVEL_0]            = CPU_MIN_FREQ_VALUE_0;
    g_al_host_init_params_etc[WLAN_CFG_INIT_DDR_FREQ_LIMIT_LEVEL_0]            = DDR_MIN_FREQ_VALUE_0;
    g_al_host_init_params_etc[WLAN_CFG_INIT_DEVICE_TYPE_LEVEL_0]               = FREQ_IDLE;
    g_al_host_init_params_etc[WLAN_CFG_INIT_PSS_THRESHOLD_LEVEL_1]             = PPS_VALUE_1;
    g_al_host_init_params_etc[WLAN_CFG_INIT_CPU_FREQ_LIMIT_LEVEL_1]            = CPU_MIN_FREQ_VALUE_1;
    g_al_host_init_params_etc[WLAN_CFG_INIT_DDR_FREQ_LIMIT_LEVEL_1]            = DDR_MIN_FREQ_VALUE_1;
    g_al_host_init_params_etc[WLAN_CFG_INIT_DEVICE_TYPE_LEVEL_1]               = FREQ_MIDIUM;
    g_al_host_init_params_etc[WLAN_CFG_INIT_PSS_THRESHOLD_LEVEL_2]             = PPS_VALUE_2;
    g_al_host_init_params_etc[WLAN_CFG_INIT_CPU_FREQ_LIMIT_LEVEL_2]            = CPU_MIN_FREQ_VALUE_2;
    g_al_host_init_params_etc[WLAN_CFG_INIT_DDR_FREQ_LIMIT_LEVEL_2]            = DDR_MIN_FREQ_VALUE_2;
    g_al_host_init_params_etc[WLAN_CFG_INIT_DEVICE_TYPE_LEVEL_2]               = FREQ_HIGHER;
    g_al_host_init_params_etc[WLAN_CFG_INIT_PSS_THRESHOLD_LEVEL_3]             = PPS_VALUE_3;
    g_al_host_init_params_etc[WLAN_CFG_INIT_CPU_FREQ_LIMIT_LEVEL_3]            = CPU_MIN_FREQ_VALUE_3;
    g_al_host_init_params_etc[WLAN_CFG_INIT_DDR_FREQ_LIMIT_LEVEL_3]            = DDR_MIN_FREQ_VALUE_3;
    g_al_host_init_params_etc[WLAN_CFG_INIT_DEVICE_TYPE_LEVEL_3]               = FREQ_HIGHEST;
#endif
    /* 动态绑PCIE中断 */
    g_al_host_init_params_etc[WLAN_CFG_INIT_IRQ_AFFINITY]                      = OAL_FALSE;
    g_al_host_init_params_etc[WLAN_CFG_INIT_IRQ_TH_HIGH]                       = 250;
    g_al_host_init_params_etc[WLAN_CFG_INIT_IRQ_TH_LOW]                        = 150;
    g_al_host_init_params_etc[WLAN_CFG_INIT_IRQ_PPS_TH_HIGH]                   = 25000;
    g_al_host_init_params_etc[WLAN_CFG_INIT_IRQ_PPS_TH_LOW]                    = 5000;
#ifdef _PRE_WLAN_FEATURE_AMPDU_TX_HW
    /* 硬件聚合定制化项 */
    g_al_host_init_params_etc[WLAN_CFG_INIT_HW_AMPDU]                          = OAL_FALSE;
    g_al_host_init_params_etc[WLAN_CFG_INIT_HW_AMPDU_TH_HIGH]                  = 300;
    g_al_host_init_params_etc[WLAN_CFG_INIT_HW_AMPDU_TH_LOW]                   = 200;
#endif
#ifdef _PRE_WLAN_FEATURE_MULTI_NETBUF_AMSDU
    g_al_host_init_params_etc[WLAN_CFG_INIT_AMPDU_AMSDU_SKB]                   = OAL_FALSE;
    g_al_host_init_params_etc[WLAN_CFG_INIT_AMSDU_AMPDU_TH_HIGH]               = 300;
    g_al_host_init_params_etc[WLAN_CFG_INIT_AMSDU_AMPDU_TH_LOW]                = 200;
#endif
#ifdef _PRE_WLAN_TCP_OPT
    g_al_host_init_params_etc[WLAN_CFG_INIT_TCP_ACK_FILTER]                    = OAL_FALSE;
    g_al_host_init_params_etc[WLAN_CFG_INIT_TCP_ACK_FILTER_TH_HIGH]            = 60;
    g_al_host_init_params_etc[WLAN_CFG_INIT_TCP_ACK_FILTER_TH_LOW]             = 20;
#endif


    g_al_host_init_params_etc[WLAN_CFG_INIT_TX_SMALL_AMSDU]                    = OAL_TRUE;
    g_al_host_init_params_etc[WLAN_CFG_INIT_SMALL_AMSDU_HIGH]                  = 300;
    g_al_host_init_params_etc[WLAN_CFG_INIT_SMALL_AMSDU_LOW]                   = 200;
    g_al_host_init_params_etc[WLAN_CFG_INIT_SMALL_AMSDU_PPS_HIGH]              = 25000;
    g_al_host_init_params_etc[WLAN_CFG_INIT_SMALL_AMSDU_PPS_LOW]               = 5000;

    g_al_host_init_params_etc[WLAN_CFG_INIT_TX_TCP_ACK_BUF]                    = OAL_TRUE;
    g_al_host_init_params_etc[WLAN_CFG_INIT_TCP_ACK_BUF_HIGH]                  = 90;
    g_al_host_init_params_etc[WLAN_CFG_INIT_TCP_ACK_BUF_LOW]                   = 50;
    g_al_host_init_params_etc[WLAN_CFG_INIT_TCP_ACK_BUF_HIGH_40M]               = 300;
    g_al_host_init_params_etc[WLAN_CFG_INIT_TCP_ACK_BUF_LOW_40M]                = 150;
    g_al_host_init_params_etc[WLAN_CFG_INIT_TCP_ACK_BUF_HIGH_80M]               = 550;
    g_al_host_init_params_etc[WLAN_CFG_INIT_TCP_ACK_BUF_LOW_80M]                = 450;
    g_al_host_init_params_etc[WLAN_CFG_INIT_TCP_ACK_BUF_HIGH_160M]              = 800;
    g_al_host_init_params_etc[WLAN_CFG_INIT_TCP_ACK_BUF_LOW_160M]               = 700;

    g_al_host_init_params_etc[WLAN_CFG_INIT_RX_DYN_BYPASS_EXTLNA]                  = OAL_FALSE;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RX_DYN_BYPASS_EXTLNA_HIGH]             = 100;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RX_DYN_BYPASS_EXTLNA_LOW]              = 50;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RX_AMPDU_AMSDU_SKB]                = OAL_FALSE;

    /* 低功耗 */
    g_al_host_init_params_etc[WLAN_CFG_INIT_POWERMGMT_SWITCH]                  = OAL_TRUE;
    g_al_host_init_params_etc[WLAN_CFG_INIT_PS_MODE]                           = 1;
    g_al_host_init_params_etc[WLAN_CFG_INIT_FAST_CHECK_CNT]                    = 1;

    /* 可维可测 */
    /* 日志级别 */
    g_al_host_init_params_etc[WLAN_CFG_INIT_LOGLEVEL]                          = OAM_LOG_LEVEL_WARNING;

    /* 2G RF前端 */
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST)
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_2G_BAND1]    = 0xF4F4;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_2G_BAND2]    = 0xF4F4;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_2G_BAND3]    = 0xF4F4;
    /* 5G RF前端 */
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND1]    = 0xF8F8;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND2]    = 0xF8F8;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND3]    = 0xF8F8;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND4]    = 0xF8F8;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND5]    = 0xF8F8;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND6]    = 0xF8F8;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND7]    = 0xF8F8;

    /* fem */
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_LNA_BYPASS_GAIN_DB_2G]            = 0xFFF4FFF4;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_LNA_GAIN_DB_2G]                   = 0x00140014;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_PA_GAIN_DB_B0_2G]                 = 0xFFF4FFF4;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_PA_GAIN_DB_B1_2G]                 = 0xFFF4FFF4;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_PA_GAIN_LVL_2G]                   = 0x00010001;
    g_al_host_init_params_etc[WLAN_CFG_INIT_EXT_SWITCH_ISEXIST_2G]               = 0x00010001;
    g_al_host_init_params_etc[WLAN_CFG_INIT_EXT_PA_ISEXIST_2G]                   = 0x00010001;
    g_al_host_init_params_etc[WLAN_CFG_INIT_EXT_LNA_ISEXIST_2G]                  = 0x00010001;
    g_al_host_init_params_etc[WLAN_CFG_INIT_LNA_ON2OFF_TIME_NS_2G]               = 0x02760276;
    g_al_host_init_params_etc[WLAN_CFG_INIT_LNA_OFF2ON_TIME_NS_2G]               = 0x01400140;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_LNA_BYPASS_GAIN_DB_5G]            = 0xFFF4FFF4;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_LNA_GAIN_DB_5G]                   = 0x00140014;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_PA_GAIN_DB_B0_5G]                 = 0xFFF4FFF4;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_PA_GAIN_DB_B1_5G]                 = 0xFFF4FFF4;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_PA_GAIN_LVL_5G]                   = 0x00010001;
    g_al_host_init_params_etc[WLAN_CFG_INIT_EXT_SWITCH_ISEXIST_5G]               = 0x00010001;
    g_al_host_init_params_etc[WLAN_CFG_INIT_EXT_PA_ISEXIST_5G]                   = 0x00010001;
    g_al_host_init_params_etc[WLAN_CFG_INIT_EXT_LNA_ISEXIST_5G]                  = 0x00010001;
    g_al_host_init_params_etc[WLAN_CFG_INIT_LNA_ON2OFF_TIME_NS_5G]               = 0x02760276;
    g_al_host_init_params_etc[WLAN_CFG_INIT_LNA_OFF2ON_TIME_NS_5G]               = 0x01400140;

    /* 用于定制化计算PWR RF值的偏差 */
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_PWR_REF_RSSI_2G_C0_MULT4]    = 0;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_PWR_REF_RSSI_2G_C1_MULT4]    = 0;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_PWR_REF_RSSI_5G_C0_MULT4]    = 0;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_PWR_REF_RSSI_5G_C1_MULT4]    = 0;
#else
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_2G_BAND1]    = -12;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_2G_BAND2]    = -12;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_2G_BAND3]    = -12;
    /* 5G RF前端 */
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND1]    = -8;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND2]    = -8;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND3]    = -8;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND4]    = -8;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND5]    = -8;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND6]    = -8;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND7]    = -8;

    /* fem */
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_LNA_BYPASS_GAIN_DB_2G]            = -12;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_LNA_GAIN_DB_2G]                   = 20;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_PA_GAIN_DB_B0_2G]                 = -12;
    g_al_host_init_params_etc[WLAN_CFG_INIT_EXT_SWITCH_ISEXIST_2G]               = 0;
    g_al_host_init_params_etc[WLAN_CFG_INIT_EXT_PA_ISEXIST_2G]                   = 0;
    g_al_host_init_params_etc[WLAN_CFG_INIT_EXT_LNA_ISEXIST_2G]                  = 0;
    g_al_host_init_params_etc[WLAN_CFG_INIT_LNA_ON2OFF_TIME_NS_2G]               = 630;
    g_al_host_init_params_etc[WLAN_CFG_INIT_LNA_OFF2ON_TIME_NS_2G]               = 320;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_LNA_BYPASS_GAIN_DB_5G]            = -12;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_LNA_GAIN_DB_5G]                   = 20;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_PA_GAIN_DB_B0_5G]                 = -12;
    g_al_host_init_params_etc[WLAN_CFG_INIT_EXT_SWITCH_ISEXIST_5G]               = 1;
    g_al_host_init_params_etc[WLAN_CFG_INIT_EXT_PA_ISEXIST_5G]                   = 1;
    g_al_host_init_params_etc[WLAN_CFG_INIT_EXT_LNA_ISEXIST_5G]                  = 1;
    g_al_host_init_params_etc[WLAN_CFG_INIT_LNA_ON2OFF_TIME_NS_5G]               = 630;
    g_al_host_init_params_etc[WLAN_CFG_INIT_LNA_OFF2ON_TIME_NS_5G]               = 320;
#endif //#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST)

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_HOST)
    /* 温度上升导致发射功率下降过多的功率补偿 */
    g_al_host_init_params_etc[WLAN_CFG_INIT_TX_RATIO_LEVEL_0]                   = 900;
    g_al_host_init_params_etc[WLAN_CFG_INIT_TX_PWR_COMP_VAL_LEVEL_0]            = 17;
    g_al_host_init_params_etc[WLAN_CFG_INIT_TX_RATIO_LEVEL_1]                   = 650;
    g_al_host_init_params_etc[WLAN_CFG_INIT_TX_PWR_COMP_VAL_LEVEL_1]            = 13;
    g_al_host_init_params_etc[WLAN_CFG_INIT_TX_RATIO_LEVEL_2]                   = 280;
    g_al_host_init_params_etc[WLAN_CFG_INIT_TX_PWR_COMP_VAL_LEVEL_2]            = 5;
    g_al_host_init_params_etc[WLAN_CFG_INIT_MORE_PWR]                           = 7;
#endif //#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_HOST)
    /* SCAN */
    g_al_host_init_params_etc[WLAN_CFG_INIT_RANDOM_MAC_ADDR_SCAN]               = 1;
    /* 11AC2G */
    g_al_host_init_params_etc[WLAN_CFG_INIT_11AC2G_ENABLE]                      = 1;
    g_al_host_init_params_etc[WLAN_CFG_INIT_DISABLE_CAPAB_2GHT40]               = 0;
    g_al_host_init_params_etc[WLAN_CFG_INIT_DUAL_ANTENNA_ENABLE]                 = 0;
    /* sta keepalive cnt th*/
    g_al_host_init_params_etc[WLAN_CFG_INIT_STA_KEEPALIVE_CNT_TH]               = 3;
    g_al_host_init_params_etc[WLAN_CFG_INIT_FAR_DIST_POW_GAIN_SWITCH]           = 1;
    g_al_host_init_params_etc[WLAN_CFG_INIT_FAR_DIST_DSSS_SCALE_PROMOTE_SWITCH] = 1;
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST)
    g_al_host_init_params_etc[WLAN_CFG_INIT_CHANN_RADIO_CAP]                   = 0xF;

    g_al_host_init_params_etc[WLAN_CFG_LTE_GPIO_CHECK_SWITCH]                  = 0;
    g_al_host_init_params_etc[WLAN_ATCMDSRV_ISM_PRIORITY]                      = 0;
    g_al_host_init_params_etc[WLAN_ATCMDSRV_LTE_RX]                            = 0;
    g_al_host_init_params_etc[WLAN_ATCMDSRV_LTE_TX]                            = 0;
    g_al_host_init_params_etc[WLAN_ATCMDSRV_LTE_INACT]                         = 0;
    g_al_host_init_params_etc[WLAN_ATCMDSRV_ISM_RX_ACT]                        = 0;
    g_al_host_init_params_etc[WLAN_ATCMDSRV_BANT_PRI]                          = 0;
    g_al_host_init_params_etc[WLAN_ATCMDSRV_BANT_STATUS]                       = 0;
    g_al_host_init_params_etc[WLAN_ATCMDSRV_WANT_PRI]                          = 0;
    g_al_host_init_params_etc[WLAN_ATCMDSRV_WANT_STATUS]                       = 0;
    g_al_host_init_params_etc[WLAN_TX5G_UPC_MIX_GAIN_CTRL_1]                   = 0;
    g_al_host_init_params_etc[WLAN_TX5G_UPC_MIX_GAIN_CTRL_2]                   = 0;
    g_al_host_init_params_etc[WLAN_TX5G_UPC_MIX_GAIN_CTRL_3]                   = 0;
    g_al_host_init_params_etc[WLAN_TX5G_UPC_MIX_GAIN_CTRL_4]                   = 0;
    g_al_host_init_params_etc[WLAN_TX5G_UPC_MIX_GAIN_CTRL_5]                   = 0;
    g_al_host_init_params_etc[WLAN_TX5G_UPC_MIX_GAIN_CTRL_6]                   = 0;
    g_al_host_init_params_etc[WLAN_TX5G_UPC_MIX_GAIN_CTRL_7]                   = 0;
    /* PA bias */
    g_al_host_init_params_etc[WLAN_TX2G_PA_GATE_VCTL_REG236]                   =  0x12081208;
    g_al_host_init_params_etc[WLAN_TX2G_PA_GATE_VCTL_REG237]                   =  0x2424292D;
    g_al_host_init_params_etc[WLAN_TX2G_PA_GATE_VCTL_REG238]                   =  0x24242023;
    g_al_host_init_params_etc[WLAN_TX2G_PA_GATE_VCTL_REG239]                   =  0x24242020;
    g_al_host_init_params_etc[WLAN_TX2G_PA_GATE_VCTL_REG240]                   =  0x24242020;
    g_al_host_init_params_etc[WLAN_TX2G_PA_GATE_VCTL_REG241]                   =  0x24241B1B;
    g_al_host_init_params_etc[WLAN_TX2G_PA_GATE_VCTL_REG242]                   =  0x24241B1B;
    g_al_host_init_params_etc[WLAN_TX2G_PA_GATE_VCTL_REG243]                   =  0x24241B1B;
    g_al_host_init_params_etc[WLAN_TX2G_PA_GATE_VCTL_REG244]                   =  0x24241B1B;

    g_al_host_init_params_etc[WLAN_TX2G_PA_VRECT_GATE_THIN_REG253] = 0x14141414;
    g_al_host_init_params_etc[WLAN_TX2G_PA_VRECT_GATE_THIN_REG254] = 0x13131313;
    g_al_host_init_params_etc[WLAN_TX2G_PA_VRECT_GATE_THIN_REG255] = 0x12121212;
    g_al_host_init_params_etc[WLAN_TX2G_PA_VRECT_GATE_THIN_REG256] = 0x12121212;
    g_al_host_init_params_etc[WLAN_TX2G_PA_VRECT_GATE_THIN_REG257] = 0x12121212;
    g_al_host_init_params_etc[WLAN_TX2G_PA_VRECT_GATE_THIN_REG258] = 0x12121212;
    g_al_host_init_params_etc[WLAN_TX2G_PA_VRECT_GATE_THIN_REG259] = 0x12121212;
    g_al_host_init_params_etc[WLAN_TX2G_PA_VRECT_GATE_THIN_REG260] = 0x12121212;
    g_al_host_init_params_etc[WLAN_TX2G_PA_VRECT_GATE_THIN_REG261] = 0x0F0F0F0F;
    g_al_host_init_params_etc[WLAN_TX2G_PA_VRECT_GATE_THIN_REG262] = 0x0D0D0D0D;
    g_al_host_init_params_etc[WLAN_TX2G_PA_VRECT_GATE_THIN_REG263] = 0x0A0B0A0B;
    g_al_host_init_params_etc[WLAN_TX2G_PA_VRECT_GATE_THIN_REG264] = 0x0A0A0A0A;
    g_al_host_init_params_etc[WLAN_TX2G_PA_VRECT_GATE_THIN_REG265] = 0x0A0A0A0A;
    g_al_host_init_params_etc[WLAN_TX2G_PA_VRECT_GATE_THIN_REG266] = 0x0A0A0A0A;
    g_al_host_init_params_etc[WLAN_TX2G_PA_VRECT_GATE_THIN_REG267] = 0x0A0A0A0A;
    g_al_host_init_params_etc[WLAN_TX2G_PA_VRECT_GATE_THIN_REG268] = 0x0A0A0A0A;
    g_al_host_init_params_etc[WLAN_TX2G_PA_VRECT_GATE_THIN_REG269] = 0x0F0F0F0F;
    g_al_host_init_params_etc[WLAN_TX2G_PA_VRECT_GATE_THIN_REG270] = 0x0D0D0D0D;
    g_al_host_init_params_etc[WLAN_TX2G_PA_VRECT_GATE_THIN_REG271] = 0x0A0B0A0B;
    g_al_host_init_params_etc[WLAN_TX2G_PA_VRECT_GATE_THIN_REG272] = 0x0A0A0A0A;
    g_al_host_init_params_etc[WLAN_TX2G_PA_VRECT_GATE_THIN_REG273] = 0x0A0A0A0A;
    g_al_host_init_params_etc[WLAN_TX2G_PA_VRECT_GATE_THIN_REG274] = 0x0A0A0A0A;
    g_al_host_init_params_etc[WLAN_TX2G_PA_VRECT_GATE_THIN_REG275] = 0x0A0A0A0A;
    g_al_host_init_params_etc[WLAN_TX2G_PA_VRECT_GATE_THIN_REG276] = 0x0A0A0A0A;
    g_al_host_init_params_etc[WLAN_TX2G_PA_VRECT_GATE_THIN_REG277]             =  0x0D0D0D0D;
    g_al_host_init_params_etc[WLAN_TX2G_PA_VRECT_GATE_THIN_REG278]             =  0x0D0D0D0D;
    g_al_host_init_params_etc[WLAN_TX2G_PA_VRECT_GATE_THIN_REG279]             =  0x0D0D0A0B;
    g_al_host_init_params_etc[WLAN_TX2G_PA_VRECT_GATE_THIN_REG280_BAND1]       =  0x0D0D0A0A;
    g_al_host_init_params_etc[WLAN_TX2G_PA_VRECT_GATE_THIN_REG280_BAND2]       =  0x0D0D0A0A;
    g_al_host_init_params_etc[WLAN_TX2G_PA_VRECT_GATE_THIN_REG280_BAND3]       =  0x0D0D0A0A;
    g_al_host_init_params_etc[WLAN_TX2G_PA_VRECT_GATE_THIN_REG281]             =  0x0D0D0A0A;
    g_al_host_init_params_etc[WLAN_TX2G_PA_VRECT_GATE_THIN_REG282]             =  0x0D0D0A0A;
    g_al_host_init_params_etc[WLAN_TX2G_PA_VRECT_GATE_THIN_REG283]             =  0x0D0D0A0A;
    g_al_host_init_params_etc[WLAN_TX2G_PA_VRECT_GATE_THIN_REG284]             =  0x0D0D0A0A;
#endif
    g_al_host_init_params_etc[WLAN_CFG_INIT_DELTA_CCA_ED_HIGH_20TH_2G]         = 0;
    g_al_host_init_params_etc[WLAN_CFG_INIT_DELTA_CCA_ED_HIGH_40TH_2G]         = 0;
    g_al_host_init_params_etc[WLAN_CFG_INIT_DELTA_CCA_ED_HIGH_20TH_5G]         = 0;
    g_al_host_init_params_etc[WLAN_CFG_INIT_DELTA_CCA_ED_HIGH_40TH_5G]         = 0;

    /* ldac m2s rssi */
    g_al_host_init_params_etc[WLAN_CFG_INIT_LDAC_THRESHOLD_M2S]            = -45;  /* 默认最大门限，不支持 */
    g_al_host_init_params_etc[WLAN_CFG_INIT_LDAC_THRESHOLD_S2M]            = -75;

}


regdomain_enum_uint8 hwifi_get_regdomain_from_country_code(const countrycode_t country_code)
{
    regdomain_enum_uint8  en_regdomain = REGDOMAIN_COMMON;
    int32                 table_idx = 0;

    while (g_ast_country_info_table[table_idx].en_regdomain != REGDOMAIN_COUNT)
    {
        if (0 == oal_memcmp(country_code, g_ast_country_info_table[table_idx].auc_country_code, COUNTRY_CODE_LEN))
        {
            en_regdomain = g_ast_country_info_table[table_idx].en_regdomain;
            break;
        }
        ++table_idx;
    }

    return en_regdomain;
}


int32 hwifi_is_regdomain_changed_etc(const countrycode_t country_code_old, const countrycode_t country_code_new)
{
    return hwifi_get_regdomain_from_country_code(country_code_old) != hwifi_get_regdomain_from_country_code(country_code_new);
}

#if 0

OAL_STATIC int32 hwifi_get_plat_tag_from_country_code(const countrycode_t country_code)
{
    regdomain_enum_uint8  en_regdomain;
    int32           table_idx = 0;

    en_regdomain = hwifi_get_regdomain_from_country_code(country_code);

    while(g_ast_plat_tag_mapping_table[table_idx].en_regdomain != REGDOMAIN_COUNT
        && g_ast_plat_tag_mapping_table[table_idx].plat_tag != INI_MODU_INVALID)
    {
        /* matched */
        if (en_regdomain == g_ast_plat_tag_mapping_table[table_idx].en_regdomain)
        {
            return g_ast_plat_tag_mapping_table[table_idx].plat_tag;
        }

        ++table_idx;
    }

    /* not found, use common regdomain */
    return INI_MODU_WIFI;
}
#endif


int32 hwifi_fetch_ori_caldata_etc(uint8* auc_caldata, int32 l_nvm_len)
{
    int32 l_ret = INI_FAILED;
    int32 l_cfg_id;
    int32 aul_nvram_params[NVRAM_PARAMS_BASE_INDEX_BUTT]={0};

    if (l_nvm_len != HISI_CUST_NVRAM_LEN)
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "hwifi_fetch_ori_caldata_etc atcmd[nv_len:%d] and plat_ini[nv_len:%d] model have different nvm lenth!!",
                        l_nvm_len, HISI_CUST_NVRAM_LEN);
        return INI_FAILED;
    }

    oal_memset(auc_caldata, 0x00, HISI_CUST_NVRAM_LEN);

    for (l_cfg_id = NVRAM_PARAMS_INDEX_0; l_cfg_id < NVRAM_PARAMS_BASE_INDEX_BUTT; l_cfg_id++)
    {
        l_ret = get_cust_conf_int32_etc(INI_MODU_WIFI, g_ast_nvram_config_ini[l_cfg_id].name, &aul_nvram_params[l_cfg_id]);
        if(INI_FAILED == l_ret)
        {
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "hwifi_fetch_ori_caldata_etc read ori caldata %d from ini failed!", l_cfg_id);
            return INI_FAILED;
        }
    }

    OAM_INFO_LOG0(0, OAM_SF_ANY, "hwifi_fetch_ori_caldata_etc read ori caldata from ini success!");
    oal_memcopy(auc_caldata, aul_nvram_params, HISI_CUST_NVRAM_LEN);

    return INI_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ

int32 hwifi_custom_adapt_device_ini_freq_param(oal_uint8 *puc_data, oal_uint32 *pul_data_len)
{
    config_device_freq_h2d_stru            st_device_freq_data;
    oal_uint8                              uc_index;
    oal_int32                              l_val;
    oal_uint32                             cfg_id;
    hmac_to_dmac_cfg_custom_data_stru      st_syn_msg;

    if (NULL == puc_data)
    {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_ini_freq_param puc_data is NULL last data_len[%d].}", *pul_data_len);
        return INI_FAILED;
    }

    st_syn_msg.en_syn_id = CUSTOM_CFGID_INI_FREQ_ID;

    for(uc_index = 0, cfg_id = WLAN_CFG_INIT_PSS_THRESHOLD_LEVEL_0; uc_index < 4; uc_index++)
    {
        l_val = hwifi_get_init_value_etc(CUS_TAG_INI, cfg_id);
        if (PPS_VALUE_0 <= l_val && PPS_VALUE_3 >= l_val)
        {
            st_device_freq_data.st_device_data[uc_index].ul_speed_level = (oal_uint32)l_val;
            cfg_id += 3;
        }
        else
        {
            OAM_ERROR_LOG1(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_ini_freq_param get wrong PSS_THRESHOLD_LEVEL[%d]!}", l_val);
            return OAL_FALSE;
        }
    }

    for(uc_index = 0, cfg_id = WLAN_CFG_INIT_DEVICE_TYPE_LEVEL_0; uc_index < 4; uc_index++)
    {
        l_val = hwifi_get_init_value_etc(CUS_TAG_INI, cfg_id);

        if(FREQ_IDLE <= l_val && FREQ_HIGHEST >= l_val)
        {
            st_device_freq_data.st_device_data[uc_index].ul_cpu_freq_level = (oal_uint32)l_val;
            cfg_id++;
        }
        else
        {
            OAM_ERROR_LOG1(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_ini_freq_param get wrong DEVICE_TYPE_LEVEL [%d]!}", l_val);
            return OAL_FALSE;
        }

    }
    st_device_freq_data.uc_set_type = FREQ_SYNC_DATA;

    st_syn_msg.ul_len = OAL_SIZEOF(st_device_freq_data);

    oal_memcopy(puc_data, &st_syn_msg, CUSTOM_MSG_DATA_HDR_LEN);
    oal_memcopy(puc_data + CUSTOM_MSG_DATA_HDR_LEN, &st_device_freq_data, OAL_SIZEOF(st_device_freq_data));

    *pul_data_len += (st_syn_msg.ul_len + CUSTOM_MSG_DATA_HDR_LEN);
    OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_ini_freq_param da_len[%d].}", *pul_data_len);

    return OAL_SUCC;
}
#endif // #ifdef _PRE_WLAN_FEATURE_AUTO_FREQ


extern oal_uint32 hcc_assemble_count_etc;
OAL_STATIC oal_void hwifi_custom_adapt_device_ini_perf_param(oal_uint8 *puc_data, oal_uint32 *pul_data_len)
{
    oal_int8                               ac_tmp[8]           = {0};
    oal_uint8                              uc_sdio_assem_h2d;
    oal_uint8                              uc_sdio_assem_d2h;
    config_device_perf_h2d_stru            st_device_perf;
    hmac_to_dmac_cfg_custom_data_stru      st_syn_msg;

    if (NULL == puc_data)
    {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_ini_perf_param puc_data is NULL last data_len[%d].}", *pul_data_len);
        return;
    }

    OAL_MEMZERO(&st_device_perf, OAL_SIZEOF(st_device_perf));

    st_syn_msg.en_syn_id = CUSTOM_CFGID_INI_PERF_ID;

    /* SDIO FLOWCTRL */
    //device侧做合法性判断
    oal_itoa(hwifi_get_init_value_etc(CUS_TAG_INI, WLAN_CFG_INIT_USED_MEM_FOR_START), st_device_perf.ac_used_mem_param, 5);
    oal_itoa(hwifi_get_init_value_etc(CUS_TAG_INI, WLAN_CFG_INIT_USED_MEM_FOR_STOP),ac_tmp, 5);
    st_device_perf.ac_used_mem_param[OAL_STRLEN(st_device_perf.ac_used_mem_param)] = ' ';
    oal_memcopy(st_device_perf.ac_used_mem_param + OAL_STRLEN(st_device_perf.ac_used_mem_param), ac_tmp, OAL_STRLEN(ac_tmp));

    st_device_perf.ac_used_mem_param[OAL_STRLEN(st_device_perf.ac_used_mem_param)] = '\0';


    /* SDIO ASSEMBLE COUNT:H2D */
    uc_sdio_assem_h2d = (oal_uint8)hwifi_get_init_value_etc(CUS_TAG_INI, WLAN_CFG_INIT_SDIO_H2D_ASSEMBLE_COUNT);
    //判断值的合法性
    if (uc_sdio_assem_h2d >= 1 && uc_sdio_assem_h2d <= HISDIO_HOST2DEV_SCATT_MAX)
    {
        hcc_assemble_count_etc = uc_sdio_assem_h2d;
    }
    else
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{hwifi_custom_adapt_device_ini_perf_param::sdio_assem_h2d[%d] out of range(0,%d], check value in ini file!}\r\n",
                            uc_sdio_assem_h2d, HISDIO_HOST2DEV_SCATT_MAX);
    }

    /* SDIO ASSEMBLE COUNT:D2H */
    uc_sdio_assem_d2h = (oal_uint8)hwifi_get_init_value_etc(CUS_TAG_INI, WLAN_CFG_INIT_SDIO_D2H_ASSEMBLE_COUNT);
    //判断值的合法性
    if(uc_sdio_assem_d2h >= 1 && uc_sdio_assem_d2h <= HISDIO_DEV2HOST_SCATT_MAX)
    {
        st_device_perf.uc_sdio_assem_d2h = uc_sdio_assem_d2h;
    }
    else
    {
        st_device_perf.uc_sdio_assem_d2h = HISDIO_DEV2HOST_SCATT_MAX;
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{hwifi_custom_adapt_device_ini_perf_param::sdio_assem_d2h[%d] out of range(0,%d], check value in ini file!}\r\n",
                            uc_sdio_assem_d2h, HISDIO_DEV2HOST_SCATT_MAX);
    }

    st_syn_msg.ul_len = OAL_SIZEOF(st_device_perf);

    oal_memcopy(puc_data, &st_syn_msg, CUSTOM_MSG_DATA_HDR_LEN);
    oal_memcopy(puc_data + CUSTOM_MSG_DATA_HDR_LEN, &st_device_perf, OAL_SIZEOF(st_device_perf));

    *pul_data_len += (OAL_SIZEOF(st_device_perf) + CUSTOM_MSG_DATA_HDR_LEN);

    OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_ini_perf_param::da_len[%d].}", *pul_data_len);
}


OAL_STATIC oal_void hwifi_custom_adapt_device_ini_end_param (oal_uint8 *puc_data, oal_uint32 *pul_data_len)
{
    hmac_to_dmac_cfg_custom_data_stru     st_syn_msg;

    if (NULL == puc_data)
    {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_ini_end_param puc_data::NULL data_len[%d].}", *pul_data_len);
        return;
    }

    st_syn_msg.en_syn_id = CUSTOM_CFGID_INI_ENDING_ID;
    st_syn_msg.ul_len    = 0;

    oal_memcopy(puc_data, &st_syn_msg, OAL_SIZEOF(st_syn_msg));

    *pul_data_len += OAL_SIZEOF(st_syn_msg);

    OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_ini_end_param::da_len[%d].}", *pul_data_len);
}


OAL_STATIC oal_void hwifi_custom_adapt_device_ini_linkloss_param (oal_uint8 *puc_data, oal_uint32 *pul_data_len)
{
    oal_uint8 ast_threshold[WLAN_LINKLOSS_MODE_BUTT] = {0};

    hmac_to_dmac_cfg_custom_data_stru      st_syn_msg;

    if (NULL == puc_data)
    {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_ini_linkloss_param::puc_data is NULL data_len[%d].}", *pul_data_len);
        return;
    }

    st_syn_msg.en_syn_id = CUSTOM_CFGID_INI_LINKLOSS_ID;

    ast_threshold[WLAN_LINKLOSS_MODE_BT] = (oal_uint8)hwifi_get_init_value_etc(CUS_TAG_INI, WLAN_CFG_INIT_LINK_LOSS_THRESHOLD_BT);
    ast_threshold[WLAN_LINKLOSS_MODE_DBAC] = (oal_uint8)hwifi_get_init_value_etc(CUS_TAG_INI, WLAN_CFG_INIT_LINK_LOSS_THRESHOLD_DBAC);
    ast_threshold[WLAN_LINKLOSS_MODE_NORMAL] = (oal_uint8)hwifi_get_init_value_etc(CUS_TAG_INI, WLAN_CFG_INIT_LINK_LOSS_THRESHOLD_NORMAL);

    st_syn_msg.ul_len = OAL_SIZEOF(ast_threshold);

    oal_memcopy(puc_data, &st_syn_msg, CUSTOM_MSG_DATA_HDR_LEN);
    oal_memcopy(puc_data + CUSTOM_MSG_DATA_HDR_LEN, &ast_threshold, OAL_SIZEOF(ast_threshold));

    *pul_data_len += (OAL_SIZEOF(ast_threshold) + CUSTOM_MSG_DATA_HDR_LEN);

    OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_ini_linkloss_param::da_len[%d].}", *pul_data_len);
}


OAL_STATIC oal_void hwifi_custom_adapt_device_ini_ldac_m2s_rssi_param (oal_uint8 *puc_data, oal_uint32 *pul_data_len)
{
    oal_int8 ast_ldac_m2s_rssi_threshold[WLAN_M2S_LDAC_RSSI_BUTT] = {0,0}; /* 当前m2s和s2m门限，后续扩展再添加枚举 */

    hmac_to_dmac_cfg_custom_data_stru      st_syn_msg;

    if (NULL == puc_data)
    {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_ini_ldac_m2s_rssi_param::puc_data is NULL data_len[%d].}", *pul_data_len);
        return;
    }

    st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_INI_LDAC_M2S_TH_ID;

    ast_ldac_m2s_rssi_threshold[WLAN_M2S_LDAC_RSSI_TO_SISO] = (oal_int8)hwifi_get_init_value_etc(CUS_TAG_INI, WLAN_CFG_INIT_LDAC_THRESHOLD_M2S);
    ast_ldac_m2s_rssi_threshold[WLAN_M2S_LDAC_RSSI_TO_MIMO] = (oal_int8)hwifi_get_init_value_etc(CUS_TAG_INI, WLAN_CFG_INIT_LDAC_THRESHOLD_S2M);

    st_syn_msg.ul_len = OAL_SIZEOF(ast_ldac_m2s_rssi_threshold);

    oal_memcopy(puc_data, &st_syn_msg, CUSTOM_MSG_DATA_HDR_LEN);
    oal_memcopy(puc_data + CUSTOM_MSG_DATA_HDR_LEN, &ast_ldac_m2s_rssi_threshold, OAL_SIZEOF(ast_ldac_m2s_rssi_threshold));

    *pul_data_len += (OAL_SIZEOF(ast_ldac_m2s_rssi_threshold) + CUSTOM_MSG_DATA_HDR_LEN);

    OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_ini_ldac_m2s_rssi_param::da_len[%d].}", *pul_data_len);
}


extern  oal_uint8 g_wlan_device_pm_switch;
extern oal_bool_enum g_wlan_pm_switch_etc;
OAL_STATIC oal_void hwifi_custom_adapt_device_ini_pm_switch_param(oal_uint8 *puc_data, oal_uint32 *pul_data_len)
{
    hmac_to_dmac_cfg_custom_data_stru     st_syn_msg;

    if (NULL == puc_data)
    {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_ini_pm_switch_param::puc_data is NULL data_len[%d].}", *pul_data_len);
        return;
    }

    st_syn_msg.en_syn_id = CUSTOM_CFGID_INI_PM_SWITCH_ID;

    g_wlan_device_pm_switch = hwifi_get_init_value_etc(CUS_TAG_INI, WLAN_CFG_INIT_POWERMGMT_SWITCH);
    g_wlan_pm_switch_etc = (g_wlan_device_pm_switch == 1 || g_wlan_device_pm_switch == 4)  ?  OAL_TRUE : OAL_FALSE;

    st_syn_msg.auc_msg_body[0] = g_wlan_device_pm_switch;
    st_syn_msg.ul_len = OAL_SIZEOF(st_syn_msg) - CUSTOM_MSG_DATA_HDR_LEN;

    oal_memcopy(puc_data, &st_syn_msg, OAL_SIZEOF(st_syn_msg));

    *pul_data_len += OAL_SIZEOF(st_syn_msg);

    OAM_WARNING_LOG3(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_ini_pm_switch_param::da_len[%d].device[%d]host[%d]pm switch}",
                    *pul_data_len, g_wlan_device_pm_switch, g_wlan_pm_switch_etc);
}


OAL_STATIC oal_void hwifi_custom_adapt_device_ini_fast_ps_check_cnt(oal_uint8 *puc_data, oal_uint32 *pul_data_len)
{
    hmac_to_dmac_cfg_custom_data_stru     st_syn_msg;

    if (NULL == puc_data)
    {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_ini_fast_ps_check_cnt::puc_data is NULL data_len[%d].}", *pul_data_len);
        return;
    }

    st_syn_msg.en_syn_id = CUSTOM_CFGID_INI_PS_FAST_CHECK_CNT_ID;

    st_syn_msg.auc_msg_body[0] = g_wlan_fast_check_cnt;
    st_syn_msg.ul_len = OAL_SIZEOF(st_syn_msg) - CUSTOM_MSG_DATA_HDR_LEN;

    oal_memcopy(puc_data, &st_syn_msg, OAL_SIZEOF(st_syn_msg));

    *pul_data_len += OAL_SIZEOF(st_syn_msg);

    OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_ini_fast_ps_check_cnt::da_len[%d].fast ps check cnt[%d]}",
                    *pul_data_len, g_wlan_fast_check_cnt);
}



OAL_STATIC oal_int32 hwifi_custom_adapt_device_priv_ini_radio_cap_param(oal_uint8 *puc_data, oal_uint32 *pul_data_len)
{

    oal_int32                              l_ret;
    hmac_to_dmac_cfg_custom_data_stru      st_syn_msg;
    oal_int32                              l_priv_value   = 0 ;
    oal_uint8                              uc_cmd_idx;
    oal_uint8                              uc_device_idx;
    oal_uint8                              auc_wlan_service_device_per_chip[WLAN_SERVICE_DEVICE_MAX_NUM_PER_CHIP] = {WLAN_INIT_DEVICE_RADIO_CAP};

    if (NULL == puc_data)
    {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_priv_ini_radio_cap_param::puc_data is NULL data_len[%d].}", *pul_data_len);
        return OAL_FAIL;
    }

    /* 为了不影响host device初始化，这里重新获取定制化文件读到的值 */
    uc_cmd_idx = WLAN_CFG_PRIV_DBDC_RADIO_0;
    for (uc_device_idx = 0; uc_device_idx < WLAN_SERVICE_DEVICE_MAX_NUM_PER_CHIP; uc_device_idx++)
    {
        l_ret = hwifi_get_init_priv_value(uc_cmd_idx++, &l_priv_value);
        if (OAL_SUCC == l_ret)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{hwifi_custom_adapt_device_priv_ini_radio_cap_param::WLAN_CFG_PRIV_DBDC_RADIO_0 [%d].}", l_priv_value);
            auc_wlan_service_device_per_chip[uc_device_idx] = (oal_uint8)(oal_uint32)l_priv_value;
        }
    }

    st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_INI_RADIO_CAP_ID;
    st_syn_msg.ul_len = OAL_SIZEOF(auc_wlan_service_device_per_chip);

    oal_memcopy(puc_data, &st_syn_msg, CUSTOM_MSG_DATA_HDR_LEN);
    oal_memcopy(puc_data + CUSTOM_MSG_DATA_HDR_LEN, auc_wlan_service_device_per_chip, OAL_SIZEOF(auc_wlan_service_device_per_chip));

    *pul_data_len += (OAL_SIZEOF(auc_wlan_service_device_per_chip) + CUSTOM_MSG_DATA_HDR_LEN);

    OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_priv_ini_radio_cap_param::da_len[%d] radio_cap_0[%d].}",
                        *pul_data_len, auc_wlan_service_device_per_chip[0]);

    return OAL_SUCC;
}



OAL_STATIC oal_int32 hwifi_custom_adapt_device_priv_ini_temper_thread_param(oal_uint8 *puc_data, oal_uint32 *pul_data_len)
{
    oal_int32                              l_ret;
    hmac_to_dmac_cfg_custom_data_stru      st_syn_msg;
    oal_int32                              l_priv_val   = 0 ;
    oal_uint32                             ul_over_temp_protect_thread;

    if (NULL == puc_data)
    {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_priv_ini_temper_thread_param::puc_data is NULL data_len[%d].}", *pul_data_len);
        return OAL_FAIL;
    }

    l_ret = hwifi_get_init_priv_value(WLAN_CFG_PRIV_OVER_TEMPER_PROTECT_THRESHOLD, &l_priv_val);

    if (OAL_SUCC == l_ret)
    {
        ul_over_temp_protect_thread = (oal_uint32)l_priv_val;
        OAL_IO_PRINT("hwifi_custom_adapt_device_priv_ini_temper_thread_param::read over_temp_protect_thread[%d]\r\n", ul_over_temp_protect_thread);
    }
    else
    {
        return OAL_FAIL;
    }

    st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_INI_OVER_TEMPER_PROTECT_THRESHOLD_ID;
    st_syn_msg.ul_len = OAL_SIZEOF(ul_over_temp_protect_thread) ;

    oal_memcopy(puc_data, &st_syn_msg, CUSTOM_MSG_DATA_HDR_LEN);
    oal_memcopy(puc_data + CUSTOM_MSG_DATA_HDR_LEN, &ul_over_temp_protect_thread, OAL_SIZEOF(ul_over_temp_protect_thread));

    *pul_data_len += (OAL_SIZEOF(ul_over_temp_protect_thread) + CUSTOM_MSG_DATA_HDR_LEN);

    OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_priv_ini_temper_thread_param::da_len[%d] over_temp_protect_thread[0x%x].}", *pul_data_len, ul_over_temp_protect_thread);

    return OAL_SUCC;
}


OAL_STATIC oal_int32 hwifi_custom_adapt_priv_ini_param(wlan_cfg_priv_id_uint8 uc_cfg_id, oal_uint8 *puc_data, oal_uint32 *pul_len)
{
    oal_int32                              l_ret;
    hmac_to_dmac_cfg_custom_data_stru      st_syn_msg;
    oal_int32                              l_priv_val   = 0 ;
    oal_uint8                              uc_priv_cfg_value;
    oal_uint8                              uc_cali_interval;

    if (NULL == puc_data)
    {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{hwifi_custom_adapt_mac_device_priv_ini_param::puc_data is NULL data_len[%d].}", *pul_len);
        return OAL_FAIL;
    }

    l_ret = hwifi_get_init_priv_value(uc_cfg_id, &l_priv_val);
    if(OAL_SUCC != l_ret)
    {
        return OAL_FAIL;
    }

    uc_priv_cfg_value = (oal_uint8)(oal_uint32)l_priv_val;

    switch(uc_cfg_id)
    {
        case WLAN_CFG_PRIV_LDPC_CODING:
            st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_INI_LDPC_CODING_ID;
            OAL_IO_PRINT("hwifi_custom_adapt_mac_device_priv_ini_param::ldpc coding[%d].\r\n", uc_priv_cfg_value);
            break;
        case WLAN_CFG_PRIV_BW_MAX_WITH:
            st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_INI_BW_MAX_WITH_ID;
            OAL_IO_PRINT("hwifi_custom_adapt_mac_device_priv_ini_param::max_bw[%d].\r\n", uc_priv_cfg_value);
            break;
        case WLAN_CFG_PRIV_RX_STBC:
            st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_INI_RX_STBC_ID;
            OAL_IO_PRINT("hwifi_custom_adapt_mac_device_priv_ini_param::rx_stbc[%d].\r\n", uc_priv_cfg_value);
            break;
        case WLAN_CFG_PRIV_TX_STBC:
            st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_INI_TX_STBC_ID;
            OAL_IO_PRINT("hwifi_custom_adapt_mac_device_priv_ini_param::tx_stbc[%d].\r\n", uc_priv_cfg_value);
            break;
        case WLAN_CFG_PRIV_SU_BFER:
            st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_INI_SU_BFER_ID;
            OAL_IO_PRINT("hwifi_custom_adapt_mac_device_priv_ini_param::su bfer[%d].\r\n", uc_priv_cfg_value);
            break;
        case WLAN_CFG_PRIV_SU_BFEE:
            st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_INI_SU_BFEE_ID;
            OAL_IO_PRINT("hwifi_custom_adapt_mac_device_priv_ini_param::su bfee[%d].\r\n", uc_priv_cfg_value);
            break;
        case WLAN_CFG_PRIV_MU_BFER:
            st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_INI_MU_BFER_ID;
            OAL_IO_PRINT("hwifi_custom_adapt_mac_device_priv_ini_param::mu bfer[%d].\r\n", uc_priv_cfg_value);
            break;
        case WLAN_CFG_PRIV_MU_BFEE:
            st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_INI_MU_BFEE_ID;
            OAL_IO_PRINT("hwifi_custom_adapt_mac_device_priv_ini_param::mu bfee[%d].\r\n", uc_priv_cfg_value);
            break;
        case WLAN_CFG_PRIV_11N_TXBF:
            st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_INI_11N_TXBF_ID;
            OAL_IO_PRINT("hwifi_custom_adapt_mac_device_priv_ini_param::11n txbf[%d].\r\n", uc_priv_cfg_value);
            break;

        case WLAN_CFG_PRIV_1024_QAM:
            st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_INI_1024_QAM_ID;
            OAL_IO_PRINT("hwifi_custom_adapt_mac_device_priv_ini_param::1024qam[%d].\r\n", uc_priv_cfg_value);
            break;
        case WLAN_CFG_PRIV_CALI_DATA_MASK:
            /* 开机默认打开校准数据上传下发 */
            g_uc_wlan_open_cnt++;

            if (g_uc_custom_cali_done_etc == OAL_FALSE)
            {
                uc_priv_cfg_value |= (CALI_DATA_REFRESH_MASK | CALI_DATA_REUPLOAD_MASK);
            }
            else
            {
                if (uc_priv_cfg_value & CALI_DATA_REFRESH_MASK)
                {
                    uc_cali_interval = (uc_priv_cfg_value >> CALI_INTVL_OFFSET) + 1;
                    if (g_uc_wlan_open_cnt % uc_cali_interval)
                    {
                        uc_priv_cfg_value &= (~CALI_DATA_REFRESH_MASK);
                    }
                }

            }
            st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_CALI_DATA_MASK_ID;
            OAL_IO_PRINT("hwifi_custom_adapt_mac_device_priv_ini_param::g_uc_wlan_open_cnt[%d]priv_cali_data_up_down[0x%x].\r\n",
                           g_uc_wlan_open_cnt, uc_priv_cfg_value);
            break;
        case WLAN_CFG_PRIV_CALI_AUTOCALI_MASK:
            /* 开机默认不打开开机校准 */
            uc_priv_cfg_value = (g_uc_custom_cali_done_etc == OAL_FALSE) ? OAL_FALSE : uc_priv_cfg_value;
            st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_INI_AUTOCALI_MASK_ID;
            OAL_IO_PRINT("hwifi_custom_adapt_mac_device_priv_ini_param::g_uc_custom_cali_done_etc[%d]auto_cali_mask[0x%x].\r\n",
                           g_uc_custom_cali_done_etc, uc_priv_cfg_value);
            break;

        case WLAN_CFG_PRIV_M2S_FUNCTION_MASK:
            st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_M2S_FUNCTION_MASK_ID;
            OAL_IO_PRINT("hwifi_custom_adapt_mac_device_priv_ini_param::btcoex_mask[0x%x].\r\n",uc_priv_cfg_value);
            break;

        case WLAN_CFG_PRIV_FASTSCAN_SWITCH:
            st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_FASTSCAN_SWITCH_ID;
            OAL_IO_PRINT("hwifi_custom_adapt_mac_device_priv_ini_param::fastcan [0x%x].\r\n",uc_priv_cfg_value);
            break;
        case WLAN_CFG_ANT_SWITCH:
            st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_ANT_SWITCH_ID;
            OAL_IO_PRINT("hwifi_custom_adapt_mac_device_priv_ini_param::temp pro safe th[%d].\r\n", uc_priv_cfg_value);
            break;
#ifdef _PRE_WLAN_FEATURE_TXOPPS
            case WLAN_CFG_PRIV_TXOPPS_SWITCH:
            st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_INI_TXOPPS_SWITCH_ID;
            OAL_IO_PRINT("hwifi_custom_adapt_mac_device_priv_ini_param::uc_priv_cfg_value[0x%x].\r\n",uc_priv_cfg_value);
            break;
#endif
        case WLAN_CFG_PRIV_OVER_TEMP_PRO_ENABLE:
            st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_INI_TEMP_PRO_ENABLE_ID;
            OAL_IO_PRINT("hwifi_custom_adapt_mac_device_priv_ini_param::temp pro enable[%d].\r\n", uc_priv_cfg_value);
            break;
        case WLAN_CFG_PRIV_OVER_TEMP_PRO_REDUCE_PWR_ENABLE:
            st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_INI_TEMP_PRO_REDUCE_PWR_ENABLE_ID;
            OAL_IO_PRINT("hwifi_custom_adapt_mac_device_priv_ini_param::temp pro reduce pwr enable[%d].\r\n", uc_priv_cfg_value);
            break;
        case WLAN_CFG_PRIV_OVER_TEMP_PRO_SAFE_TH:
            st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_INI_TEMP_PRO_SAFE_TH_ID;
            OAL_IO_PRINT("hwifi_custom_adapt_mac_device_priv_ini_param::temp pro safe th[%d].\r\n", uc_priv_cfg_value);
            break;
        case WLAN_CFG_PRIV_OVER_TEMP_PRO_OVER_TH:
            st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_INI_TEMP_PRO_OVER_TH_ID;
            OAL_IO_PRINT("hwifi_custom_adapt_mac_device_priv_ini_param::temp pro over th[%d].\r\n", uc_priv_cfg_value);
            break;
        case WLAN_CFG_PRIV_OVER_TEMP_PRO_PA_OFF_TH:
            st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_INI_TEMP_PRO_PA_OFF_TH_ID;
            OAL_IO_PRINT("hwifi_custom_adapt_mac_device_priv_ini_param::temp pro pa off th[%d].\r\n", uc_priv_cfg_value);
            break;

        case WLAN_CFG_PRIV_EVM_PLL_REG_FIX:
            st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_INI_EVM_PLL_REG_FIX_ID;
            OAL_IO_PRINT("hwifi_custom_adapt_mac_device_priv_ini_param::temp pro safe th[%d].\r\n", uc_priv_cfg_value);
            break;
        case WLAN_CFG_PRIV_VOE_SWITCH:
            st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_INI_VOE_SWITCH_ID;
            OAL_IO_PRINT("hwifi_custom_adapt_mac_device_priv_ini_param::temp pro safe th[%d].\r\n", uc_priv_cfg_value);
            break;
        case WLAN_CFG_PRIV_11AX_SWITCH:
            st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_INI_11AX_SWITCH_ID;
            OAL_IO_PRINT("hwifi_custom_adapt_mac_device_priv_ini_param::WLAN_CFG_PRIV_11AX_SWITCH = [%d].\r\n", uc_priv_cfg_value);
            break;
        case WLAN_CFG_PRIV_DYN_BYPASS_EXTLNA:
            st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_DYN_BYPASS_EXTLNA_ID;
            OAL_IO_PRINT("hwifi_custom_adapt_mac_device_priv_ini_param::dyn_bypass_extlna_enable[%d].\r\n", uc_priv_cfg_value);
            break;
        case WLAN_CFG_PRIV_CTRL_FRAME_TX_CHAIN:
            st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_CTRL_FRAME_TX_CHAIN_ID;
            OAL_IO_PRINT("hwifi_custom_adapt_mac_device_priv_ini_param::Ctrl frame tx chain[%d].\r\n", uc_priv_cfg_value);
            break;
        case WLAN_CFG_PRIV_CTRL_UPC_FOR_18DBM_CO:
            st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_CTRL_UPC_FOR_18DBM_C0_ID;
            OAL_IO_PRINT("hwifi_custom_adapt_mac_device_priv_ini_param::18dBm_upc0[0x%x].\r\n", uc_priv_cfg_value);
            break;
        case WLAN_CFG_PRIV_CTRL_UPC_FOR_18DBM_C1:
            st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_CTRL_UPC_FOR_18DBM_C1_ID;
            OAL_IO_PRINT("hwifi_custom_adapt_mac_device_priv_ini_param::18dBm_upc1[0x%x].\r\n", uc_priv_cfg_value);
            break;

        default:
            break;
    }

    st_syn_msg.ul_len = OAL_SIZEOF(uc_priv_cfg_value);
    oal_memcopy(puc_data, &st_syn_msg, CUSTOM_MSG_DATA_HDR_LEN);
    oal_memcopy(puc_data + CUSTOM_MSG_DATA_HDR_LEN, &uc_priv_cfg_value, OAL_SIZEOF(uc_priv_cfg_value));

    *pul_len += (OAL_SIZEOF(uc_priv_cfg_value) + CUSTOM_MSG_DATA_HDR_LEN);

    /*
    OAM_WARNING_LOG3(0, OAM_SF_CUSTOM, "{hwifi_custom_adapt_mac_device_priv_ini_param::cfg_id[%d] data_len[%d] cfg_val[0x%x].}", uc_cfg_id, *pul_len, uc_priv_cfg_value);
    */

    return OAL_SUCC;
}



OAL_STATIC oal_int32 hwifi_custom_adapt_device_priv_ini_dsss2ofdm_dbb_pwr_bo_param(oal_uint8 *puc_data, oal_uint32 *pul_data_len)
{
    oal_int32                              l_ret;
    hmac_to_dmac_cfg_custom_data_stru      st_syn_msg;
    oal_int32                              l_priv_val   = 0 ;
    oal_int16                              l_dsss2ofdm_dbb_pwr_bo;

    if (NULL == puc_data)
    {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_priv_ini_dsss2ofdm_dbb_pwr_bo_param::puc_data is NULL data_len[%d].}", *pul_data_len);
        return OAL_FAIL;
    }

    l_ret = hwifi_get_init_priv_value(WLAN_DSSS2OFDM_DBB_PWR_BO_VAL, &l_priv_val);
    if (OAL_SUCC != l_ret)
    {
        return OAL_FAIL;
    }

    l_dsss2ofdm_dbb_pwr_bo = (oal_int16)l_priv_val;
    st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_INI_DSSS2OFDM_DBB_PWR_BO_VAL_ID;
    st_syn_msg.ul_len = OAL_SIZEOF(l_dsss2ofdm_dbb_pwr_bo) ;
    oal_memcopy(puc_data, &st_syn_msg, CUSTOM_MSG_DATA_HDR_LEN);
    oal_memcopy(puc_data + CUSTOM_MSG_DATA_HDR_LEN, &l_dsss2ofdm_dbb_pwr_bo, OAL_SIZEOF(l_dsss2ofdm_dbb_pwr_bo));
    *pul_data_len += (OAL_SIZEOF(l_dsss2ofdm_dbb_pwr_bo) + CUSTOM_MSG_DATA_HDR_LEN);

    OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_priv_ini_dsss2ofdm_dbb_pwr_bo_param::da_len[%d] l_dsss2ofdm_dbb_pwr_bo[0x%x].}", *pul_data_len, l_dsss2ofdm_dbb_pwr_bo);
    return OAL_SUCC;
}



OAL_STATIC oal_int32 hwifi_custom_adapt_device_priv_ini_cali_mask_param(oal_uint8 *puc_data, oal_uint32 *pul_data_len)
{
    oal_int32                              l_ret;
    hmac_to_dmac_cfg_custom_data_stru      st_syn_msg;
    oal_int32                              l_priv_val   = 0 ;
    oal_uint16                             us_cali_mask;

    if (NULL == puc_data)
    {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_priv_ini_cali_mask_param::puc_data is NULL data_len[%d].}", *pul_data_len);
        return OAL_FAIL;
    }

    l_ret = hwifi_get_init_priv_value(WLAN_CFG_PRIV_CALI_MASK, &l_priv_val);

    if (OAL_SUCC == l_ret)
    {
        us_cali_mask = (oal_uint16)(oal_uint32)l_priv_val;
        OAL_IO_PRINT("hwifi_custom_adapt_device_priv_ini_cali_mask_param::read cali_mask[%d]l_ret[%d]\r\n", us_cali_mask, l_ret);
    }
    else
    {
        return OAL_FAIL;
    }

    st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_INI_CALI_MASK_ID;
    st_syn_msg.ul_len = OAL_SIZEOF(us_cali_mask) ;

    oal_memcopy(puc_data, &st_syn_msg, CUSTOM_MSG_DATA_HDR_LEN);
    oal_memcopy(puc_data + CUSTOM_MSG_DATA_HDR_LEN, &us_cali_mask, OAL_SIZEOF(us_cali_mask));

    *pul_data_len += (OAL_SIZEOF(us_cali_mask) + CUSTOM_MSG_DATA_HDR_LEN);

    OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_priv_ini_cali_mask_param::da_len[%d] cali_mask[0x%x].}", *pul_data_len, us_cali_mask);

    return OAL_SUCC;
}
#ifdef _PRE_WLAN_DOWNLOAD_PM

OAL_STATIC oal_int32 hwifi_custom_adapt_device_priv_ini_download_pm_param(oal_uint8 *puc_data, oal_uint32 *pul_data_len)
{
    oal_int32                              l_ret;
    hmac_to_dmac_cfg_custom_data_stru      st_syn_msg;
    oal_int32                              l_priv_val   = 0 ;
    oal_uint16                             us_download_rate_limit_pps;

    if (NULL == puc_data)
    {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_priv_ini_download_pm_param::puc_data is NULL data_len[%d].}", *pul_data_len);
        return OAL_FAIL;
    }

    l_ret = hwifi_get_init_priv_value(WLAN_CFG_PRIV_DOWNLOAD_RATE_LIMIT_PPS, &l_priv_val);

    if (OAL_SUCC == l_ret)
    {
        us_download_rate_limit_pps = (oal_uint16)(oal_uint32)l_priv_val;
        OAL_IO_PRINT("hwifi_custom_adapt_device_priv_ini_download_pm_param::read download_rate_limit_pps[%d]l_ret[%d]\r\n", us_download_rate_limit_pps, l_ret);
    }
    else
    {
        return OAL_FAIL;
    }

    st_syn_msg.en_syn_id = CUSTOM_CFGID_PRIV_INI_DOWNLOAD_RATELIMIT_PPS;
    st_syn_msg.ul_len = OAL_SIZEOF(us_download_rate_limit_pps) ;

    oal_memcopy(puc_data, &st_syn_msg, CUSTOM_MSG_DATA_HDR_LEN);
    oal_memcopy(puc_data + CUSTOM_MSG_DATA_HDR_LEN, &us_download_rate_limit_pps, OAL_SIZEOF(us_download_rate_limit_pps));

    *pul_data_len += (OAL_SIZEOF(us_download_rate_limit_pps) + CUSTOM_MSG_DATA_HDR_LEN);

    OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_priv_ini_download_pm_param::da_len[%d] download_rate_limit [%d]pps.}", *pul_data_len, us_download_rate_limit_pps);

    return OAL_SUCC;
}
#endif


int32 hwifi_custom_adapt_device_ini_param(oal_uint8 *puc_data)
{
    oal_uint32                     ul_data_len = 0;

    if (NULL == puc_data)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_ini_param::puc_data is NULL.}");
        return INI_FAILED;
    }

    /* 发送消息的格式如下:                                                   */
    /* +-------------------------------------------------------------------+ */
    /* | CFGID0    |DATA0 Length| DATA0 Value | ......................... | */
    /* +-------------------------------------------------------------------+ */
    /* | 4 Bytes   |4 Byte      | DATA  Length| ......................... | */
    /* +-------------------------------------------------------------------+ */

#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
    /* 自动调频 */
    hwifi_custom_adapt_device_ini_freq_param(puc_data + ul_data_len, &ul_data_len);
#endif //  #ifdef _PRE_WLAN_FEATURE_AUTO_FREQ

    /* 性能 */
    hwifi_custom_adapt_device_ini_perf_param(puc_data + ul_data_len, &ul_data_len);

    /* linkloss */
    hwifi_custom_adapt_device_ini_linkloss_param(puc_data + ul_data_len, &ul_data_len);

    /* 低功耗 */
    hwifi_custom_adapt_device_ini_pm_switch_param(puc_data + ul_data_len, &ul_data_len);

    /* fast ps mode 检查次数*/
    hwifi_custom_adapt_device_ini_fast_ps_check_cnt(puc_data + ul_data_len, &ul_data_len);

    /* ldac m2s rssi门限 */
    hwifi_custom_adapt_device_ini_ldac_m2s_rssi_param(puc_data + ul_data_len, &ul_data_len);

    /* 结束 */
    hwifi_custom_adapt_device_ini_end_param(puc_data + ul_data_len, &ul_data_len);

    return ul_data_len;
}


int32 hwifi_custom_adapt_device_priv_ini_param(oal_uint8 *puc_data)
{
    oal_uint32                     ul_data_len = 0;

    if (NULL == puc_data)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hwifi_custom_adapt_device_priv_ini_param::puc_data is NULL.}");
        return INI_FAILED;
    }

    /* 发送消息的格式如下:                                                   */
    /* +-------------------------------------------------------------------+ */
    /* | CFGID0    |DATA0 Length| DATA0 Value | ......................... | */
    /* +-------------------------------------------------------------------+ */
    /* | 4 Bytes   |4 Byte      | DATA  Length| ......................... | */
    /* +-------------------------------------------------------------------+ */

    /* 私有定制化 */
    hwifi_custom_adapt_device_priv_ini_radio_cap_param(puc_data, &ul_data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_BW_MAX_WITH, puc_data + ul_data_len, &ul_data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_LDPC_CODING, puc_data + ul_data_len, &ul_data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_RX_STBC, puc_data + ul_data_len, &ul_data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_TX_STBC, puc_data + ul_data_len, &ul_data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_SU_BFER, puc_data + ul_data_len, &ul_data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_SU_BFEE, puc_data + ul_data_len, &ul_data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_MU_BFER, puc_data + ul_data_len, &ul_data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_MU_BFEE, puc_data + ul_data_len, &ul_data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_11N_TXBF, puc_data + ul_data_len, &ul_data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_1024_QAM, puc_data + ul_data_len, &ul_data_len);
    hwifi_custom_adapt_device_priv_ini_cali_mask_param(puc_data + ul_data_len, &ul_data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_CALI_DATA_MASK, puc_data + ul_data_len, &ul_data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_CALI_AUTOCALI_MASK, puc_data + ul_data_len, &ul_data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_OVER_TEMP_PRO_ENABLE, puc_data + ul_data_len, &ul_data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_OVER_TEMP_PRO_REDUCE_PWR_ENABLE, puc_data + ul_data_len, &ul_data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_OVER_TEMP_PRO_SAFE_TH, puc_data + ul_data_len, &ul_data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_OVER_TEMP_PRO_OVER_TH, puc_data + ul_data_len, &ul_data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_OVER_TEMP_PRO_PA_OFF_TH, puc_data + ul_data_len, &ul_data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_FASTSCAN_SWITCH, puc_data + ul_data_len, &ul_data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_ANT_SWITCH, puc_data + ul_data_len, &ul_data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_M2S_FUNCTION_MASK, puc_data + ul_data_len, &ul_data_len);

#ifdef _PRE_WLAN_DOWNLOAD_PM
    hwifi_custom_adapt_device_priv_ini_download_pm_param(puc_data + ul_data_len, &ul_data_len);
#endif
#ifdef _PRE_WLAN_FEATURE_TXOPPS
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_TXOPPS_SWITCH, puc_data + ul_data_len, &ul_data_len);
#endif
    hwifi_custom_adapt_device_priv_ini_temper_thread_param(puc_data + ul_data_len, &ul_data_len);
    hwifi_custom_adapt_device_priv_ini_dsss2ofdm_dbb_pwr_bo_param(puc_data + ul_data_len, &ul_data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_EVM_PLL_REG_FIX, puc_data + ul_data_len, &ul_data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_VOE_SWITCH, puc_data + ul_data_len, &ul_data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_11AX_SWITCH, puc_data + ul_data_len, &ul_data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_DYN_BYPASS_EXTLNA, puc_data + ul_data_len, &ul_data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_CTRL_FRAME_TX_CHAIN, puc_data + ul_data_len, &ul_data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_CTRL_UPC_FOR_18DBM_CO, puc_data + ul_data_len, &ul_data_len);
    hwifi_custom_adapt_priv_ini_param(WLAN_CFG_PRIV_CTRL_UPC_FOR_18DBM_C1, puc_data + ul_data_len, &ul_data_len);
    OAL_IO_PRINT("hwifi_custom_adapt_device_priv_ini_param::data_len[%d]\r\n", ul_data_len);

    return ul_data_len;
}


int32 hwifi_hcc_custom_ini_data_buf(uint16 us_syn_id)
{
    oal_netbuf_stru                       *pst_netbuf;
    oal_uint32                             ul_data_len = 0;
    oal_int32                              l_ret;
    oal_uint32                             ul_max_data_len;

    struct hcc_transfer_param st_hcc_transfer_param = {0};
    struct hcc_handler* hcc = hcc_get_110x_handler();
    if(NULL == hcc)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "hwifi_hcc_custom_ini_data_buf hcc::is is null");
        return -OAL_EFAIL;
    }

    ul_max_data_len = hcc_get_max_buf_len();
    pst_netbuf = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, WLAN_LARGE_NETBUF_SIZE, OAL_NETBUF_PRIORITY_HIGH);

    if(NULL == pst_netbuf)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "hwifi_hcc_custom_ini_data_buf::alloc netbuf fail.");
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    /*组netbuf*/
    if (CUSTOM_CFGID_INI_ID == us_syn_id)
    {
        /* INI hmac to dmac 配置项*/
        ul_data_len = hwifi_custom_adapt_device_ini_param((oal_uint8 *)OAL_NETBUF_DATA(pst_netbuf));
    }
    else if (CUSTOM_CFGID_PRIV_INI_ID == us_syn_id)
    {
        /* 私有定制化配置项 */
        ul_data_len = hwifi_custom_adapt_device_priv_ini_param((oal_uint8 *)OAL_NETBUF_DATA(pst_netbuf));
    }
    else
    {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "hwifi_hcc_custom_ini_data_buf::unknown us_syn_id[%d]", us_syn_id);
    }

    if (ul_data_len > ul_max_data_len)
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "hwifi_hcc_custom_ini_data_buf::got wrong ul_data_len[%d] max_len[%d]", ul_data_len, ul_max_data_len);
        oal_netbuf_free(pst_netbuf);
        return OAL_FAIL;
    }

    if (0 == ul_data_len)
    {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "hwifi_hcc_custom_ini_data_buf::data is null us_syn_id[%d]", us_syn_id);
        oal_netbuf_free(pst_netbuf);
        return OAL_SUCC;
    }

    if((pst_netbuf->data_len) || (NULL == pst_netbuf->data))
    {
        OAL_IO_PRINT("netbuf:0x%lx, len:%d\r\n", (oal_ulong)pst_netbuf, pst_netbuf->data_len);
         return OAL_FAIL;
    }

    oal_netbuf_put(pst_netbuf, ul_data_len);
    hcc_hdr_param_init(&st_hcc_transfer_param,
                        HCC_ACTION_TYPE_CUSTOMIZE,
                        us_syn_id,
                        0,
                        HCC_FC_WAIT,
                        DATA_HI_QUEUE);

    l_ret = (oal_uint32)hcc_tx_etc(hcc, pst_netbuf, &st_hcc_transfer_param);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "hwifi_hcc_custom_ini_data_buf fail ret[%d]pst_netbuf[%p]", l_ret, pst_netbuf);
        oal_netbuf_free(pst_netbuf);
    }

    return l_ret;
}


oal_int32 hwifi_custom_host_read_dyn_cali_nvram(oal_void)
{
    oal_int32      l_ret;
    oal_uint8      uc_idx;
    oal_uint8      uc_param_idx;
    oal_uint8      uc_times_idx = 0;
    oal_int8      *puc_str;
    oal_uint8     *pc_end = ";";
    oal_uint8     *pc_sep = ",";
    oal_int8      *pc_ctx;
    oal_int8      *pc_token;
    oal_int32      l_priv_value;
    oal_bool_enum_uint8 en_get_nvram_data_flag = OAL_FALSE;
    oal_uint8     *puc_buffer_cust_nvram_tmp   = OAL_PTR_NULL;
    oal_int32     *pl_params                   = OAL_PTR_NULL;
    oal_uint8     *puc_cust_nvram_info         = OAL_PTR_NULL;  /* NVRAM数组 */
    oal_bool_enum_uint8 en_fact_cali_completed = OAL_FALSE;


    /* 判断定制化中是否使用nvram中的动态校准参数 */
    l_ret = hwifi_get_init_priv_value(WLAN_CFG_PRIV_CALI_DATA_MASK, &l_priv_value);
    if (OAL_SUCC == l_ret)
    {
        en_get_nvram_data_flag = !!(HI1103_CUST_READ_NVRAM_MASK & l_priv_value);
        if (en_get_nvram_data_flag)
        {
            OAL_IO_PRINT("hwifi_custom_host_read_dyn_cali_nvram::get_nvram_data_flag[%d] to abandon nvram data!!\r\n", l_priv_value);
            OAL_MEMZERO(g_auc_cust_nvram_info, OAL_SIZEOF(g_auc_cust_nvram_info));
            return INI_FILE_TIMESPEC_UNRECONFIG;
        }
    }

    puc_buffer_cust_nvram_tmp = (oal_uint8 *)OS_KZALLOC_GFP(CUS_PARAMS_LEN_MAX * OAL_SIZEOF(oal_uint8));
    if (OAL_PTR_NULL == puc_buffer_cust_nvram_tmp)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CUSTOM, "hwifi_custom_host_read_dyn_cali_nvram::puc_buffer_cust_nvram_tmp mem alloc fail!");
        OAL_MEMZERO(g_auc_cust_nvram_info, OAL_SIZEOF(g_auc_cust_nvram_info));
        return INI_FILE_TIMESPEC_UNRECONFIG;
    }

    pl_params = (oal_int32 *)OS_KZALLOC_GFP(DY_CALI_PARAMS_NUM*DY_CALI_PARAMS_TIMES * OAL_SIZEOF(oal_int32));
    if (OAL_PTR_NULL == pl_params)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CUSTOM, "hwifi_custom_host_read_dyn_cali_nvram::ps_params mem alloc fail!");
        OS_MEM_KFREE(puc_buffer_cust_nvram_tmp);
        OAL_MEMZERO(g_auc_cust_nvram_info, OAL_SIZEOF(g_auc_cust_nvram_info));
        return INI_FILE_TIMESPEC_UNRECONFIG;
    }

    puc_cust_nvram_info = (oal_uint8 *)OS_KZALLOC_GFP(WLAN_CFG_DTS_NVRAM_END * CUS_PARAMS_LEN_MAX * OAL_SIZEOF(oal_uint8));
    if (OAL_PTR_NULL == puc_cust_nvram_info)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CUSTOM, "hwifi_custom_host_read_dyn_cali_nvram::puc_cust_nvram_info mem alloc fail!");
        OS_MEM_KFREE(puc_buffer_cust_nvram_tmp);
        OS_MEM_KFREE(pl_params);
        OAL_MEMZERO(g_auc_cust_nvram_info, OAL_SIZEOF(g_auc_cust_nvram_info));
        return INI_FILE_TIMESPEC_UNRECONFIG;
    }

    OAL_MEMZERO(puc_buffer_cust_nvram_tmp, CUS_PARAMS_LEN_MAX * OAL_SIZEOF(oal_uint8));
    OAL_MEMZERO(pl_params, DY_CALI_PARAMS_NUM*DY_CALI_PARAMS_TIMES * OAL_SIZEOF(oal_int32));
    OAL_MEMZERO(puc_cust_nvram_info, WLAN_CFG_DTS_NVRAM_END * CUS_PARAMS_LEN_MAX * OAL_SIZEOF(oal_uint8));

#ifdef _PRE_WLAN_DPINIT_CALI
    /* DP init */
    for (uc_idx = WLAN_CFG_NVRAM_DP2G_INIT0; uc_idx <= WLAN_CFG_NVRAM_DP2G_INIT1; uc_idx++)
    {
        l_ret = read_conf_from_nvram_etc(puc_buffer_cust_nvram_tmp, CUS_PARAMS_LEN_MAX,
                         g_ast_wifi_nvram_cfg_handler[uc_idx].ul_nv_map_idx, g_ast_wifi_nvram_cfg_handler[uc_idx].puc_nv_name);

        if (l_ret != INI_SUCC)
        {
            OAM_WARNING_LOG1(0, OAM_SF_CUSTOM, "hwifi_custom_host_read_dyn_cali_nvram::get dpint null NV id[%d]!",
                           g_ast_wifi_nvram_cfg_handler[uc_idx].ul_nv_map_idx);
            break;
        }

        puc_str = OAL_STRSTR(puc_buffer_cust_nvram_tmp, g_ast_wifi_nvram_cfg_handler[uc_idx].puc_param_name);
        if(puc_str == OAL_PTR_NULL)
        {
            l_ret = INI_FAILED;
            break;
        }

        /* 获取等号后面的实际参数 */
        puc_str += (OAL_STRLEN(g_ast_wifi_nvram_cfg_handler[uc_idx].puc_param_name) + 1);
        pc_token = oal_strtok(puc_str, pc_end, &pc_ctx);
        if (OAL_PTR_NULL == pc_token)
        {
            OAM_ERROR_LOG1(0, OAM_SF_CUSTOM, "hwifi_custom_host_read_dyn_cali_nvram::read get null check id[%d]!",
                             g_ast_wifi_nvram_cfg_handler[uc_idx].ul_nv_map_idx);
            l_ret = INI_FAILED;
            break;
        }

        oal_memcopy(puc_cust_nvram_info + (uc_idx * CUS_PARAMS_LEN_MAX * OAL_SIZEOF(oal_uint8)), pc_token, OAL_STRLEN(pc_token));
        *(puc_cust_nvram_info + (uc_idx * CUS_PARAMS_LEN_MAX * OAL_SIZEOF(oal_uint8)) + OAL_STRLEN(pc_token)) = *pc_end;

        pc_token = oal_strtok(pc_token, pc_sep, &pc_ctx);

        /* 产测系数合理性检查 */
        while (OAL_PTR_NULL != pc_token)
        {
            OAL_IO_PRINT( "hwifi_custom_host_read_dyn_cali_nvram::get [%s]\n!", pc_token);
            pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
            uc_times_idx++;
        }

        if (uc_times_idx % MAC_2G_CHANNEL_NUM)
        {
            OAM_ERROR_LOG1(0, OAM_SF_CUSTOM, "hwifi_custom_host_read_dyn_cali_nvram::get wrong times[%d]!",
                           g_ast_wifi_nvram_cfg_handler[uc_idx].ul_nv_map_idx);
            l_ret = INI_FAILED;
            break;
        }
        uc_times_idx = 0;
    }

    if (l_ret != INI_SUCC)
    {
        /* nvram中读取DPinit异常 */
        OAL_MEMZERO(puc_cust_nvram_info, WLAN_CFG_DTS_NVRAM_END * CUS_PARAMS_LEN_MAX * OAL_SIZEOF(oal_uint8));
        /* 更新标志位给产线读取 */
        g_en_nv_dp_init_is_null = OAL_TRUE;
    }
    else
    {
        g_en_nv_dp_init_is_null = OAL_FALSE;
    }
#endif //#ifdef _PRE_WLAN_DPINIT_CALI

    /* 拟合系数 */
    for (uc_idx = WLAN_CFG_DTS_NVRAM_RATIO_PA2GCCKA0; uc_idx < WLAN_CFG_DTS_NVRAM_END; uc_idx++)
    {
        l_ret = read_conf_from_nvram_etc(puc_buffer_cust_nvram_tmp, CUS_PARAMS_LEN_MAX,
                                     g_ast_wifi_nvram_cfg_handler[uc_idx].ul_nv_map_idx,
                                     g_ast_wifi_nvram_cfg_handler[uc_idx].puc_nv_name);
        if (l_ret != INI_SUCC)
        {
            OAL_MEMZERO(puc_cust_nvram_info + (uc_idx * CUS_PARAMS_LEN_MAX * OAL_SIZEOF(oal_uint8)), CUS_PARAMS_LEN_MAX*OAL_SIZEOF(oal_uint8));
            OAL_IO_PRINT("hwifi_custom_host_read_dyn_cali_nvram::NVRAM get fail NV id[%d] name[%s] para[%s]!\r\n",
                         g_ast_wifi_nvram_cfg_handler[uc_idx].ul_nv_map_idx,
                         g_ast_wifi_nvram_cfg_handler[uc_idx].puc_nv_name,
                         g_ast_wifi_nvram_cfg_handler[uc_idx].puc_param_name);
            continue;
        }

        puc_str = OAL_STRSTR(puc_buffer_cust_nvram_tmp, g_ast_wifi_nvram_cfg_handler[uc_idx].puc_param_name);
        if (puc_str == OAL_PTR_NULL)
        {
            OAL_MEMZERO(puc_cust_nvram_info + (uc_idx * CUS_PARAMS_LEN_MAX * OAL_SIZEOF(oal_uint8)), CUS_PARAMS_LEN_MAX*OAL_SIZEOF(oal_uint8));
            OAL_IO_PRINT("hwifi_custom_host_read_dyn_cali_nvram::NVRAM get wrong val NV id[%d] name[%s] para[%s]!\r\n",
                         g_ast_wifi_nvram_cfg_handler[uc_idx].ul_nv_map_idx,
                         g_ast_wifi_nvram_cfg_handler[uc_idx].puc_nv_name,
                         g_ast_wifi_nvram_cfg_handler[uc_idx].puc_param_name);
            continue;
        }

        /* 获取等号后面的实际参数 */
        puc_str += (OAL_STRLEN(g_ast_wifi_nvram_cfg_handler[uc_idx].puc_param_name) + 1);
        pc_token = oal_strtok(puc_str, pc_end, &pc_ctx);
        if (OAL_PTR_NULL == pc_token)
        {
            OAM_ERROR_LOG1(0, OAM_SF_CUSTOM, "hwifi_custom_host_read_dyn_cali_nvram::get null value check NV id[%d]!",
                           g_ast_wifi_nvram_cfg_handler[uc_idx].ul_nv_map_idx);
            OAL_IO_PRINT("hwifi_custom_host_read_dyn_cali_nvram::get null check NV id[%d] name[%s] para[%s]!\r\n", g_ast_wifi_nvram_cfg_handler[uc_idx].ul_nv_map_idx,
                         g_ast_wifi_nvram_cfg_handler[uc_idx].puc_nv_name, g_ast_wifi_nvram_cfg_handler[uc_idx].puc_param_name);
            OAL_MEMZERO(puc_cust_nvram_info + (uc_idx * CUS_PARAMS_LEN_MAX * OAL_SIZEOF(oal_uint8)), CUS_PARAMS_LEN_MAX*OAL_SIZEOF(oal_uint8));
            continue;
        }
        oal_memcopy(puc_cust_nvram_info + (uc_idx * CUS_PARAMS_LEN_MAX * OAL_SIZEOF(oal_uint8)), pc_token, OAL_STRLEN(pc_token));
        *(puc_cust_nvram_info + (uc_idx * CUS_PARAMS_LEN_MAX * OAL_SIZEOF(oal_uint8)) + OAL_STRLEN(pc_token)) = *pc_end;


        /* 拟合系数获取检查 */
        if (uc_idx <= WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1_LOW)
        {
            /* 二次参数合理性检查 */
            pc_token = oal_strtok(pc_token, pc_sep, &pc_ctx);
            uc_param_idx = 0;
            /* 获取定制化系数 */
            while (OAL_PTR_NULL != pc_token)
            {
                OAL_IO_PRINT( "hwifi_custom_host_read_dyn_cali_nvram::get [%s]\n!", pc_token);
                *(pl_params + uc_param_idx) = (oal_int32)oal_strtol(pc_token, OAL_PTR_NULL, 10);
                pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
                uc_param_idx++;
            }
            if (uc_param_idx % DY_CALI_PARAMS_TIMES)
            {
                OAM_ERROR_LOG1(0, OAM_SF_CUSTOM, "hwifi_custom_host_read_dyn_cali_nvram::check NV id[%d]!",
                               g_ast_wifi_nvram_cfg_handler[uc_idx].ul_nv_map_idx);
                OAL_MEMZERO(puc_cust_nvram_info + (uc_idx * CUS_PARAMS_LEN_MAX * OAL_SIZEOF(oal_uint8)), CUS_PARAMS_LEN_MAX*OAL_SIZEOF(oal_uint8));
                continue;
            }
            uc_times_idx = uc_param_idx / DY_CALI_PARAMS_TIMES;
            /* 二次项系数非0检查 */
            while (uc_times_idx--)
            {
                if (0 == pl_params[(uc_times_idx)*DY_CALI_PARAMS_TIMES])
                {
                    OAM_ERROR_LOG1(0, OAM_SF_CUSTOM, "hwifi_custom_host_read_dyn_cali_nvram::check NV id[%d]!",
                                   g_ast_wifi_nvram_cfg_handler[uc_idx].ul_nv_map_idx);
                    OAL_MEMZERO(puc_cust_nvram_info + (uc_idx * CUS_PARAMS_LEN_MAX * OAL_SIZEOF(oal_uint8)), CUS_PARAMS_LEN_MAX*OAL_SIZEOF(oal_uint8));
                    break;
                }
            }

            if (OAL_FALSE == en_fact_cali_completed)
            {
                en_fact_cali_completed = OAL_TRUE;
            }
        }
    }

    g_en_fact_cali_completed = en_fact_cali_completed;

    OS_MEM_KFREE(puc_buffer_cust_nvram_tmp);
    OS_MEM_KFREE(pl_params);

    /*检查NVRAM是否修改 */
    if (0 == oal_memcmp(puc_cust_nvram_info, g_auc_cust_nvram_info, OAL_SIZEOF(g_auc_cust_nvram_info)))
    {
        OS_MEM_KFREE(puc_cust_nvram_info);
        return INI_FILE_TIMESPEC_UNRECONFIG;
    }

    oal_memcopy(g_auc_cust_nvram_info, puc_cust_nvram_info, OAL_SIZEOF(g_auc_cust_nvram_info));
    OS_MEM_KFREE(puc_cust_nvram_info);

    return INI_NVRAM_RECONFIG;
}


oal_uint8 *hwifi_get_nvram_param(oal_uint32 ul_nvram_param_idx)
{
    return g_auc_cust_nvram_info[ul_nvram_param_idx];
}


int32 hwifi_custom_host_read_cfg_init(void)
{
    oal_int32      l_nv_read_ret;
    oal_int32      l_ini_read_ret;

    /* 先获取私有定制化项 */
    hwifi_config_init_etc(CUS_TAG_PRIV_INI);

    /* 读取nvram参数是否修改 */
    l_nv_read_ret  = hwifi_custom_host_read_dyn_cali_nvram();
    /* 检查定制化文件中的产线配置是否修改 */
    l_ini_read_ret =  ini_file_check_conf_update();
    if (l_ini_read_ret ||  l_nv_read_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "hwifi_custom_host_read_cfg_init config is updated");
        hwifi_config_init_etc(CUS_TAG_PRO_LINE_INI);
    }

    if (INI_FILE_TIMESPEC_UNRECONFIG == l_ini_read_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "hwifi_custom_host_read_cfg_init file is not updated");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    hwifi_config_init_etc(CUS_TAG_DTS);
    l_ini_read_ret = hwifi_config_init_etc(CUS_TAG_NV);
    if (OAL_UNLIKELY(OAL_SUCC != l_ini_read_ret))
    {
        OAL_IO_PRINT("hwifi_custom_host_read_cfg_init NV fail l_ret[%d].\r\n", l_ini_read_ret);
    }

    hwifi_config_init_etc(CUS_TAG_INI);

    /*启动完成后，输出打印*/
    OAL_IO_PRINT("hwifi_custom_host_read_cfg_init finish!\r\n");

    return OAL_SUCC;
}


int32 hwifi_hcc_customize_h2d_data_cfg(void)
{
    oal_int32                              l_ret;

    /* wifi上电时重读定制化配置 */
    l_ret = hwifi_custom_host_read_cfg_init();
    if (OAL_SUCC != l_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "hwifi_hcc_customize_h2d_data_cfg data ret[%d]", l_ret);
    }

    //如果不成功，返回失败
    l_ret = hwifi_hcc_custom_ini_data_buf(CUSTOM_CFGID_PRIV_INI_ID);
    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "hwifi_hcc_customize_h2d_data_cfg priv data fail, ret[%d]", l_ret);
        return OAL_FAIL;
    }

    l_ret = hwifi_hcc_custom_ini_data_buf(CUSTOM_CFGID_INI_ID);
    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "hwifi_hcc_customize_h2d_data_cfg ini data fail, ret[%d]", l_ret);
        return OAL_FAIL;
    }

    return INI_SUCC;
}


OAL_STATIC int32 hwifi_config_init_fcc_txpwr_nvram(void)
{
    int32     l_ret = INI_FAILED;
    uint8     uc_cfg_id;
    uint8     uc_param_idx = 0;
    int32    *pl_nvram_params = OAL_PTR_NULL;
    int32    *pl_fcc_txpwr_limit_params = OAL_PTR_NULL;
    oal_uint8 uc_param_len = (NVRAM_PARAMS_FCC_END_INDEX_BUTT - NVRAM_PARAMS_FCC_START_INDEX_BUTT) * OAL_SIZEOF(int32);

    pl_fcc_txpwr_limit_params = (int32 *)OS_KZALLOC_GFP(uc_param_len);
    if (OAL_PTR_NULL == pl_fcc_txpwr_limit_params)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CUSTOM, "hwifi_config_init_fcc_txpwr_nvram::pl_nvram_params mem alloc fail!");
        return INI_FAILED;
    }
    OAL_MEMZERO(pl_fcc_txpwr_limit_params, uc_param_len);
    pl_nvram_params = pl_fcc_txpwr_limit_params;

    for (uc_cfg_id = NVRAM_PARAMS_FCC_START_INDEX_BUTT; uc_cfg_id < NVRAM_PARAMS_FCC_END_INDEX_BUTT; uc_cfg_id++)
    {
        l_ret = get_cust_conf_int32_etc(INI_MODU_WIFI, g_ast_nvram_config_ini[uc_cfg_id].name, pl_nvram_params+uc_param_idx);
        OAL_IO_PRINT("{hwifi_config_init_fcc_txpwr_nvram params[%d]=0x%x!}", uc_param_idx, pl_nvram_params[uc_param_idx]);

        if(INI_SUCC != l_ret)
        {
            OAM_WARNING_LOG1(0, OAM_SF_CFG, "hwifi_config_init_nvram read id[%d] from ini failed!", uc_cfg_id);
            /* 读取失败时,使用初始值 */
            pl_nvram_params[uc_param_idx] = g_al_nvram_init_params[uc_cfg_id];
        }
        uc_param_idx++;
    }
    /* 5g */
    oal_memcopy(g_st_cust_nv_params.auc_5g_fcc_txpwr_limit_params_20M, pl_nvram_params, OAL_SIZEOF(int32));
    pl_nvram_params++;
    oal_memcopy(g_st_cust_nv_params.auc_5g_fcc_txpwr_limit_params_20M+OAL_SIZEOF(int32), pl_nvram_params, 2*OAL_SIZEOF(oal_uint8));
    pl_nvram_params++;
    oal_memcopy(g_st_cust_nv_params.auc_5g_fcc_txpwr_limit_params_40M, pl_nvram_params, OAL_SIZEOF(int32));
    pl_nvram_params++;
    oal_memcopy(g_st_cust_nv_params.auc_5g_fcc_txpwr_limit_params_40M+OAL_SIZEOF(int32), pl_nvram_params, 2*OAL_SIZEOF(oal_uint8));
    pl_nvram_params++;
    oal_memcopy(g_st_cust_nv_params.auc_5g_fcc_txpwr_limit_params_80M, pl_nvram_params, OAL_SIZEOF(int32));
    pl_nvram_params++;
    oal_memcopy(g_st_cust_nv_params.auc_5g_fcc_txpwr_limit_params_80M+OAL_SIZEOF(int32), pl_nvram_params, OAL_SIZEOF(oal_uint8));
    pl_nvram_params++;
    oal_memcopy(g_st_cust_nv_params.auc_5g_fcc_txpwr_limit_params_160M, pl_nvram_params, CUS_NUM_5G_160M_SIDE_BAND*OAL_SIZEOF(oal_uint8));
    /* 2.4g */
    for (uc_cfg_id = 0; uc_cfg_id < MAC_2G_CHANNEL_NUM; uc_cfg_id++)
    {
        pl_nvram_params++;
        oal_memcopy(g_st_cust_nv_params.auc_2g_fcc_txpwr_limit_params[uc_cfg_id], pl_nvram_params, CUS_NUM_FCC_2G_PRO*OAL_SIZEOF(oal_uint8));
    }

    OS_MEM_KFREE(pl_fcc_txpwr_limit_params);
    return INI_SUCC;
}


OAL_STATIC oal_int32 hwifi_config_init_sar_ctrl_nvram(void)
{
    oal_int32     l_ret = INI_FAILED;
    oal_uint8     uc_cfg_id;
    oal_uint8     uc_sar_lvl_idx;
    oal_uint8     uc_sar_params_idx = 0;
    oal_uint32    ul_nvram_params = 0;
    oal_uint8     auc_nvram_params[CUS_NUM_OF_SAR_LVL] = {0};

    for (uc_cfg_id = NVRAM_PARAMS_SAR_START_INDEX_BUTT; uc_cfg_id < NVRAM_PARAMS_SAR_END_INDEX_BUTT; uc_cfg_id++)
    {
        l_ret = get_cust_conf_int32_etc(INI_MODU_WIFI, g_ast_nvram_config_ini[uc_cfg_id].name, &ul_nvram_params);
        OAL_IO_PRINT("{hwifi_config_init_sar_ctrl_nvram params[%d]=0x%x!}", uc_cfg_id, ul_nvram_params);

        if(INI_SUCC != l_ret)
        {
            OAM_WARNING_LOG1(0, OAM_SF_CFG, "hwifi_config_init_sar_ctrl_nvram read id[%d] from ini failed!", uc_cfg_id);
            /* 读取失败时,使用初始值 */
            ul_nvram_params = g_al_nvram_init_params[uc_cfg_id];
        }

        oal_memcopy(auc_nvram_params, &ul_nvram_params, OAL_SIZEOF(auc_nvram_params));

        for (uc_sar_lvl_idx = 0; uc_sar_lvl_idx < CUS_NUM_OF_SAR_LVL; uc_sar_lvl_idx++)
        {
            if (auc_nvram_params[uc_sar_lvl_idx] <= CUS_MIN_OF_SAR_VAL)
            {
                OAM_ERROR_LOG4(0, OAM_SF_CUSTOM, "hwifi_config_init_sar_ctrl_nvram::uc_cfg_id[%d]:0x%x got[%d] out of the normal[%d] check ini file!",
                               uc_cfg_id, ul_nvram_params, auc_nvram_params[uc_sar_lvl_idx], CUS_MIN_OF_SAR_VAL);
                auc_nvram_params[uc_sar_lvl_idx] = 0xFF;
            }
            g_st_cust_nv_params.auc_sar_ctrl_params[uc_sar_lvl_idx][uc_sar_params_idx] = auc_nvram_params[uc_sar_lvl_idx];
        }
        uc_sar_params_idx++;
    }

    return INI_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH

OAL_STATIC oal_int32 hwifi_config_init_tas_ctrl_nvram(oal_void)
{
    oal_int32     l_ret = INI_FAILED;
    oal_uint8     uc_band_idx;
    oal_uint8     uc_rf_idx;
    oal_uint32    ul_nvram_params = 0;
    oal_int8      ac_tas_ctrl_params[WLAN_BAND_BUTT][WLAN_RF_CHANNEL_NUMS] = {{0}};

    l_ret = get_cust_conf_int32_etc(INI_MODU_WIFI, g_ast_nvram_config_ini[NVRAM_PARAMS_TAS_ANT_SWITCH_EN].name, &ul_nvram_params);
    if(INI_SUCC == l_ret)
    {
        g_en_tas_switch_en = (oal_bool_enum_uint8)ul_nvram_params;
        OAM_WARNING_LOG1(0, OAM_SF_CUSTOM, "hwifi_config_init_tas_ctrl_nvram g_en_tas_switch_en[%d]!", g_en_tas_switch_en);
    }

    l_ret = get_cust_conf_int32_etc(INI_MODU_WIFI, g_ast_nvram_config_ini[NVRAM_PARAMS_TAS_PWR_CTRL].name, &ul_nvram_params);
    OAL_IO_PRINT("{hwifi_config_init_tas_ctrl_nvram params[%d]=0x%x!}", NVRAM_PARAMS_TAS_PWR_CTRL, ul_nvram_params);

    if(INI_SUCC != l_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_CUSTOM, "hwifi_config_init_tas_ctrl_nvram read id[%d] from ini failed!", NVRAM_PARAMS_TAS_PWR_CTRL);
        /* 读取失败时,使用初始值 */
        ul_nvram_params = g_al_nvram_init_params[NVRAM_PARAMS_TAS_PWR_CTRL];
    }

    oal_memcopy(ac_tas_ctrl_params, &ul_nvram_params, OAL_SIZEOF(ac_tas_ctrl_params));
    for (uc_band_idx = 0; uc_band_idx < WLAN_BAND_BUTT; uc_band_idx++)
    {
        for (uc_rf_idx = 0; uc_rf_idx < WLAN_RF_CHANNEL_NUMS; uc_rf_idx++)
        {
            if ((ac_tas_ctrl_params[uc_band_idx][uc_rf_idx] > CUS_MAX_OF_TAS_PWR_CTRL_VAL) || (ac_tas_ctrl_params[uc_rf_idx][uc_rf_idx] < 0))
            {
                OAM_ERROR_LOG4(0, OAM_SF_CUSTOM,
                               "hwifi_config_init_tas_ctrl_nvram::band[%d] rf[%d] ul_nvram_params[%d],val[%d] out of the normal check ini file!",
                               uc_band_idx, uc_rf_idx, ul_nvram_params, ac_tas_ctrl_params[uc_rf_idx][uc_rf_idx]);
                ac_tas_ctrl_params[uc_band_idx][uc_rf_idx] = 0;
            }
            g_st_cust_nv_params.auc_tas_ctrl_params[uc_rf_idx][uc_band_idx] = (oal_uint8)ac_tas_ctrl_params[uc_band_idx][uc_rf_idx];
        }
    }

    return INI_SUCC;
}
#endif


OAL_STATIC oal_uint32 hwifi_config_sepa_coefficient_from_param(oal_uint8 *puc_cust_param_info, oal_int32 *pl_coe_params,
                                                               oal_uint16 *pus_param_num, oal_uint16 us_max_idx)
{
    oal_int8       *pc_token;
    oal_int8       *pc_ctx;
    oal_int8       *pc_end = ";";
    oal_int8       *pc_sep = ",";
    oal_uint16      us_param_num = 0;
    oal_uint8       auc_cust_param[CUS_PARAMS_LEN_MAX];

    oal_memcopy(auc_cust_param, puc_cust_param_info, OAL_STRLEN(puc_cust_param_info));
    pc_token = oal_strtok(auc_cust_param, pc_end, &pc_ctx);
    if (OAL_PTR_NULL == pc_token)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CUSTOM, "hwifi_config_sepa_coefficient_from_param read get null value check!");
        return OAL_PTR_NULL;
    }
    pc_token = oal_strtok(pc_token, pc_sep, &pc_ctx);
    /* 获取定制化系数 */
    while (pc_token)
    {
        if (us_param_num == us_max_idx)
        {
            OAM_ERROR_LOG2(0, OAM_SF_CUSTOM, "hwifi_config_sepa_coefficient_from_param::nv or ini param is too many idx[%d] Max[%d]",
                            us_param_num, us_max_idx);
            return OAL_FAIL;
        }
        //OAL_IO_PRINT( "hwifi_config_sepa_coefficient_from_param get [%s]\n!", pc_token);
        *(pl_coe_params + us_param_num) = (oal_int32)oal_strtol(pc_token, OAL_PTR_NULL, 10);
        pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
        us_param_num++;
    }

    *pus_param_num = us_param_num;
    return OAL_SUCC;
}


oal_void hwifi_get_max_txpwr_base(oal_int32 l_plat_tag, oal_uint8 uc_nvram_base_param_idx, oal_uint8 *puc_txpwr_base_params, oal_uint8 uc_param_num)
{
    oal_uint8       uc_param_idx;
    oal_int32       l_ret               = INI_FAILED;
    oal_uint8      *puc_base_pwr_params = OAL_PTR_NULL;
    oal_uint16      us_per_param_num = 0;
    oal_int32       l_nv_params[DY_CALI_NUM_5G_BAND] = {0};

    puc_base_pwr_params = (oal_uint8 *)OS_KZALLOC_GFP(CUS_PARAMS_LEN_MAX * OAL_SIZEOF(oal_uint8));
    if (OAL_PTR_NULL == puc_base_pwr_params)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CUSTOM, "hwifi_get_max_txpwr_base::puc_base_pwr_params mem alloc fail!");
        l_ret  = INI_FAILED;
    }
    else
    {
        OAL_MEMZERO(puc_base_pwr_params, CUS_PARAMS_LEN_MAX * OAL_SIZEOF(oal_uint8));
        l_ret = get_cust_conf_string_etc(l_plat_tag, g_ast_nvram_config_ini[uc_nvram_base_param_idx].name, puc_base_pwr_params, CUS_PARAMS_LEN_MAX-1);
        if (INI_SUCC == l_ret)
        {
            if ((OAL_SUCC == hwifi_config_sepa_coefficient_from_param(puc_base_pwr_params, l_nv_params, &us_per_param_num, uc_param_num)) &&
                (us_per_param_num == uc_param_num))
            {
                /* 参数合理性检查 */
                for (uc_param_idx = 0; uc_param_idx < uc_param_num; uc_param_idx++)
                {
                    if ((l_nv_params[uc_param_idx] < CUS_MIN_BASE_TXPOWER_VAL) || (l_nv_params[uc_param_idx] > CUS_MAX_BASE_TXPOWER_VAL))
                    {
                        OAM_ERROR_LOG3(0, OAM_SF_CUSTOM, "hwifi_get_max_txpwr_base read %dth from ini val[%d] out of range replaced by [%d]!",
                                       uc_nvram_base_param_idx, l_nv_params[uc_param_idx], CUS_MAX_BASE_TXPOWER_VAL);
                        *(puc_txpwr_base_params + uc_param_idx) = CUS_MAX_BASE_TXPOWER_VAL;
                    }
                    else
                    {
                        *(puc_txpwr_base_params + uc_param_idx) = (oal_uint8)l_nv_params[uc_param_idx];
                    }
                }
                OS_MEM_KFREE(puc_base_pwr_params);
                return;
            }
        }
        else
        {
            OAM_ERROR_LOG3(0, OAM_SF_CUSTOM, "hwifi_get_max_txpwr_base::l_plat_tag[%d] read %dth failed ret[%d] check ini files!",
                           l_plat_tag, uc_nvram_base_param_idx, l_ret);
        }
    }

    if (INI_SUCC != l_ret)
    {
        /* 失败默认使用初始值 */
        for (uc_param_idx = 0; uc_param_idx < uc_param_num; uc_param_idx++)
        {
            *(puc_txpwr_base_params + uc_param_idx) = CUS_MAX_BASE_TXPOWER_VAL;
        }

        OAM_ERROR_LOG2(0, OAM_SF_CFG, "hwifi_get_max_txpwr_base read failed ret[%d] replaced by ini_val[%d]!", l_ret, CUS_MAX_BASE_TXPOWER_VAL);
    }
    OS_MEM_KFREE(puc_base_pwr_params);
    return;
}


OAL_STATIC int32 hwifi_config_init_nvram(void)
{
    int32     l_ret = INI_FAILED;
    int32     l_cfg_id;
    int32     al_nvram_params[NVRAM_PARAMS_TXPWR_INDEX_BUTT] = {0};
    int32     l_val = 0xFF;

    OAL_MEMZERO(&g_st_cust_nv_params, OAL_SIZEOF(g_st_cust_nv_params));

    /* read nvm failed or data not exist or country_code updated, read ini:cust_spec > cust_common > default */
    /* find plat tag */
    for (l_cfg_id = NVRAM_PARAMS_INDEX_0; l_cfg_id < NVRAM_PARAMS_TXPWR_INDEX_BUTT; l_cfg_id++)
    {
        l_ret = get_cust_conf_int32_etc(INI_MODU_WIFI, g_ast_nvram_config_ini[l_cfg_id].name, &al_nvram_params[l_cfg_id]);
        OAM_INFO_LOG2(0, OAM_SF_CFG, "{hwifi_config_init_nvram aul_nvram_params[%d]=0x%x!}", l_cfg_id, al_nvram_params[l_cfg_id]);

        if(INI_SUCC != l_ret)
        {
            OAM_WARNING_LOG1(0, OAM_SF_CFG, "hwifi_config_init_nvram read id[%d] from ini failed!", l_cfg_id);

            /* 读取失败时,使用初始值 */
            al_nvram_params[l_cfg_id] = g_al_nvram_init_params[l_cfg_id];
        }
    }

    oal_memcopy(g_st_cust_nv_params.ac_delt_txpwr_params, al_nvram_params, NUM_OF_NV_MAX_TXPOWER);
    oal_memcopy(g_st_cust_nv_params.ac_dpd_delt_txpwr_params, al_nvram_params + NVRAM_PARAMS_INDEX_DPD_0, NUM_OF_NV_DPD_MAX_TXPOWER);
    oal_memcopy(g_st_cust_nv_params.ac_11b_delt_txpwr_params, al_nvram_params + NVRAM_PARAMS_INDEX_62, NUM_OF_NV_11B_DELTA_TXPOWER);
    oal_memcopy(g_st_cust_nv_params.auc_5g_upper_upc_params, al_nvram_params + NVRAM_PARAMS_INDEX_63, NUM_OF_NV_5G_UPPER_UPC);

    /* 基准功率 */
    hwifi_get_max_txpwr_base(INI_MODU_WIFI, NVRAM_PARAMS_INDEX_19, g_st_cust_nv_params.auc_2g_txpwr_base_params[WLAN_RF_CHANNEL_ZERO], CUS_BASE_PWR_NUM_2G);
    hwifi_get_max_txpwr_base(INI_MODU_WIFI, NVRAM_PARAMS_INDEX_20, g_st_cust_nv_params.auc_5g_txpwr_base_params[WLAN_RF_CHANNEL_ZERO], CUS_BASE_PWR_NUM_5G);
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST)
    hwifi_get_max_txpwr_base(INI_MODU_WIFI, NVRAM_PARAMS_INDEX_21, g_st_cust_nv_params.auc_2g_txpwr_base_params[WLAN_RF_CHANNEL_ONE], CUS_BASE_PWR_NUM_2G);
    hwifi_get_max_txpwr_base(INI_MODU_WIFI, NVRAM_PARAMS_INDEX_22, g_st_cust_nv_params.auc_5g_txpwr_base_params[WLAN_RF_CHANNEL_ONE], CUS_BASE_PWR_NUM_5G);
#endif

    /* FCC */
    hwifi_config_init_fcc_txpwr_nvram();
    /* SAR */
    hwifi_config_init_sar_ctrl_nvram();
#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH
    /* TAS */
    hwifi_config_init_tas_ctrl_nvram();
#endif

    if(INI_SUCC == get_cust_conf_int32_etc(INI_MODU_WIFI, g_ast_nvram_config_ini[NVRAM_PARAMS_5G_CE_HIGH_BAND_MAX_PWR].name, &l_val))
    {
        if ((l_val < CUS_MIN_BASE_TXPOWER_VAL) || l_val > 0xFF)
        {
            OAM_ERROR_LOG1(0, OAM_SF_CFG, "hwifi_config_init_nvram read 5G_CE_HIGH_BAND_MAX_PWR[%d] failed!", l_val);
            l_val = 0xFF;
        }
    }
    else
    {
        /* 读取失败时,使用初始值 */
        l_val = g_al_nvram_init_params[NVRAM_PARAMS_5G_CE_HIGH_BAND_MAX_PWR];
    }
    g_st_cust_nv_params.uc_5g_max_pwr_for_ce_high_band = (oal_uint8)l_val;

    OAM_INFO_LOG0(0, OAM_SF_CFG, "hwifi_config_init_nvram read from ini success!");
    return INI_SUCC;
}


OAL_STATIC int32 hwifi_config_init_private_custom(void)
{
    int32               l_cfg_id;
    int32               l_ret = INI_FAILED;

    for(l_cfg_id = 0; l_cfg_id < WLAN_CFG_PRIV_BUTT; l_cfg_id++)
    {
        /* 获取 private 的配置值 */
        l_ret = get_cust_conf_int32_etc(INI_MODU_WIFI, g_ast_wifi_config_priv[l_cfg_id].name, &(g_al_priv_cust_params[l_cfg_id].l_val));

        if (INI_FAILED == l_ret)
        {
            g_al_priv_cust_params[l_cfg_id].en_value_state = OAL_FALSE;
            continue;
        }

        g_al_priv_cust_params[l_cfg_id].en_value_state = OAL_TRUE;
        /*
        OAM_WARNING_LOG2(0, OAM_SF_CFG, "hwifi_config_init_private_custom got priv_var id[%d]val[%d]!", l_cfg_id, g_al_priv_cust_params[l_cfg_id].l_val);
        */
    }

    OAM_WARNING_LOG0(0, OAM_SF_CFG, "hwifi_config_init_private_custom read from ini success!");

    return INI_SUCC;
}


OAL_STATIC oal_void hwifi_config_get_5g_curv_switch_point(oal_uint8 *puc_ini_pa_params, oal_uint32 ul_cfg_id)
{
    oal_int32        l_ini_params[CUS_NUM_5G_BW*DY_CALI_PARAMS_TIMES] = {0};
    oal_uint16       us_ini_param_num = 0;
    oal_uint8        uc_secon_ratio_idx = 0;
    oal_uint8        uc_param_idx;
    oal_uint8        uc_chain_idx;
    oal_int16       *ps_extre_point_val;

    if ((WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0 == ul_cfg_id) || (WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0_BAND1 == ul_cfg_id))
    {
        uc_chain_idx = WLAN_RF_CHANNEL_ZERO;
    }
    else if ((WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1 == ul_cfg_id) || (WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1_BAND1 == ul_cfg_id))
    {
        uc_chain_idx = WLAN_RF_CHANNEL_ONE;
    }
    else
    {
        return;
    }

    /* 获取拟合系数项 */
    if (OAL_SUCC != hwifi_config_sepa_coefficient_from_param(puc_ini_pa_params, l_ini_params, &us_ini_param_num, OAL_SIZEOF(l_ini_params)/OAL_SIZEOF(oal_int32)) ||
        (us_ini_param_num % DY_CALI_PARAMS_TIMES))
    {
        OAM_ERROR_LOG2(0, OAM_SF_CUSTOM, "hwifi_config_get_5g_curv_switch_point::ini is unsuitable,num of ini[%d] cfg_id[%d]!", us_ini_param_num, ul_cfg_id);
        return;
    }

    ps_extre_point_val = gs_extre_point_vals[uc_chain_idx];
    us_ini_param_num /= DY_CALI_PARAMS_TIMES;
    if (ul_cfg_id <= WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1)
    {
        if (us_ini_param_num != CUS_NUM_5G_BW)
        {
            OAM_ERROR_LOG2(0, OAM_SF_CUSTOM, "hwifi_config_get_5g_curv_switch_point::ul_cfg_id[%d] us_ini_param_num[%d]", ul_cfg_id, us_ini_param_num);
            return;
        }
        ps_extre_point_val++;
    }
    else
    {
        if (us_ini_param_num != 1)
        {
            OAM_ERROR_LOG2(0, OAM_SF_CUSTOM, "hwifi_config_get_5g_curv_switch_point::ul_cfg_id[%d] us_ini_param_num[%d]", ul_cfg_id, us_ini_param_num);
            return;
        }
    }

    /* 计算5g曲线switch point */
    for (uc_param_idx = 0; uc_param_idx < us_ini_param_num; uc_param_idx++)
    {
        *(ps_extre_point_val + uc_param_idx) = (oal_int16)HWIFI_DYN_CALI_GET_EXTRE_POINT(l_ini_params+uc_secon_ratio_idx);
        OAL_IO_PRINT("hwifi_config_get_5g_curv_switch_point::extre power[%d] param_idx[%d] cfg_id[%d]!\r\n",
                      *(ps_extre_point_val + uc_param_idx), uc_param_idx, ul_cfg_id);
        OAL_IO_PRINT("hwifi_config_get_5g_curv_switch_point::param[%d %d] uc_secon_ratio_idx[%d]!\r\n",
                      (l_ini_params+uc_secon_ratio_idx)[0], (l_ini_params+uc_secon_ratio_idx)[1], uc_secon_ratio_idx);
        uc_secon_ratio_idx += DY_CALI_PARAMS_TIMES;
    }

    return;
}


OAL_STATIC oal_uint32 hwifi_config_nvram_second_coefficient_check(oal_uint8 *puc_cust_nvram_info, oal_uint8 *puc_ini_pa_params,
                                                                  oal_uint32 ul_cfg_id, oal_int16 *ps_5g_delt_power)
{
    oal_int32        l_ini_params[CUS_NUM_5G_BW*DY_CALI_PARAMS_TIMES] = {0};
    oal_int32        l_nv_params[CUS_NUM_5G_BW*DY_CALI_PARAMS_TIMES] = {0};
    oal_uint16       us_ini_param_num = 0;
    oal_uint16       us_nv_param_num = 0;
    oal_uint8        uc_secon_ratio_idx = 0;
    oal_uint8        uc_param_idx;

    /* 获取拟合系数项 */
    if (OAL_SUCC != hwifi_config_sepa_coefficient_from_param(puc_cust_nvram_info, l_nv_params, &us_nv_param_num, OAL_SIZEOF(l_nv_params)/OAL_SIZEOF(oal_int16)) ||
        (us_nv_param_num % DY_CALI_PARAMS_TIMES) ||
         OAL_SUCC != hwifi_config_sepa_coefficient_from_param(puc_ini_pa_params, l_ini_params, &us_ini_param_num, OAL_SIZEOF(l_ini_params)/OAL_SIZEOF(oal_int16)) ||
        (us_ini_param_num % DY_CALI_PARAMS_TIMES) ||
        (us_nv_param_num != us_ini_param_num))
    {
        OAM_ERROR_LOG2(0, OAM_SF_CUSTOM, "hwifi_config_nvram_second_coefficient_check::nvram or ini is unsuitable,num of nv and ini[%d %d]!",
                       us_nv_param_num, us_ini_param_num);
        return OAL_FAIL;
    }

    us_nv_param_num /= DY_CALI_PARAMS_TIMES;
    /* 检查nv和ini中二次系数是否匹配 */
    for (uc_param_idx = 0; uc_param_idx < us_nv_param_num; uc_param_idx++)
    {
        if (l_ini_params[uc_secon_ratio_idx] != l_nv_params[uc_secon_ratio_idx])
        {
            OAM_WARNING_LOG4(0, OAM_SF_CUSTOM, "hwifi_config_nvram_second_coefficient_check::nvram get mismatch value idx[%d %d] val are [%d] and [%d]!",
                           uc_param_idx, uc_secon_ratio_idx, l_ini_params[uc_secon_ratio_idx], l_nv_params[uc_secon_ratio_idx]);

            /* 量产后二次系数以nvram中为准，刷新NV中的二次拟合曲线切换点 */
            hwifi_config_get_5g_curv_switch_point(puc_cust_nvram_info, ul_cfg_id);
            uc_secon_ratio_idx += DY_CALI_PARAMS_TIMES;
            continue;
        }

        if ((WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0 == ul_cfg_id) || (WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1 == ul_cfg_id))
        {
            /* 计算产线上的delt power */
            *(ps_5g_delt_power + uc_param_idx) = HWIFI_GET_5G_PRO_LINE_DELT_POW_PER_BAND(l_nv_params+uc_secon_ratio_idx, l_ini_params+uc_secon_ratio_idx);
            OAL_IO_PRINT("hwifi_config_nvram_second_coefficient_check::delt power[%d] param_idx[%d] cfg_id[%d]!\r\n",
                          *(ps_5g_delt_power + uc_param_idx), uc_param_idx, ul_cfg_id);
        }
        uc_secon_ratio_idx += DY_CALI_PARAMS_TIMES;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 hwifi_config_init_dy_cali_custom(oal_void)
{
    oal_uint32            ul_cfg_id;
    oal_uint32            ul_ret       = OAL_SUCC;
    oal_uint8             uc_idx       = 0;
    oal_uint16            us_param_num = 0;
    oal_uint16            us_per_param_num = 0;
    oal_uint8             uc_rf_idx;
    oal_uint8             uc_cali_param_idx;
    oal_int16             s_5g_delt_power[WLAN_RF_CHANNEL_NUMS][CUS_NUM_5G_BW] = {{0}};
    oal_uint8             uc_delt_pwr_idx = 0;
    oal_uint32            ul_cfg_id_tmp;
    oal_uint8            *puc_cust_nvram_info = OAL_PTR_NULL;
    oal_uint8            *puc_nv_pa_params    = OAL_PTR_NULL;
    oal_int32            *pl_params           = OAL_PTR_NULL;
    oal_uint16            us_param_len        = WLAN_RF_CHANNEL_NUMS*DY_CALI_PARAMS_TIMES*DY_CALI_PARAMS_NUM*OAL_SIZEOF(oal_int32);

    puc_nv_pa_params = (oal_uint8 *)OS_KZALLOC_GFP(CUS_PARAMS_LEN_MAX * OAL_SIZEOF(oal_uint8));
    if (OAL_PTR_NULL == puc_nv_pa_params)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CUSTOM, "hwifi_config_init_dy_cali_custom::puc_nv_pa_params mem alloc fail!");
        return OAL_FAIL;
    }

    pl_params = (oal_int32 *)OS_KZALLOC_GFP(us_param_len);
    if (OAL_PTR_NULL == pl_params)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CUSTOM, "hwifi_config_init_dy_cali_custom::ps_params mem alloc fail!");
        OS_MEM_KFREE(puc_nv_pa_params);
        return OAL_FAIL;
    }

    OAL_MEMZERO(puc_nv_pa_params, CUS_PARAMS_LEN_MAX * OAL_SIZEOF(oal_uint8));
    OAL_MEMZERO(pl_params, us_param_len);

    for (ul_cfg_id = WLAN_CFG_DTS_NVRAM_RATIO_PA2GCCKA0; ul_cfg_id < WLAN_CFG_DTS_NVRAM_PARAMS_BUTT; ul_cfg_id++)
    {
        /* 二次拟合系数 */
        if ((ul_cfg_id >= WLAN_CFG_DTS_NVRAM_MUFREQ_2GCCK_C0) && (ul_cfg_id < WLAN_CFG_DTS_NVRAM_END))
        {
            /* DPN */
            continue;
        }

        if(INI_FAILED == get_cust_conf_string_etc(INI_MODU_WIFI, g_ast_nvram_pro_line_config_ini[ul_cfg_id].name, puc_nv_pa_params, CUS_PARAMS_LEN_MAX-1))
        {
            if ((ul_cfg_id == WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0_LOW) || (ul_cfg_id == WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1_LOW)
                 ||(ul_cfg_id == WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0_BAND1_LOW) || (ul_cfg_id == WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1_BAND1_LOW))
            {
                ul_cfg_id_tmp = ((ul_cfg_id == WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0_BAND1_LOW) ? WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0_BAND1 :
                                 (ul_cfg_id == WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0_LOW) ? WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0 :
                                 (ul_cfg_id == WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1_BAND1_LOW) ? WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1_BAND1 :
                                 (ul_cfg_id == WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1_LOW) ? WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1 : ul_cfg_id);
                get_cust_conf_string_etc(INI_MODU_WIFI, g_ast_nvram_pro_line_config_ini[ul_cfg_id_tmp].name, puc_nv_pa_params, CUS_PARAMS_LEN_MAX-1);
            }
            else
            {
                OAM_ERROR_LOG1(0, OAM_SF_CUSTOM, "hwifi_config_init_dy_cali_custom read, check id[%d] exists!", ul_cfg_id);
                ul_ret = OAL_FAIL;
                break;
            }
        }

        /* 获取ini中的二次拟合曲线切换点 */
        hwifi_config_get_5g_curv_switch_point(puc_nv_pa_params, ul_cfg_id);

        if (WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1_LOW >= ul_cfg_id)
        {
            puc_cust_nvram_info = hwifi_get_nvram_param(ul_cfg_id);
            /* 先取nv中的参数值,为空则从ini文件中读取 */
            if (OAL_STRLEN(puc_cust_nvram_info))
            {
                /* NVRAM二次系数异常保护 */
                if (OAL_SUCC == hwifi_config_nvram_second_coefficient_check(puc_cust_nvram_info, puc_nv_pa_params, ul_cfg_id,
                                                                            s_5g_delt_power[ul_cfg_id < WLAN_CFG_DTS_NVRAM_RATIO_PA2GCCKA1 ? WLAN_RF_CHANNEL_ZERO: WLAN_RF_CHANNEL_ONE]))
                {
                    /* 手机如果low part为空,则取ini中的系数,并根据产测结果修正;否则直接从nvram中取得 */
                    if ((WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0_LOW == ul_cfg_id) && (oal_memcmp(puc_cust_nvram_info, puc_nv_pa_params, OAL_STRLEN(puc_cust_nvram_info))))
                    {
                        OAL_MEMZERO(s_5g_delt_power[WLAN_RF_CHANNEL_ZERO], CUS_NUM_5G_BW*OAL_SIZEOF(oal_int16));
                    }
                    if ((WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1_LOW == ul_cfg_id) && (oal_memcmp(puc_cust_nvram_info, puc_nv_pa_params, OAL_STRLEN(puc_cust_nvram_info))))
                    {
                        OAL_MEMZERO(s_5g_delt_power[WLAN_RF_CHANNEL_ONE], CUS_NUM_5G_BW*OAL_SIZEOF(oal_int16));
                    }

                    oal_memcopy(puc_nv_pa_params, puc_cust_nvram_info, OAL_STRLEN(puc_cust_nvram_info));
                }
                else
                {
                    ul_ret = OAL_FAIL;
                    break;
                }
            }
            else
            {
                /* 提供产线第一次上电校准初始值 */
                oal_memcopy(puc_cust_nvram_info, puc_nv_pa_params, OAL_STRLEN(puc_nv_pa_params));
            }
        }

        if (OAL_SUCC != hwifi_config_sepa_coefficient_from_param(puc_nv_pa_params, pl_params + us_param_num, &us_per_param_num, us_param_len-us_param_num) ||
            (us_per_param_num % DY_CALI_PARAMS_TIMES))
        {
            ul_ret = OAL_FAIL;
            OAM_ERROR_LOG3(0, OAM_SF_CUSTOM, "hwifi_config_init_dy_cali_custom read get wrong value,len[%d] check id[%d] exists us_per_param_num[%d]!",
                           OAL_STRLEN(puc_cust_nvram_info), ul_cfg_id, us_per_param_num);
            break;
        }
        us_param_num += us_per_param_num;
    }

    OS_MEM_KFREE(puc_nv_pa_params);

    if (OAL_FAIL == ul_ret)
    {
        /* 置零防止下发到device */
        OAL_MEMZERO(g_ast_pro_line_params, OAL_SIZEOF(g_ast_pro_line_params));
    }
    else
    {
        if (us_param_num != us_param_len/OAL_SIZEOF(oal_int32))
        {
            OAM_ERROR_LOG1(0, OAM_SF_CUSTOM, "hwifi_config_init_dy_cali_custom read get wrong ini value num[%d]!", us_param_num);
            OAL_MEMZERO(g_ast_pro_line_params, OAL_SIZEOF(g_ast_pro_line_params));
            OS_MEM_KFREE(pl_params);
            return OAL_FAIL;
        }
        for (uc_rf_idx = 0; uc_rf_idx < WLAN_RF_CHANNEL_NUMS; uc_rf_idx++)
        {
            for (uc_cali_param_idx = 0; uc_cali_param_idx < DY_CALI_PARAMS_BASE_NUM; uc_cali_param_idx++)
            {
                if (DY_2G_CALI_PARAMS_NUM-1 == uc_cali_param_idx)
                {
                    /* band1 & CW */
                    uc_cali_param_idx+=2;
                }
                g_ast_pro_line_params[uc_rf_idx][uc_cali_param_idx].l_pow_par2 = pl_params[uc_idx++];
                g_ast_pro_line_params[uc_rf_idx][uc_cali_param_idx].l_pow_par1 = pl_params[uc_idx++];
                g_ast_pro_line_params[uc_rf_idx][uc_cali_param_idx].l_pow_par0 = pl_params[uc_idx++];
            }
        }

        /* 5g band2&3 4&5 6 7 low power */
        for (uc_rf_idx = 0; uc_rf_idx < WLAN_RF_CHANNEL_NUMS; uc_rf_idx++)
        {
            uc_delt_pwr_idx = 0;
            for (uc_cali_param_idx = DY_CALI_PARAMS_BASE_NUM+1; uc_cali_param_idx < DY_CALI_PARAMS_NUM - 1; uc_cali_param_idx++)
            {
                g_ast_pro_line_params[uc_rf_idx][uc_cali_param_idx].l_pow_par2 = pl_params[uc_idx++];
                g_ast_pro_line_params[uc_rf_idx][uc_cali_param_idx].l_pow_par1 = pl_params[uc_idx++];
                g_ast_pro_line_params[uc_rf_idx][uc_cali_param_idx].l_pow_par0 = pl_params[uc_idx++];

                CUS_FLUSH_NV_RATIO_BY_DELT_POW(g_ast_pro_line_params[uc_rf_idx][uc_cali_param_idx].l_pow_par2,
                                              g_ast_pro_line_params[uc_rf_idx][uc_cali_param_idx].l_pow_par1,
                                              g_ast_pro_line_params[uc_rf_idx][uc_cali_param_idx].l_pow_par0,
                                              s_5g_delt_power[uc_rf_idx][uc_delt_pwr_idx]);
                uc_delt_pwr_idx++;
            }
        }

        /* band1 & CW */
        for (uc_rf_idx = 0; uc_rf_idx < WLAN_RF_CHANNEL_NUMS; uc_rf_idx++)
        {
            g_ast_pro_line_params[uc_rf_idx][DY_2G_CALI_PARAMS_NUM].l_pow_par2 = pl_params[uc_idx++];
            g_ast_pro_line_params[uc_rf_idx][DY_2G_CALI_PARAMS_NUM].l_pow_par1 = pl_params[uc_idx++];
            g_ast_pro_line_params[uc_rf_idx][DY_2G_CALI_PARAMS_NUM].l_pow_par0 = pl_params[uc_idx++];
        }
        for (uc_rf_idx = 0; uc_rf_idx < WLAN_RF_CHANNEL_NUMS; uc_rf_idx++)
        {
            g_ast_pro_line_params[uc_rf_idx][DY_2G_CALI_PARAMS_NUM-1].l_pow_par2 = pl_params[uc_idx++];
            g_ast_pro_line_params[uc_rf_idx][DY_2G_CALI_PARAMS_NUM-1].l_pow_par1 = pl_params[uc_idx++];
            g_ast_pro_line_params[uc_rf_idx][DY_2G_CALI_PARAMS_NUM-1].l_pow_par0 = pl_params[uc_idx++];
        }
        for (uc_rf_idx = 0; uc_rf_idx < WLAN_RF_CHANNEL_NUMS; uc_rf_idx++)
        {
            /* 5g band1 low power */
            /* band1产线不校准 */
            g_ast_pro_line_params[uc_rf_idx][DY_CALI_PARAMS_BASE_NUM].l_pow_par2 = pl_params[uc_idx++];
            g_ast_pro_line_params[uc_rf_idx][DY_CALI_PARAMS_BASE_NUM].l_pow_par1 = pl_params[uc_idx++];
            g_ast_pro_line_params[uc_rf_idx][DY_CALI_PARAMS_BASE_NUM].l_pow_par0 = pl_params[uc_idx++];
        }

        for (uc_rf_idx = 0; uc_rf_idx < WLAN_RF_CHANNEL_NUMS; uc_rf_idx++)
        {
            /* 2g cw ppa */
            g_ast_pro_line_params[uc_rf_idx][DY_CALI_PARAMS_NUM-1].l_pow_par2 = pl_params[uc_idx++];
            g_ast_pro_line_params[uc_rf_idx][DY_CALI_PARAMS_NUM-1].l_pow_par1 = pl_params[uc_idx++];
            g_ast_pro_line_params[uc_rf_idx][DY_CALI_PARAMS_NUM-1].l_pow_par0 = pl_params[uc_idx++];
        }
    }

    OS_MEM_KFREE(pl_params);
    return ul_ret;
}


int32 hwifi_config_init_etc(int32 cus_tag)
{
    int32               l_cfg_id;
    int32               l_ret = INI_FAILED;
    int32               l_ori_val;
    wlan_cfg_cmd*       pgast_wifi_config;
    int32*              pgal_params;
    int32               l_cfg_value = 0;
    int32               l_wlan_cfg_butt;

    switch (cus_tag)
    {
        case CUS_TAG_NV:
            original_value_for_nvram_params();
            return hwifi_config_init_nvram();
        case CUS_TAG_INI:
            host_params_init_first();
            pgast_wifi_config = g_ast_wifi_config_cmds;
            pgal_params = g_al_host_init_params_etc;
            l_wlan_cfg_butt = WLAN_CFG_INIT_BUTT;
            break;
        case CUS_TAG_DTS:
            original_value_for_dts_params();
            pgast_wifi_config = g_ast_wifi_config_dts;
            pgal_params = g_al_dts_params_etc;
            l_wlan_cfg_butt = WLAN_CFG_DTS_BUTT;
            break;
        case CUS_TAG_PRIV_INI:
            return hwifi_config_init_private_custom();
        case CUS_TAG_PRO_LINE_INI:
            return hwifi_config_init_dy_cali_custom();
        default:
            OAM_ERROR_LOG1(0, OAM_SF_CUSTOM, "hwifi_config_init_etc tag number[0x%x] not correct!", cus_tag);
            return INI_FAILED;
    }

    for(l_cfg_id = 0; l_cfg_id < l_wlan_cfg_butt; l_cfg_id++)
    {
        /* 获取ini的配置值 */
        l_ret = get_cust_conf_int32_etc(INI_MODU_WIFI, pgast_wifi_config[l_cfg_id].name, &l_cfg_value);

        if (INI_FAILED == l_ret)
        {
            OAM_WARNING_LOG2(0, OAM_SF_CUSTOM, "hwifi_config_init_etc read ini file cfg_id[%d]tag[%d] not exist!", l_cfg_id, cus_tag);
            continue;
        }
        l_ori_val = pgal_params[pgast_wifi_config[l_cfg_id].case_entry];
        pgal_params[pgast_wifi_config[l_cfg_id].case_entry] = l_cfg_value;

        //OAM_WARNING_LOG4(0, OAM_SF_CUSTOM, "hwifi_config_init_etc [id:%d tag:%d] changed from [%d]to[%d]",
                        //pgast_wifi_config[l_cfg_id].case_entry, cus_tag, l_ori_val, pgal_params[pgast_wifi_config[l_cfg_id].case_entry]);
    }

    return INI_SUCC;
}


OAL_STATIC int char2byte( char* strori, char* outbuf )
{
    int i = 0;
    int temp = 0;
    int sum = 0;

    for( i = 0; i < 12; i++ )
    {
        switch (strori[i]) {
            case '0' ... '9':
                temp = strori[i] - '0';
                break;

            case 'a' ... 'f':
                temp = (strori[i] - 'a') + 10;
                break;

            case 'A' ... 'F':
                temp = (strori[i] - 'A') + 10;
                break;
            default:
                break;
        }

        sum += temp;
        if( i % 2 == 0 ){
            outbuf[i/2] |= (int8)((uint32)temp << 4);
        }
        else{
            outbuf[i/2] |= (int8)temp;
        }
    }

    return sum;
}


int32 hwifi_get_mac_addr_etc(uint8 *puc_buf)
{
    struct hisi_nve_info_user st_info;
    int32 l_ret = -1;
    int32 l_sum = 0;

    if (NULL == puc_buf)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "hwifi_get_mac_addr_etc::buf is NULL!");
        return INI_FAILED;
    }

    oal_memset(puc_buf, 0, MAC_LEN);

    oal_memset(&st_info, 0, sizeof(st_info));
    st_info.nv_number  = NV_WLAN_NUM;   //nve item

    strncpy(st_info.nv_name, "MACWLAN", sizeof("MACWLAN"));

    st_info.valid_size = NV_WLAN_VALID_SIZE;
    st_info.nv_operation = NV_READ;

    if (0 != g_auc_wifimac_etc[0] || 0 != g_auc_wifimac_etc[1] || 0 != g_auc_wifimac_etc[2] || 0 != g_auc_wifimac_etc[3]
        || 0 != g_auc_wifimac_etc[4] || 0 != g_auc_wifimac_etc[5])
    {
        memcpy(puc_buf, g_auc_wifimac_etc, MAC_LEN);
        return INI_SUCC;
    }

    l_ret = hisi_nve_direct_access(&st_info);

    if (!l_ret)
    {
        l_sum = char2byte(st_info.nv_data, (int8*)puc_buf);
        if (0 != l_sum)
        {
            INI_WARNING("hwifi_get_mac_addr_etc get MAC from NV: mac="MACFMT"\n", MAC2STR(puc_buf));
            oal_memcopy(g_auc_wifimac_etc, puc_buf, MAC_LEN);
        }else{
            random_ether_addr(puc_buf);
            puc_buf[1] = 0x11;
            puc_buf[2] = 0x02;
        }
    }else{
        random_ether_addr(puc_buf);
        puc_buf[1] = 0x11;
        puc_buf[2] = 0x02;
    }

    return INI_SUCC;
}

int32 hwifi_get_init_value_etc(int32 cus_tag, int32 cfg_id)
{
    int32*              pgal_params = OAL_PTR_NULL;
    int32               l_wlan_cfg_butt;

    if (CUS_TAG_INI == cus_tag)
    {
        pgal_params = &g_al_host_init_params_etc[0];
        l_wlan_cfg_butt = WLAN_CFG_INIT_BUTT;
    }
    else if (CUS_TAG_DTS == cus_tag)
    {
        pgal_params = &g_al_dts_params_etc[0];
        l_wlan_cfg_butt = WLAN_CFG_DTS_BUTT;
    }
    else
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "hwifi_get_init_value_etc tag number[0x%2x] not correct!", cus_tag);
        return INI_FAILED;
    }

    if (0 > cfg_id || l_wlan_cfg_butt <= cfg_id)
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "hwifi_get_init_value_etc cfg id[%d] out of range, max cfg id is:%d", cfg_id, (l_wlan_cfg_butt - 1));
        return INI_FAILED;
    }

    return pgal_params[cfg_id];
}


int32 hwifi_get_init_priv_value(oal_int32 l_cfg_id, oal_int32 *pl_priv_value)
{
    if (0 > l_cfg_id || WLAN_CFG_PRIV_BUTT <= l_cfg_id)
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "hwifi_get_init_priv_value cfg id[%d] out of range, max[%d]", l_cfg_id, WLAN_CFG_PRIV_BUTT - 1);
        return OAL_FAIL;
    }

    if (OAL_FALSE == g_al_priv_cust_params[l_cfg_id].en_value_state)
    {
        return OAL_FAIL;
    }

    *pl_priv_value = g_al_priv_cust_params[l_cfg_id].l_val;

    return OAL_SUCC;
}


int8 *hwifi_get_country_code_etc(void)
{
    /*Note:declaration of symbol "l_ret" hides symbol "l_ret"*/
    /*lint -e578*/
    int32 l_ret;
    /*lint +e578*/

    if (g_ac_country_code_etc[0] != '0' && g_ac_country_code_etc[1] != '0')
    {
        return g_ac_country_code_etc;
    }

    /* 获取cust国家码 */
    l_ret = get_cust_conf_string_etc(INI_MODU_WIFI, STR_COUNTRY_CODE, g_ac_country_code_etc, sizeof(g_ac_country_code_etc)-1);

    if(INI_FAILED == l_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "hwifi_get_country_code_etc read country code failed, check if it exists!");
        strncpy(g_ac_country_code_etc, "99", 2);
    }
    else
    {
        if (!OAL_MEMCMP(g_ac_country_code_etc, "99", 2))
        {
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "hwifi_get_country_code_etc is set 99!");
            g_st_cust_country_code_ignore_flag.en_country_code_ingore_ini_flag = OAL_TRUE;
        }
    }

    g_ac_country_code_etc[2] = '\0';

    return g_ac_country_code_etc;
}


void hwifi_set_country_code_etc(int8* country_code, const uint32 len)
{
    if (OAL_PTR_NULL == country_code || len != COUNTRY_CODE_LEN)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "hwifi_get_country_code_etc ptr null or illegal len!");
        return;
    }

    oal_memcopy(g_ac_country_code_etc, country_code, COUNTRY_CODE_LEN);
    g_ac_country_code_etc[2] = '\0';
}


void *hwifi_get_nvram_params_etc(void)
{
    return &g_st_cust_nv_params;
}


int hwifi_get_cfg_params(void)
{
    int32                  l_cfg_idx_one = 0;
    int32                  l_cfg_idx_two = 0;
    wlan_cust_nvram_params *pst_cust_nv_params = hwifi_get_nvram_params_etc();  /* 最大发送功率定制化数组 */

    OAL_IO_PRINT("\nhwifi_get_cfg_params\n");

    //CUS_TAG_INI
    for(l_cfg_idx_one = 0; l_cfg_idx_one < WLAN_CFG_INIT_BUTT; ++l_cfg_idx_one)
    {
        OAL_IO_PRINT("%s \t [config:0x%x]\n", g_ast_wifi_config_cmds[l_cfg_idx_one].name,
            g_al_host_init_params_etc[l_cfg_idx_one]);
    }

    //CUS_TAG_TXPWR
    for(l_cfg_idx_one = 0; l_cfg_idx_one < NUM_OF_NV_MAX_TXPOWER; ++l_cfg_idx_one)
    {
        OAL_IO_PRINT("%s%d \t [config:%d]\n", "delt_txpwr_params", l_cfg_idx_one, pst_cust_nv_params->ac_delt_txpwr_params[l_cfg_idx_one]);
    }

    for(l_cfg_idx_one = 0; l_cfg_idx_one < NUM_OF_NV_DPD_MAX_TXPOWER; ++l_cfg_idx_one)
    {
        OAL_IO_PRINT("%s%d \t [config:%d]\n", "delt_dpd_txpwr_params", l_cfg_idx_one, pst_cust_nv_params->ac_dpd_delt_txpwr_params[l_cfg_idx_one]);
    }

    for(l_cfg_idx_one = 0; l_cfg_idx_one < NUM_OF_NV_11B_DELTA_TXPOWER; ++l_cfg_idx_one)
    {
        OAL_IO_PRINT("%s%d \t [config:%d]\n", "delt_11b_txpwr_params", l_cfg_idx_one, pst_cust_nv_params->ac_11b_delt_txpwr_params[l_cfg_idx_one]);
    }

    for(l_cfg_idx_one = 0; l_cfg_idx_one < NUM_OF_NV_5G_UPPER_UPC; ++l_cfg_idx_one)
    {
        OAL_IO_PRINT("%s%d \t [config:%d]\n", "5g_upper_upc_params", l_cfg_idx_one, pst_cust_nv_params->auc_5g_upper_upc_params[l_cfg_idx_one]);
    }

    for (l_cfg_idx_one = 0; l_cfg_idx_one < WLAN_RF_CHANNEL_NUMS; l_cfg_idx_one++)
    {
        for (l_cfg_idx_two = 0; l_cfg_idx_two < CUS_BASE_PWR_NUM_2G; l_cfg_idx_two++)
        {
            OAL_IO_PRINT("%s[%d][%d] \t [config:%d]\n", "2G base_pwr_params",l_cfg_idx_one, l_cfg_idx_two, pst_cust_nv_params->auc_2g_txpwr_base_params[l_cfg_idx_one][l_cfg_idx_two]);
        }
        for (l_cfg_idx_two = 0; l_cfg_idx_two < CUS_BASE_PWR_NUM_5G; l_cfg_idx_two++)
        {
            OAL_IO_PRINT("%s[%d][%d] \t [config:%d]\n", "5G base_pwr_params",l_cfg_idx_one, l_cfg_idx_two, pst_cust_nv_params->auc_5g_txpwr_base_params[l_cfg_idx_one][l_cfg_idx_two]);
        }
    }

    for (l_cfg_idx_one = 0; l_cfg_idx_one < CUS_NUM_5G_20M_SIDE_BAND; l_cfg_idx_one++)
    {
        OAL_IO_PRINT("%s[%d] \t [config:%d]\n", "fcc_txpwr_limit_5g:20M side_band",l_cfg_idx_one,
                     pst_cust_nv_params->auc_5g_fcc_txpwr_limit_params_20M[l_cfg_idx_one]);
    }

    for (l_cfg_idx_one = 0; l_cfg_idx_one < CUS_NUM_5G_40M_SIDE_BAND; l_cfg_idx_one++)
    {
        OAL_IO_PRINT("%s[%d] \t [config:%d]\n", "fcc_txpwr_limit_5g:40M side_band",l_cfg_idx_one,
                     pst_cust_nv_params->auc_5g_fcc_txpwr_limit_params_40M[l_cfg_idx_one]);
    }
    for (l_cfg_idx_one = 0; l_cfg_idx_one < CUS_NUM_5G_80M_SIDE_BAND; l_cfg_idx_one++)
    {
        OAL_IO_PRINT("%s[%d] \t [config:%d]\n", "fcc_txpwr_limit_5g:80M side_band",l_cfg_idx_one,
                     pst_cust_nv_params->auc_5g_fcc_txpwr_limit_params_80M[l_cfg_idx_one]);
    }
    for (l_cfg_idx_one = 0; l_cfg_idx_one < CUS_NUM_5G_160M_SIDE_BAND; l_cfg_idx_one++)
    {
        OAL_IO_PRINT("%s[%d] \t [config:%d]\n", "fcc_txpwr_limit_5g:160M side_band",l_cfg_idx_one,
                     pst_cust_nv_params->auc_5g_fcc_txpwr_limit_params_160M[l_cfg_idx_one]);
    }
    for (l_cfg_idx_one = 0; l_cfg_idx_one < MAC_2G_CHANNEL_NUM; l_cfg_idx_one++)
    {
        for (l_cfg_idx_two = 0; l_cfg_idx_two < CUS_NUM_FCC_2G_PRO; l_cfg_idx_two++)
        {
            OAL_IO_PRINT("%s[%d] [%d] \t [config:%d]\n", "fcc_txpwr_limit_2g: chan",l_cfg_idx_one, l_cfg_idx_two,
                         pst_cust_nv_params->auc_2g_fcc_txpwr_limit_params[l_cfg_idx_one][l_cfg_idx_two]);
        }
    }

    for (l_cfg_idx_one = 0; l_cfg_idx_one < CUS_NUM_OF_SAR_LVL; l_cfg_idx_one++)
    {
        for (l_cfg_idx_two = 0; l_cfg_idx_two < CUS_NUM_OF_SAR_PARAMS; l_cfg_idx_two++)
        {
            OAL_IO_PRINT("%s[%d][%d] \t [config:%d]\n", "sar_ctrl_params: lvl",l_cfg_idx_one, l_cfg_idx_two,
                         pst_cust_nv_params->auc_sar_ctrl_params[l_cfg_idx_one][l_cfg_idx_two]);
        }
    }

#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH
    for (l_cfg_idx_one = 0; l_cfg_idx_one < WLAN_RF_CHANNEL_NUMS; l_cfg_idx_one++)
    {
        for (l_cfg_idx_two = 0; l_cfg_idx_two < WLAN_BAND_BUTT; l_cfg_idx_two++)
        {
            OAL_IO_PRINT("%s[%d][%d] \t [config:%d]\n", "tas_ctrl_params: lvl",l_cfg_idx_one, l_cfg_idx_two,
                         pst_cust_nv_params->auc_tas_ctrl_params[l_cfg_idx_one][l_cfg_idx_two]);
        }
    }
#endif

    OAL_IO_PRINT("%s \t [config:%d]\n", g_ast_nvram_config_ini[NVRAM_PARAMS_5G_CE_HIGH_BAND_MAX_PWR].name, pst_cust_nv_params->uc_5g_max_pwr_for_ce_high_band);

    //CUS_TAG_DTS
    for(l_cfg_idx_one = 0; l_cfg_idx_one < WLAN_CFG_DTS_BUTT; ++l_cfg_idx_one)
    {
        OAL_IO_PRINT("%s \t [config:0x%x]\n", g_ast_wifi_config_dts[l_cfg_idx_one].name,
            g_al_dts_params_etc[l_cfg_idx_one]);
    }

    /* pro line */
    for (l_cfg_idx_one = 0; l_cfg_idx_one < WLAN_RF_CHANNEL_NUMS; l_cfg_idx_one++)
    {
        for (l_cfg_idx_two = 0; l_cfg_idx_two < DY_CALI_PARAMS_NUM; l_cfg_idx_two++)
        {
            OAL_IO_PRINT("%s CORE[%d]para_idx[%d]::{%d, %d, %d}\n", "g_ast_pro_line_params: ",l_cfg_idx_one, l_cfg_idx_two,
                          g_ast_pro_line_params[l_cfg_idx_one][l_cfg_idx_two].l_pow_par2, g_ast_pro_line_params[l_cfg_idx_one][l_cfg_idx_two].l_pow_par1,
                          g_ast_pro_line_params[l_cfg_idx_one][l_cfg_idx_two].l_pow_par0);
        }
    }
    /* NVRAM */
    OAL_IO_PRINT("%s : { %d }\n", "g_en_nv_dp_init_is_null: ", g_en_nv_dp_init_is_null);
    for (l_cfg_idx_one = 0; l_cfg_idx_one < WLAN_CFG_DTS_NVRAM_END; l_cfg_idx_one++)
    {
         OAL_IO_PRINT("%s para_idx[%d] name[%s]::DATA{%s}\n", "dp init & ratios nvram_param: ", l_cfg_idx_one,
                      g_ast_wifi_nvram_cfg_handler[l_cfg_idx_one].puc_param_name,
                      hwifi_get_nvram_param(l_cfg_idx_one));
     }

    return INI_SUCC;
}





EXPORT_SYMBOL_GPL(g_st_cust_country_code_ignore_flag);
EXPORT_SYMBOL_GPL(g_st_wlan_customize_etc);
EXPORT_SYMBOL_GPL(g_ast_pro_line_params);
EXPORT_SYMBOL_GPL(gs_extre_point_vals);
EXPORT_SYMBOL_GPL(g_en_nv_dp_init_is_null);
#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH
EXPORT_SYMBOL_GPL(g_en_tas_switch_en);
#endif
EXPORT_SYMBOL_GPL(g_en_fact_cali_completed);
EXPORT_SYMBOL_GPL(hwifi_config_init_etc);
EXPORT_SYMBOL_GPL(hwifi_get_mac_addr_etc);
EXPORT_SYMBOL_GPL(hwifi_get_init_value_etc);
EXPORT_SYMBOL_GPL(hwifi_get_country_code_etc);
EXPORT_SYMBOL_GPL(hwifi_get_nvram_params_etc);
EXPORT_SYMBOL_GPL(hwifi_fetch_ori_caldata_etc);
EXPORT_SYMBOL_GPL(hwifi_set_country_code_etc);
EXPORT_SYMBOL_GPL(hwifi_is_regdomain_changed_etc);
EXPORT_SYMBOL_GPL(hwifi_get_cfg_params);
EXPORT_SYMBOL_GPL(hwifi_custom_host_read_cfg_init);
EXPORT_SYMBOL_GPL(hwifi_get_init_priv_value);
EXPORT_SYMBOL_GPL(hwifi_get_regdomain_from_country_code);
EXPORT_SYMBOL_GPL(hwifi_get_nvram_param);

#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */

#endif // #if defined(_PRE_PRODUCT_ID_HI110X_HOST)

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

