

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)

/*
 * 1 Header File Including
 */
#include "oal_types.h"
#include "oal_schedule.h"
#include "oal_ext_if.h"
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
#include <linux/kernel.h>
#include <linux/time.h>
#include "hisi_customize_wifi_hi115x.h"
#include "hisi_ini.h"
#include "plat_type.h"
#include "oam_ext_if.h"
#include "wlan_spec.h"
#include "mac_vap.h"
#include "oal_sdio_comm.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HISI_CUSTOMIZE_WIFI_HI115X_C

/*
 * 2 Global Variable Definition
 */
int32 g_al_host_init_params_etc[WLAN_CFG_INIT_BUTT] = {0};      /* ini定制化参数数组 */
int32 g_al_dts_params_etc[WLAN_CFG_DTS_BUTT] = {0};             /* dts定制化参数数组 */
uint16 g_aus_nv_params[NUM_OF_NV_PARAMS] = {0};              /* nv定制化参数数组 */
int8  g_ac_country_code_etc[COUNTRY_CODE_LEN] = "00";
int8  g_chipid_cfg[MAX_CHIPID_COUNT][CHIPID_CFG_LEN] = {{0}};

#if defined (_PRE_WLAN_FEATURE_RX_AGGR_EXTEND) || defined (_PRE_FEATURE_WAVEAPP_CLASSIFY)
uint16 g_us_ixia_mac_addr_num = 0;
wlan_ixia_mac_addr_stru g_ixia_mac_addr_cfg[MAX_IXIA_MAC_ADDR_COUNT];
#endif

#ifdef _PRE_WLAN_MAC_ADDR_EDCA_FIX
uint16 g_us_mac_addr_num = 0;
wlan_edca_mac_addr_stru g_edca_mac_addr_cfg[MAX_EDCA_MAC_ADDR_COUNT];//保存定制化中mac地址以及对应的be edca
#endif

uint8 g_auc_wifimac_etc[MAC_LEN] = {0x00,0x00,0x00,0x00,0x00,0x00};
int32 g_al_nvram_init_params[NVRAM_PARAMS_INDEX_BUTT] = {0};
wlan_customize_private_stru g_al_priv_cust_params[WLAN_CFG_PRIV_BUTT] = {{0,0}};  /* 私有定制化参数数组 */
wlan_cust_country_code_ingore_flag_stru g_st_cust_country_code_ignore_flag = {0}; /* 定制化国家码配置 */

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
    {REGDOMAIN_ETSI, {'M', 'K'}}, //MACEDONIA, THE FORMER YUGOSLAV REPUBLIC OF
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

    {REGDOMAIN_COUNT,{'9','9'}},
    /*lint +e785*/
};

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

OAL_STATIC wlan_cfg_cmd g_ast_wifi_config_dts[] =
{
#ifdef _PRE_WLAN_RF_CALI_1151V2
    /* Hi1151V200 校准序列 */
    {"1151v2_cali_seq_0",                                    WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_0},
    {"1151v2_cali_seq_1",                                    WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_1},
    {"1151v2_cali_seq_2",                                    WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_2},
    {"1151v2_cali_seq_3",                                    WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_3},
    {"1151v2_cali_seq_4",                                    WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_4},
    {"1151v2_cali_seq_5",                                    WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_5},
    {"1151v2_cali_seq_6",                                    WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_6},
    {"1151v2_cali_seq_7",                                    WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_7},
    {"1151v2_cali_seq_8",                                    WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_8},
    {"1151v2_cali_seq_9",                                    WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_9},
    {"1151v2_cali_seq_10",                                   WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_10},
    {"1151v2_cali_seq_11",                                   WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_11},
    {"1151v2_cali_seq_12",                                   WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_12},
    {"1151v2_cali_seq_13",                                   WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_13},
    {"1151v2_cali_seq_14",                                   WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_14},
    {"1151v2_cali_seq_15",                                   WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_15},
    {"1151v2_cali_seq_16",                                   WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_16},
    {"1151v2_cali_seq_17",                                   WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_17},
    {"1151v2_cali_seq_18",                                   WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_18},
    {"1151v2_cali_seq_19",                                   WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_19},
    {"1151v2_cali_seq_20",                                   WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_20},
    {"1151v2_cali_seq_21",                                   WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_21},
    {"1151v2_cali_seq_22",                                   WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_22},
    {"1151v2_cali_seq_23",                                   WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_23},
    {"1151v2_cali_seq_24",                                   WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_24},
    {"1151v2_cali_seq_25",                                   WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_25},
    {"1151v2_cali_seq_26",                                   WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_26},
    {"1151v2_cali_seq_27",                                   WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_27},
    {"1151v2_cali_seq_28",                                   WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_28},
    {"1151v2_cali_seq_29",                                   WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_29},
    {"1151v2_cali_seq_30",                                   WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_30},
    {"1151v2_cali_seq_31",                                   WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_31},
#endif

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

    {"cali_txpwr_pa_dc_ref_2g_b40_val_chan1",                WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_B40_VAL_CHAN1},
    {"cali_txpwr_pa_dc_ref_2g_b40_val_chan2",                WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_B40_VAL_CHAN2},
    {"cali_txpwr_pa_dc_ref_2g_b40_val_chan3",                WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_B40_VAL_CHAN3},
    {"cali_txpwr_pa_dc_ref_2g_b40_val_chan4",                WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_B40_VAL_CHAN4},
    {"cali_txpwr_pa_dc_ref_2g_b40_val_chan5",                WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_B40_VAL_CHAN5},
    {"cali_txpwr_pa_dc_ref_2g_b40_val_chan6",                WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_B40_VAL_CHAN6},
    {"cali_txpwr_pa_dc_ref_2g_b40_val_chan7",                WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_B40_VAL_CHAN7},
    {"cali_txpwr_pa_dc_ref_2g_b40_val_chan8",                WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_B40_VAL_CHAN8},
    {"cali_txpwr_pa_dc_ref_2g_b40_val_chan9",                WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_B40_VAL_CHAN9},
    {"cali_txpwr_pa_dc_ref_2g_b40_val_chan10",               WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_B40_VAL_CHAN10},
    {"cali_txpwr_pa_dc_ref_2g_b40_val_chan11",               WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_B40_VAL_CHAN11},
    {"cali_txpwr_pa_dc_ref_2g_b40_val_chan12",               WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_B40_VAL_CHAN12},
    {"cali_txpwr_pa_dc_ref_2g_b40_val_chan13",               WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_B40_VAL_CHAN13},

    {"cali_txpwr_pa_dc_ref_5g_val_band1",                    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND1},
    {"cali_txpwr_pa_dc_ref_5g_val_band2",                    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND2},
    {"cali_txpwr_pa_dc_ref_5g_val_band3",                    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND3},
    {"cali_txpwr_pa_dc_ref_5g_val_band4",                    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND4},
    {"cali_txpwr_pa_dc_ref_5g_val_band5",                    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND5},
    {"cali_txpwr_pa_dc_ref_5g_val_band6",                    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND6},
    {"cali_txpwr_pa_dc_ref_5g_val_band7",                    WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND7},
    {"cali_tone_amp_grade",                                  WLAN_CFG_DTS_CALI_TONE_AMP_GRADE},
    {"mimo_pow_adjust",                                      WLAN_CFG_DTS_MIMO_POW_ADJUST},
    /* 动态校准 */
    { "dyn_cali_dscr_interval",                              WLAN_CFG_DTS_DYN_CALI_DSCR_ITERVL},
    /* FCC认证 */
    {"band_edge_limit_2g_11g_txpwr",                         WLAN_CFG_DTS_BAND_EDGE_LIMIT_2G_11G_TXPWR},
    {"band_edge_limit_2g_11n_ht20_txpwr",                    WLAN_CFG_DTS_BAND_EDGE_LIMIT_2G_11N_HT20_TXPWR},
    {"band_edge_limit_2g_11n_ht40_txpwr",                    WLAN_CFG_DTS_BAND_EDGE_LIMIT_2G_11N_HT40_TXPWR},
    {"band_edge_limit_5g_11a_ht20_vht20_txpwr",              WLAN_CFG_DTS_BAND_EDGE_LIMIT_5G_11A_HT20_VHT20_TXPWR},
    {"band_edge_limit_5g_ht40_vht40_txpwr",                  WLAN_CFG_DTS_BAND_EDGE_LIMIT_5G_HT40_VHT40_TXPWR},
    {"band_edge_limit_5g_vht80_txpwr",                       WLAN_CFG_DTS_BAND_EDGE_LIMIT_5G_VHT80_TXPWR},
    {"band_edge_limit_2g_11g_dbb_scaling",                   WLAN_CFG_DTS_BAND_EDGE_LIMIT_2G_11G_DBB_SCALING},
    {"band_edge_limit_2g_11n_ht20_dbb_scaling",              WLAN_CFG_DTS_BAND_EDGE_LIMIT_2G_11N_HT20_DBB_SCALING},
    {"band_edge_limit_2g_11n_ht40_dbb_scaling",              WLAN_CFG_DTS_BAND_EDGE_LIMIT_2G_11N_HT40_DBB_SCALING},
    {"band_edge_limit_5g_11a_ht20_vht20_dbb_scaling",        WLAN_CFG_DTS_BAND_EDGE_LIMIT_5G_11A_HT20_VHT20_DBB_SCALING},
    {"band_edge_limit_5g_ht40_vht40_dbb_scaling",            WLAN_CFG_DTS_BAND_EDGE_LIMIT_5G_HT40_VHT40_DBB_SCALING},
    {"band_edge_limit_5g_vht80_dbb_scaling",                 WLAN_CFG_DTS_BAND_EDGE_LIMIT_5G_VHT80_DBB_SCALING},

    /* 动态校准 */
    {"dync_cali_ofdm_single_stone_2g_c0_chan1",              WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C0_CHAN1},
    {"dync_cali_ofdm_single_stone_2g_c0_chan2",              WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C0_CHAN2},
    {"dync_cali_ofdm_single_stone_2g_c0_chan3",              WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C0_CHAN3},
    {"dync_cali_ofdm_single_stone_2g_c0_chan4",              WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C0_CHAN4},
    {"dync_cali_ofdm_single_stone_2g_c0_chan5",              WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C0_CHAN5},
    {"dync_cali_ofdm_single_stone_2g_c0_chan6",              WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C0_CHAN6},
    {"dync_cali_ofdm_single_stone_2g_c0_chan7",              WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C0_CHAN7},
    {"dync_cali_ofdm_single_stone_2g_c0_chan8",              WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C0_CHAN8},
    {"dync_cali_ofdm_single_stone_2g_c0_chan9",              WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C0_CHAN9},
    {"dync_cali_ofdm_single_stone_2g_c0_chan10",             WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C0_CHAN10},
    {"dync_cali_ofdm_single_stone_2g_c0_chan11",             WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C0_CHAN11},
    {"dync_cali_ofdm_single_stone_2g_c0_chan12",             WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C0_CHAN12},
    {"dync_cali_ofdm_single_stone_2g_c0_chan13",             WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C0_CHAN13},
    {"dync_cali_ofdm_single_stone_2g_c1_chan1",              WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C1_CHAN1},
    {"dync_cali_ofdm_single_stone_2g_c1_chan2",              WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C1_CHAN2},
    {"dync_cali_ofdm_single_stone_2g_c1_chan3",              WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C1_CHAN3},
    {"dync_cali_ofdm_single_stone_2g_c1_chan4",              WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C1_CHAN4},
    {"dync_cali_ofdm_single_stone_2g_c1_chan5",              WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C1_CHAN5},
    {"dync_cali_ofdm_single_stone_2g_c1_chan6",              WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C1_CHAN6},
    {"dync_cali_ofdm_single_stone_2g_c1_chan7",              WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C1_CHAN7},
    {"dync_cali_ofdm_single_stone_2g_c1_chan8",              WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C1_CHAN8},
    {"dync_cali_ofdm_single_stone_2g_c1_chan9",              WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C1_CHAN9},
    {"dync_cali_ofdm_single_stone_2g_c1_chan10",             WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C1_CHAN10},
    {"dync_cali_ofdm_single_stone_2g_c1_chan11",             WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C1_CHAN11},
    {"dync_cali_ofdm_single_stone_2g_c1_chan12",             WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C1_CHAN12},
    {"dync_cali_ofdm_single_stone_2g_c1_chan13",             WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C1_CHAN13},

    {"dync_cali_ofdm_single_stone_2g_b40_c0_chan1",          WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C0_CHAN1},
    {"dync_cali_ofdm_single_stone_2g_b40_c0_chan2",          WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C0_CHAN2},
    {"dync_cali_ofdm_single_stone_2g_b40_c0_chan3",          WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C0_CHAN3},
    {"dync_cali_ofdm_single_stone_2g_b40_c0_chan4",          WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C0_CHAN4},
    {"dync_cali_ofdm_single_stone_2g_b40_c0_chan5",          WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C0_CHAN5},
    {"dync_cali_ofdm_single_stone_2g_b40_c0_chan6",          WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C0_CHAN6},
    {"dync_cali_ofdm_single_stone_2g_b40_c0_chan7",          WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C0_CHAN7},
    {"dync_cali_ofdm_single_stone_2g_b40_c0_chan8",          WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C0_CHAN8},
    {"dync_cali_ofdm_single_stone_2g_b40_c0_chan9",          WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C0_CHAN9},
    {"dync_cali_ofdm_single_stone_2g_b40_c0_chan10",         WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C0_CHAN10},
    {"dync_cali_ofdm_single_stone_2g_b40_c0_chan11",         WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C0_CHAN11},
    {"dync_cali_ofdm_single_stone_2g_b40_c0_chan12",         WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C0_CHAN12},
    {"dync_cali_ofdm_single_stone_2g_b40_c0_chan13",         WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C0_CHAN13},
    {"dync_cali_ofdm_single_stone_2g_b40_c1_chan1",          WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C1_CHAN1},
    {"dync_cali_ofdm_single_stone_2g_b40_c1_chan2",          WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C1_CHAN2},
    {"dync_cali_ofdm_single_stone_2g_b40_c1_chan3",          WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C1_CHAN3},
    {"dync_cali_ofdm_single_stone_2g_b40_c1_chan4",          WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C1_CHAN4},
    {"dync_cali_ofdm_single_stone_2g_b40_c1_chan5",          WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C1_CHAN5},
    {"dync_cali_ofdm_single_stone_2g_b40_c1_chan6",          WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C1_CHAN6},
    {"dync_cali_ofdm_single_stone_2g_b40_c1_chan7",          WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C1_CHAN7},
    {"dync_cali_ofdm_single_stone_2g_b40_c1_chan8",          WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C1_CHAN8},
    {"dync_cali_ofdm_single_stone_2g_b40_c1_chan9",          WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C1_CHAN9},
    {"dync_cali_ofdm_single_stone_2g_b40_c1_chan10",         WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C1_CHAN10},
    {"dync_cali_ofdm_single_stone_2g_b40_c1_chan11",         WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C1_CHAN11},
    {"dync_cali_ofdm_single_stone_2g_b40_c1_chan12",         WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C1_CHAN12},
    {"dync_cali_ofdm_single_stone_2g_b40_c1_chan13",         WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C1_CHAN13},

    {"dync_cali_ofdm_single_stone_5g_c0_band1",              WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_5G_C0_BAND1},
    {"dync_cali_ofdm_single_stone_5g_c0_band2",              WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_5G_C0_BAND2},
    {"dync_cali_ofdm_single_stone_5g_c0_band3",              WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_5G_C0_BAND3},
    {"dync_cali_ofdm_single_stone_5g_c0_band4",              WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_5G_C0_BAND4},
    {"dync_cali_ofdm_single_stone_5g_c0_band5",              WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_5G_C0_BAND5},
    {"dync_cali_ofdm_single_stone_5g_c0_band6",              WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_5G_C0_BAND6},
    {"dync_cali_ofdm_single_stone_5g_c0_band7",              WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_5G_C0_BAND7},
    {"dync_cali_ofdm_single_stone_5g_c1_band1",              WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_5G_C1_BAND1},
    {"dync_cali_ofdm_single_stone_5g_c1_band2",              WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_5G_C1_BAND2},
    {"dync_cali_ofdm_single_stone_5g_c1_band3",              WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_5G_C1_BAND3},
    {"dync_cali_ofdm_single_stone_5g_c1_band4",              WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_5G_C1_BAND4},
    {"dync_cali_ofdm_single_stone_5g_c1_band5",              WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_5G_C1_BAND5},
    {"dync_cali_ofdm_single_stone_5g_c1_band6",              WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_5G_C1_BAND6},
    {"dync_cali_ofdm_single_stone_5g_c1_band7",              WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_5G_C1_BAND7},
    {"dync_cali_ofdm_single_stone_txpower_spec",             WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_TXPOW_SPEC},
    {"dync_cali_equip_txpower_lut",                          WLAN_CFG_DTS_DYNC_CALI_EQUIP_TXPOW_LUT},

    /* 动态校准天线与线缆负载差异补偿值 */
    {"dync_cali_pow_offset_2g_c0",                           WLAN_CFG_DTS_DYNC_CALI_POW_OFFSET_2G_C0},
    {"dync_cali_pow_offset_2g_c1",                           WLAN_CFG_DTS_DYNC_CALI_POW_OFFSET_2G_C1},
    {"dync_cali_pow_offset_5g_c0_1",                         WLAN_CFG_DTS_DYNC_CALI_POW_OFFSET_5G_C0_1},
    {"dync_cali_pow_offset_5g_c1_1",                         WLAN_CFG_DTS_DYNC_CALI_POW_OFFSET_5G_C1_1},
    {"dync_cali_pow_offset_5g_c0_2",                         WLAN_CFG_DTS_DYNC_CALI_POW_OFFSET_5G_C0_2},
    {"dync_cali_pow_offset_5g_c1_2",                         WLAN_CFG_DTS_DYNC_CALI_POW_OFFSET_5G_C1_2},

    /* 动态校准pow&pdet曲线多项式参数值 */
    {"dync_cali_polynomial_para_2g_b1_fst",                  WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_2G_BAND1_FST},
    {"dync_cali_polynomial_para_2g_b1_snd",                  WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_2G_BAND1_SND},
    {"dync_cali_polynomial_para_2g_b1_trd",                  WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_2G_BAND1_TRD},
    {"dync_cali_polynomial_para_2g_b2_fst",                  WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_2G_BAND2_FST},
    {"dync_cali_polynomial_para_2g_b2_snd",                  WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_2G_BAND2_SND},
    {"dync_cali_polynomial_para_2g_b2_trd",                  WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_2G_BAND2_TRD},
    {"dync_cali_polynomial_para_2g_b3_fst",                  WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_2G_BAND3_FST},
    {"dync_cali_polynomial_para_2g_b3_snd",                  WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_2G_BAND3_SND},
    {"dync_cali_polynomial_para_2g_b3_trd",                  WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_2G_BAND3_TRD},

    {"dync_cali_polynomial_para_5g_b1_fst",                  WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND1_FST},
    {"dync_cali_polynomial_para_5g_b1_snd",                  WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND1_SND},
    {"dync_cali_polynomial_para_5g_b1_trd",                  WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND1_TRD},
    {"dync_cali_polynomial_para_5g_b2_fst",                  WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND2_FST},
    {"dync_cali_polynomial_para_5g_b2_snd",                  WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND2_SND},
    {"dync_cali_polynomial_para_5g_b2_trd",                  WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND2_TRD},
    {"dync_cali_polynomial_para_5g_b3_fst",                  WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND3_FST},
    {"dync_cali_polynomial_para_5g_b3_snd",                  WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND3_SND},
    {"dync_cali_polynomial_para_5g_b3_trd",                  WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND3_TRD},
    {"dync_cali_polynomial_para_5g_b4_fst",                  WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND4_FST},
    {"dync_cali_polynomial_para_5g_b4_snd",                  WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND4_SND},
    {"dync_cali_polynomial_para_5g_b4_trd",                  WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND4_TRD},
    {"dync_cali_polynomial_para_5g_b5_fst",                  WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND5_FST},
    {"dync_cali_polynomial_para_5g_b5_snd",                  WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND5_SND},
    {"dync_cali_polynomial_para_5g_b5_trd",                  WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND5_TRD},
    {"dync_cali_polynomial_para_5g_b6_fst",                  WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND6_FST},
    {"dync_cali_polynomial_para_5g_b6_snd",                  WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND6_SND},
    {"dync_cali_polynomial_para_5g_b6_trd",                  WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND7_TRD},
    {"dync_cali_polynomial_para_5g_b7_fst",                  WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND7_FST},
    {"dync_cali_polynomial_para_5g_b7_snd",                  WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND7_SND},
    {"dync_cali_polynomial_para_5g_b7_trd",                  WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND7_TRD},

    {"dync_cali_dbb_2g_para1",              WLAN_CFG_DTS_DYNC_CALI_OFDM_DBB_2G_PARA1},
    {"dync_cali_dbb_2g_para2",              WLAN_CFG_DTS_DYNC_CALI_OFDM_DBB_2G_PARA2},
    {"dync_cali_dbb_2g_para3",              WLAN_CFG_DTS_DYNC_CALI_OFDM_DBB_2G_PARA3},
    {"dync_cali_dbb_2g_para4",              WLAN_CFG_DTS_DYNC_CALI_OFDM_DBB_2G_PARA4},
    {"dync_cali_dbb_2g_para5",              WLAN_CFG_DTS_DYNC_CALI_OFDM_DBB_2G_PARA5},
    {"dync_cali_dbb_5g_para1",              WLAN_CFG_DTS_DYNC_CALI_OFDM_DBB_5G_PARA1},
    {"dync_cali_dbb_5g_para2",              WLAN_CFG_DTS_DYNC_CALI_OFDM_DBB_5G_PARA2},
    {"dync_cali_dbb_5g_para3",              WLAN_CFG_DTS_DYNC_CALI_OFDM_DBB_5G_PARA3},
    {"dync_cali_dbb_5g_para4",              WLAN_CFG_DTS_DYNC_CALI_OFDM_DBB_5G_PARA4},
    {"dync_cali_dbb_5g_para5",              WLAN_CFG_DTS_DYNC_CALI_OFDM_DBB_5G_PARA5},
    {"dync_cali_dbb_5g_para6",              WLAN_CFG_DTS_DYNC_CALI_OFDM_DBB_5G_PARA6},
    {"dync_cali_dbb_5g_para7",              WLAN_CFG_DTS_DYNC_CALI_OFDM_DBB_5G_PARA7},

    /* DAC/LPF */
    {"dac_lpf_gain_2g_legacy_1",             WLAN_CFG_DTS_DAC_LPF_GAIN_2G_LEGACY_RATE_1},
    {"dac_lpf_gain_2g_legacy_2",             WLAN_CFG_DTS_DAC_LPF_GAIN_2G_LEGACY_RATE_2},
    {"dac_lpf_gain_2g_legacy_3",             WLAN_CFG_DTS_DAC_LPF_GAIN_2G_LEGACY_RATE_3},
    {"dac_lpf_gain_2g_legacy_4",             WLAN_CFG_DTS_DAC_LPF_GAIN_2G_LEGACY_RATE_4},
    {"dac_lpf_gain_5g_legacy_1",             WLAN_CFG_DTS_DAC_LPF_GAIN_5G_LEGACY_RATE_1},
    {"dac_lpf_gain_5g_legacy_2",             WLAN_CFG_DTS_DAC_LPF_GAIN_5G_LEGACY_RATE_2},
    {"dac_lpf_gain_2g_11ac_ht20_1",          WLAN_CFG_DTS_DAC_LPF_GAIN_2G_11AC_HT20_1},
    {"dac_lpf_gain_2g_11ac_ht20_2",          WLAN_CFG_DTS_DAC_LPF_GAIN_2G_11AC_HT20_2},
    {"dac_lpf_gain_2g_11ac_ht40_1",          WLAN_CFG_DTS_DAC_LPF_GAIN_2G_11AC_HT40_1},
    {"dac_lpf_gain_2g_11ac_ht40_2",          WLAN_CFG_DTS_DAC_LPF_GAIN_2G_11AC_HT40_2},
    {"dac_lpf_gain_5g_11ac_ht20_1",          WLAN_CFG_DTS_DAC_LPF_GAIN_5G_11AC_HT20_1},
    {"dac_lpf_gain_5g_11ac_ht20_2",          WLAN_CFG_DTS_DAC_LPF_GAIN_5G_11AC_HT20_2},
    {"dac_lpf_gain_5g_11ac_ht40_1",          WLAN_CFG_DTS_DAC_LPF_GAIN_5G_11AC_HT40_1},
    {"dac_lpf_gain_5g_11ac_ht40_2",          WLAN_CFG_DTS_DAC_LPF_GAIN_5G_11AC_HT40_2},
    {"dac_lpf_gain_5g_11ac_ht40_3",          WLAN_CFG_DTS_DAC_LPF_GAIN_5G_11AC_HT40_3},
    {"dac_lpf_gain_5g_11ac_ht80_1",          WLAN_CFG_DTS_DAC_LPF_GAIN_5G_11AC_HT80_1},
    {"dac_lpf_gain_5g_11ac_ht80_2",          WLAN_CFG_DTS_DAC_LPF_GAIN_5G_11AC_HT80_2},
    {"dac_lpf_gain_5g_11ac_ht80_3",          WLAN_CFG_DTS_DAC_LPF_GAIN_5G_11AC_HT80_3},

    /* RF PLL */
    {"rf_pll_ppm_2g",                        WLAN_CFG_DTS_RF_PLL_PPM_2G},
    {"rf_pll_ppm_5g",                        WLAN_CFG_DTS_RF_PLL_PPM_5G},

    {OAL_PTR_NULL, 0}
};

OAL_STATIC wlan_cfg_cmd g_ast_wifi_config_priv[] =
{
    /* 校准开关 */
    {"cali_mask",                   WLAN_CFG_PRIV_CALI_MASK},
    {"cali_auto_cali_mask",         WLAN_CFG_PRIV_CALI_AUTOCALI_MASK},


    /* TBD:hal_cfg_customize_info_stru/mac_device_capability_stru */

    /* DBDC */
    {"radio_cap_0",                 WLAN_CFG_PRIV_DBDC_RADIO_0},
    {"radio_cap_1",                 WLAN_CFG_PRIV_DBDC_RADIO_1},

    {OAL_PTR_NULL, 0}
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

    /* 性能 */
    {"ampdu_tx_max_num",                WLAN_CFG_INIT_AMPDU_TX_MAX_NUM},
    {"used_mem_for_start",              WLAN_CFG_INIT_USED_MEM_FOR_START},
    {"used_mem_for_stop",               WLAN_CFG_INIT_USED_MEM_FOR_STOP},
    {"rx_ack_limit",                    WLAN_CFG_INIT_RX_ACK_LIMIT},
    {"sdio_d2h_assemble_count",         WLAN_CFG_INIT_SDIO_D2H_ASSEMBLE_COUNT},
    {"sdio_h2d_assemble_count",         WLAN_CFG_INIT_SDIO_H2D_ASSEMBLE_COUNT},

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
    /* AP开Tx amsdu */
    {"ap_tx_amsdu",                     WLAN_CFG_INIT_AP_TX_AMSDU},
    /* AP开Rx amsdu */
    {"ap_rx_amsdu",                     WLAN_CFG_INIT_AP_RX_AMSDU},
    /* 低功耗 */
    {"powermgmt_switch",                WLAN_CFG_INIT_POWERMGMT_SWITCH},
    /* 可维可测 */
    {"loglevel",                        WLAN_CFG_INIT_LOGLEVEL},
    /* 2G RF前端 */
    {"rf_line_txrx_gain_db_2g_band1_mult4",     WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_2G_BAND1_MULT4},
    {"rf_line_txrx_gain_db_2g_band1_mult10",    WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_2G_BAND1_MULT10},
    {"rf_line_txrx_gain_db_2g_band2_mult4",     WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_2G_BAND2_MULT4},
    {"rf_line_txrx_gain_db_2g_band2_mult10",    WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_2G_BAND2_MULT10},
    {"rf_line_txrx_gain_db_2g_band3_mult4",     WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_2G_BAND3_MULT4},
    {"rf_line_txrx_gain_db_2g_band3_mult10",    WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_2G_BAND3_MULT10},
    /* 5G RF前端 */
    {"rf_line_txrx_gain_db_5g_band1_mult4",     WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_5G_BAND1_MULT4},
    {"rf_line_txrx_gain_db_5g_band1_mult10",    WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_5G_BAND1_MULT10},
    {"rf_line_txrx_gain_db_5g_band2_mult4",     WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_5G_BAND2_MULT4},
    {"rf_line_txrx_gain_db_5g_band2_mult10",    WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_5G_BAND2_MULT10},
    {"rf_line_txrx_gain_db_5g_band3_mult4",     WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_5G_BAND3_MULT4},
    {"rf_line_txrx_gain_db_5g_band3_mult10",    WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_5G_BAND3_MULT10},
    {"rf_line_txrx_gain_db_5g_band4_mult4",     WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_5G_BAND4_MULT4},
    {"rf_line_txrx_gain_db_5g_band4_mult10",    WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_5G_BAND4_MULT10},
    {"rf_line_txrx_gain_db_5g_band5_mult4",     WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_5G_BAND5_MULT4},
    {"rf_line_txrx_gain_db_5g_band5_mult10",    WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_5G_BAND5_MULT10},
    {"rf_line_txrx_gain_db_5g_band6_mult4",     WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_5G_BAND6_MULT4},
    {"rf_line_txrx_gain_db_5g_band6_mult10",    WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_5G_BAND6_MULT10},
    {"rf_line_txrx_gain_db_5g_band7_mult4",     WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_5G_BAND7_MULT4},
    {"rf_line_txrx_gain_db_5g_band7_mult10",    WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_5G_BAND7_MULT10},
    /* fem */
    {"rf_line_rx_gain_db_2g",           WLAN_CFG_INIT_RF_LINE_RX_GAIN_DB_2G},
    {"lna_gain_db_2g",                  WLAN_CFG_INIT_LNA_GAIN_DB_2G},
    {"rf_line_tx_gain_db_2g",           WLAN_CFG_INIT_RF_LINE_TX_GAIN_DB_2G},
    {"ext_switch_isexist_2g",           WLAN_CFG_INIT_EXT_SWITCH_ISEXIST_2G},
    {"ext_pa_isexist_2g",               WLAN_CFG_INIT_EXT_PA_ISEXIST_2G},
    {"ext_lna_isexist_2g",              WLAN_CFG_INIT_EXT_LNA_ISEXIST_2G},
    {"lna_on2off_time_ns_2g",           WLAN_CFG_INIT_LNA_ON2OFF_TIME_NS_2G},
    {"lna_off2on_time_ns_2g",           WLAN_CFG_INIT_LNA_OFF2ON_TIME_NS_2G},
    {"rf_line_rx_gain_db_5g",           WLAN_CFG_INIT_RF_LINE_RX_GAIN_DB_5G},
    {"lna_gain_db_5g",                  WLAN_CFG_INIT_LNA_GAIN_DB_5G},
    {"rf_line_tx_gain_db_5g",           WLAN_CFG_INIT_RF_LINE_TX_GAIN_DB_5G},
    {"ext_switch_isexist_5g",           WLAN_CFG_INIT_EXT_SWITCH_ISEXIST_5G},
    {"ext_pa_isexist_5g",               WLAN_CFG_INIT_EXT_PA_ISEXIST_5G},
    {"ext_lna_isexist_5g",              WLAN_CFG_INIT_EXT_LNA_ISEXIST_5G},
    {"lna_on2off_time_ns_5g",           WLAN_CFG_INIT_LNA_ON2OFF_TIME_NS_5G},
    {"lna_off2on_time_ns_5g",           WLAN_CFG_INIT_LNA_OFF2ON_TIME_NS_5G},
    /* SCAN */
    {"random_mac_addr_scan",            WLAN_CFG_INIT_RANDOM_MAC_ADDR_SCAN},
    /* 11AC2G */
    {"11ac2g_enable",                   WLAN_CFG_INIT_11AC2G_ENABLE},
    {"disable_capab_2ght40",            WLAN_CFG_INIT_DISABLE_CAPAB_2GHT40},
    {"dual_antenna_enable",             WLAN_CFG_INIT_DUAL_ANTENNA_ENABLE}, /* 双天线开关 */
    /* sta keepalive cnt th*/
    {"sta_keepalive_cnt_th",            WLAN_CFG_INIT_STA_KEEPALIVE_CNT_TH}, /* 动态功率校准 */
    {"far_dist_pow_gain_switch",        WLAN_CFG_INIT_FAR_DIST_POW_GAIN_SWITCH},
    {"far_dist_dsss_scale_promote_switch",      WLAN_CFG_INIT_FAR_DIST_DSSS_SCALE_PROMOTE_SWITCH},
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    {"beacon_country_ie_switch",        WLAN_CFG_INIT_BEACON_COUNTRY_IE_SWITCH},
#endif
    {"delta_cca_ed_high_20th_2g",       WLAN_CFG_INIT_DELTA_CCA_ED_HIGH_20TH_2G},
    {"delta_cca_ed_high_40th_2g",       WLAN_CFG_INIT_DELTA_CCA_ED_HIGH_40TH_2G},
    {"delta_cca_ed_high_20th_5g",       WLAN_CFG_INIT_DELTA_CCA_ED_HIGH_20TH_5G},
    {"delta_cca_ed_high_40th_5g",       WLAN_CFG_INIT_DELTA_CCA_ED_HIGH_40TH_5G},
    {"obss_rssi_th",                    WLAN_CFG_INIT_OBSS_RSSI_TH},
    {"dyn_bw_enable",                   WLAN_CFG_INIT_DYN_BW_ENABLE},
    {OAL_PTR_NULL, 0}
};//注意顺序与宏定义顺序对应

OAL_STATIC wlan_cfg_cmd g_ast_nvram_config_ini[NVRAM_PARAMS_INDEX_BUTT] =
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
    {"nvram_params18",                    NVRAM_PARAMS_INDEX_18},
    {"nvram_params19",                    NVRAM_PARAMS_INDEX_19},
    {"nvram_params20",                    NVRAM_PARAMS_INDEX_20},
    {"nvram_params21",                    NVRAM_PARAMS_INDEX_21},
    {"nvram_params22",                    NVRAM_PARAMS_INDEX_22},
    {"nvram_params23",                    NVRAM_PARAMS_INDEX_23},
    {"nvram_params24",                    NVRAM_PARAMS_INDEX_24},
    {"nvram_params25",                    NVRAM_PARAMS_INDEX_25},
    {"nvram_params26",                    NVRAM_PARAMS_INDEX_26},
    {"nvram_params27",                    NVRAM_PARAMS_INDEX_27},
    {"nvram_params28",                    NVRAM_PARAMS_INDEX_28},
    {"nvram_params29",                    NVRAM_PARAMS_INDEX_29},
    {"nvram_params30",                    NVRAM_PARAMS_INDEX_30},
    {"nvram_params31",                    NVRAM_PARAMS_INDEX_31},
    {"nvram_params32",                    NVRAM_PARAMS_INDEX_32},
    {"nvram_params33",                    NVRAM_PARAMS_INDEX_33},
    {"nvram_params34",                    NVRAM_PARAMS_INDEX_34},
    {"nvram_params35",                    NVRAM_PARAMS_INDEX_35},
    {"nvram_params36",                    NVRAM_PARAMS_INDEX_36},
    {"nvram_params37",                    NVRAM_PARAMS_INDEX_37},
    {"nvram_params38",                    NVRAM_PARAMS_INDEX_38},
    {"nvram_params39",                    NVRAM_PARAMS_INDEX_39},
    {"nvram_params40",                    NVRAM_PARAMS_INDEX_40},
    {"nvram_params41",                    NVRAM_PARAMS_INDEX_41},
    {"nvram_params42",                    NVRAM_PARAMS_INDEX_42},
    {"nvram_params43",                    NVRAM_PARAMS_INDEX_43},
    {"nvram_params44",                    NVRAM_PARAMS_INDEX_44},
    {"nvram_params45",                    NVRAM_PARAMS_INDEX_45},
    {"nvram_params46",                    NVRAM_PARAMS_INDEX_46},
    {"nvram_params47",                    NVRAM_PARAMS_INDEX_47},
    {"nvram_params48",                    NVRAM_PARAMS_INDEX_48},
    {"nvram_params49",                    NVRAM_PARAMS_INDEX_49},
    {"nvram_params50",                    NVRAM_PARAMS_INDEX_50},
    {"nvram_params51",                    NVRAM_PARAMS_INDEX_51},
};


OAL_STATIC oal_void original_value_for_nvram_params(oal_void)
{
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_0]  =0x00480096;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_1]  =0x00480096;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_2]  =0x0062008C;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_3]  =0x0062008C;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_4]  =0x0062008C;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_5]  =0x00580078;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_6]  =0x00580078;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_7]  =0x00660096;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_8]  =0x00660096;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_9]  =0x00660096;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_10] =0x004E0082;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_11] =0x004E006E;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_12] =0x004E006E;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_13] =0x00630096;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_14] =0x00630096;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_15] =0x004F0082;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_16] =0x00450064;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_17] =0x00450064;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_18] =0x00450064;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_19] =0x00450064;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_20] =0x00720064;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_21] =0x00870082;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_22] =0x00870082;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_23] =0x00770078;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_24] =0x006A0064;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_25] =0x006A0064;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_26] =0x00890082;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_27] =0x00890082;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_28] =0x00880082;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_29] =0x006C006E;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_30] =0x006C006E;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_31] =0x006C0064;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_32] =0x00910082;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_33] =0x00910082;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_34] =0x008E0082;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_35] =0x007F006E;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_36] =0x007F006E;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_37] =0x00650064;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_38] =0x00790064;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_39] =0x00860082;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_40] =0x00860082;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_41] =0x00760078;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_42] =0x00690064;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_43] =0x00690064;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_44] =0x0052005A;
    g_al_nvram_init_params[NVRAM_PARAMS_INDEX_45] =0x00000000;
}
 /*lint -efunc(569,original_value_for_dts_params) */

OAL_STATIC oal_void original_value_for_dts_params(oal_void)
{
#ifdef _PRE_WLAN_RF_CALI_1151V2
    /* hi1151v2 校准定制化初始值 */
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_0]        = 0x90221;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_1]        = 0x90221;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_2]        = 0x90221;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_3]        = 0x90221;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_4]        = 0x90221;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_5]        = 0x90221;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_6]        = 0x90221;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_7]        = 0x90221;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_8]        = 0x90221;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_9]        = 0x90221;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_10]       = 0x90221;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_11]       = 0x90221;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_12]       = 0x90221;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_13]       = 0x90221;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_14]       = 0x90221;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_15]       = 0x90221;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_16]       = 0x90221;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_17]       = 0x90221;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_18]       = 0x90221;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_19]       = 0x90221;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_20]       = 0x90221;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_21]       = 0x90221;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_22]       = 0x90221;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_23]       = 0x90221;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_24]       = 0x90221;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_25]       = 0x90221;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_26]       = 0x90221;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_27]       = 0x90221;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_28]       = 0x90221;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_29]       = 0x90221;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_30]       = 0x90221;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_1151V2_CALI_SEQ_31]       = 0x90221;

    /* hi1151v2 动态校准开关*/
    g_al_dts_params_etc[WLAN_CFG_DTS_DYN_CALI_DSCR_ITERVL]          = 0x0;
#endif

    /* 校准 */
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN1]     = 0x228F1D70;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN2]     = 0x228F228F;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN3]     = 0x20002148;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN4]     = 0x1EB81D70;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN5]     = 0x1D70199A;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN6]     = 0x1C291852;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN7]     = 0x1D701852;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN8]     = 0x1D70199A;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN9]     = 0x1D701AE1;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN10]    = 0x1C291D70;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN11]    = 0x1D701EB8;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN12]    = 0x1D701C29;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN13]    = 0x1C2915C3;

    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_B40_VAL_CHAN1] = 0x228F1D70;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_B40_VAL_CHAN2] = 0x228F228F;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_B40_VAL_CHAN3] = 0x20002148;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_B40_VAL_CHAN4] = 0x1EB81D70;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_B40_VAL_CHAN5] = 0x1D70199A;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_B40_VAL_CHAN6] = 0x1C291852;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_B40_VAL_CHAN7] = 0x1D701852;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_B40_VAL_CHAN8] = 0x1D70199A;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_B40_VAL_CHAN9] = 0x1D701AE1;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_B40_VAL_CHAN10]= 0x1C291D70;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_B40_VAL_CHAN11]= 0x1D701EB8;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_B40_VAL_CHAN12]= 0x1D701C29;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_B40_VAL_CHAN13]= 0x1C2915C3;

    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND1]     = 0x23D723D7;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND2]     = 0x266623D7;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND3]     = 0x23D7228F;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND4]     = 0x23D72148;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND5]     = 0x228F2000;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND6]     = 0x228F2000;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND7]     = 0x228F2000;
    g_al_dts_params_etc[WLAN_CFG_DTS_CALI_TONE_AMP_GRADE]                   = 0;
    g_al_dts_params_etc[WLAN_CFG_DTS_MIMO_POW_ADJUST]                       = 0;

    /* 动态校准 */
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C0_CHAN1]     = 0xE2FBFDF9;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C0_CHAN2]     = 0xDEFD05F9;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C0_CHAN3]     = 0xE40509F9;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C0_CHAN4]     = 0xE00000FB;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C0_CHAN5]     = 0xE0FBFDFB;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C0_CHAN6]     = 0xE7FEF9F7;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C0_CHAN7]     = 0xE70404F7;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C0_CHAN8]     = 0xE70205F6;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C0_CHAN9]     = 0xE702FDF0;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C0_CHAN10]    = 0xE70705F6;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C0_CHAN11]    = 0xEC0F0D07;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C0_CHAN12]    = 0xD6F4F600;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C0_CHAN13]    = 0xD3EFEE00;

    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C1_CHAN1]     = 0xE70A09FA;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C1_CHAN2]     = 0xEA0A00F3;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C1_CHAN3]     = 0xE20A00F3;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C1_CHAN4]     = 0xE20A00F2;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C1_CHAN5]     = 0xE20500F4;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C1_CHAN6]     = 0xE205FBF6;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C1_CHAN7]     = 0xE20505F7;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C1_CHAN8]     = 0xE20705F6;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C1_CHAN9]     = 0xE20500EF;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C1_CHAN10]    = 0xE20500EF;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C1_CHAN11]    = 0xDDFDFBED;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C1_CHAN12]    = 0xDDFDF9E7;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_C1_CHAN13]    = 0xDDF8F6EC;


    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C0_CHAN1]     = 0xE2FBFDF9;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C0_CHAN2]     = 0xDEFD05F9;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C0_CHAN3]     = 0xE40509F9;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C0_CHAN4]     = 0xE00000FB;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C0_CHAN5]     = 0xE0FBFDFB;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C0_CHAN6]     = 0xE7FEF9F7;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C0_CHAN7]     = 0xE70404F7;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C0_CHAN8]     = 0xE70205F6;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C0_CHAN9]     = 0xE702FDF0;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C0_CHAN10]    = 0xE70705F6;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C0_CHAN11]    = 0xEC0F0D07;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C0_CHAN12]    = 0xD6F4F600;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C0_CHAN13]    = 0xD3EFEE00;

    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C1_CHAN1]     = 0xE70A09FA;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C1_CHAN2]     = 0xEA0A00F3;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C1_CHAN3]     = 0xE20A00F3;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C1_CHAN4]     = 0xE20A00F2;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C1_CHAN5]     = 0xE20500F4;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C1_CHAN6]     = 0xE205FBF6;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C1_CHAN7]     = 0xE20505F7;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C1_CHAN8]     = 0xE20705F6;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C1_CHAN9]     = 0xE20500EF;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C1_CHAN10]    = 0xE20500EF;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C1_CHAN11]    = 0xDDFDFBED;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C1_CHAN12]    = 0xDDFDF9E7;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_2G_B40_C1_CHAN13]    = 0xDDF8F6EC;

    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_5G_C0_BAND1]     = 0x00000000;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_5G_C0_BAND2]     = 0x0000120C;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_5G_C0_BAND3]     = 0x0000110C;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_5G_C0_BAND4]     = 0x05051811;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_5G_C0_BAND5]     = 0x0002140F;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_5G_C0_BAND6]     = 0x0004110C;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_5G_C0_BAND7]     = 0x060E1C14;

    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_5G_C1_BAND1]     = 0x00000000;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_5G_C1_BAND2]     = 0xF6F807FD;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_5G_C1_BAND3]     = 0xF3F40700;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_5G_C1_BAND4]     = 0xF6F80902;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_5G_C1_BAND5]     = 0xF4F60A00;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_5G_C1_BAND6]     = 0xF1F20700;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_5G_C1_BAND7]     = 0xF8F608FD;

    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_SINGLE_STONE_TXPOW_SPEC]      = 0x6166a5a0;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_EQUIP_TXPOW_LUT]                   = 0x00c8a078;

    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_POW_OFFSET_2G_C0]                 = 0x00000000;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_POW_OFFSET_2G_C1]                 = 0x00000000;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_POW_OFFSET_5G_C0_1]               = 0x00000000;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_POW_OFFSET_5G_C1_1]               = 0x00000000;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_POW_OFFSET_5G_C0_2]               = 0x00000000;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_POW_OFFSET_5G_C1_2]               = 0x00000000;

    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_2G_BAND1_FST]     = 0x00000000;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_2G_BAND1_SND]     = 0xF6F807FD;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_2G_BAND1_TRD]     = 0xF3F40700;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_2G_BAND2_FST]     = 0x00000000;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_2G_BAND2_SND]     = 0xF6F807FD;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_2G_BAND2_TRD]     = 0xF3F40700;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_2G_BAND3_FST]     = 0x00000000;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_2G_BAND3_SND]     = 0xF6F807FD;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_2G_BAND3_TRD]     = 0xF3F40700;

    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND1_FST]     = 0x00000000;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND1_SND]     = 0xF6F807FD;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND1_TRD]     = 0xF3F40700;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND2_FST]     = 0x00000000;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND2_SND]     = 0xF6F807FD;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND2_TRD]     = 0xF3F40700;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND3_FST]     = 0x00000000;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND3_SND]     = 0xF6F807FD;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND3_TRD]     = 0xF3F40700;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND4_FST]     = 0x00000000;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND4_SND]     = 0xF6F807FD;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND4_TRD]     = 0xF3F40700;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND5_FST]     = 0x00000000;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND5_SND]     = 0xF6F807FD;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND5_TRD]     = 0xF3F40700;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND6_FST]     = 0x00000000;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND6_SND]     = 0xF6F807FD;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND6_TRD]     = 0xF3F40700;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND7_FST]     = 0x00000000;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND7_SND]     = 0xF6F807FD;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_POLYNOMIAL_PARA_5G_BAND7_TRD]     = 0xF3F40700;

    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_DBB_2G_PARA1]     = 0x8BFDAE11;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_DBB_2G_PARA2]     = 0xB41479F1;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_DBB_2G_PARA3]     = 0x7AF288FC;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_DBB_2G_PARA4]     = 0xAD10BE18;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_DBB_2G_PARA5]     = 0x7FF58AFD;

    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_DBB_5G_PARA1]     = 0xA6EAD701;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_DBB_5G_PARA2]     = 0x73CA94E0;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_DBB_5G_PARA3]     = 0xC8FAE506;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_DBB_5G_PARA4]     = 0x9FE6B3F0;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_DBB_5G_PARA5]     = 0xBFF6D901;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_DBB_5G_PARA6]     = 0xD60098E2;
    g_al_dts_params_etc[WLAN_CFG_DTS_DYNC_CALI_OFDM_DBB_5G_PARA7]     = 0x95E1BEF6;

    /* DAC/LPF */
    g_al_dts_params_etc[WLAN_CFG_DTS_DAC_LPF_GAIN_2G_LEGACY_RATE_1]  = 0x11111111;
    g_al_dts_params_etc[WLAN_CFG_DTS_DAC_LPF_GAIN_2G_LEGACY_RATE_2]  = 0x00000000;
    g_al_dts_params_etc[WLAN_CFG_DTS_DAC_LPF_GAIN_2G_LEGACY_RATE_3]  = 0x21212121;
    g_al_dts_params_etc[WLAN_CFG_DTS_DAC_LPF_GAIN_2G_LEGACY_RATE_4]  = 0x21212121;
    g_al_dts_params_etc[WLAN_CFG_DTS_DAC_LPF_GAIN_5G_LEGACY_RATE_1]  = 0x00000000;
    g_al_dts_params_etc[WLAN_CFG_DTS_DAC_LPF_GAIN_5G_LEGACY_RATE_2]  = 0x00000000;
    g_al_dts_params_etc[WLAN_CFG_DTS_DAC_LPF_GAIN_2G_11AC_HT20_1]    = 0x11111111;
    g_al_dts_params_etc[WLAN_CFG_DTS_DAC_LPF_GAIN_2G_11AC_HT20_2]    = 0x21111111;
    g_al_dts_params_etc[WLAN_CFG_DTS_DAC_LPF_GAIN_2G_11AC_HT40_1]    = 0x11111111;
    g_al_dts_params_etc[WLAN_CFG_DTS_DAC_LPF_GAIN_2G_11AC_HT40_2]    = 0x21210101;
    g_al_dts_params_etc[WLAN_CFG_DTS_DAC_LPF_GAIN_5G_11AC_HT20_1]    = 0x00000000;
    g_al_dts_params_etc[WLAN_CFG_DTS_DAC_LPF_GAIN_5G_11AC_HT20_2]    = 0x00000000;
    g_al_dts_params_etc[WLAN_CFG_DTS_DAC_LPF_GAIN_5G_11AC_HT40_1]    = 0x00000000;
    g_al_dts_params_etc[WLAN_CFG_DTS_DAC_LPF_GAIN_5G_11AC_HT40_2]    = 0x10100000;
    g_al_dts_params_etc[WLAN_CFG_DTS_DAC_LPF_GAIN_5G_11AC_HT40_3]    = 0x00100000;
    g_al_dts_params_etc[WLAN_CFG_DTS_DAC_LPF_GAIN_5G_11AC_HT80_1]    = 0x10000000;
    g_al_dts_params_etc[WLAN_CFG_DTS_DAC_LPF_GAIN_5G_11AC_HT80_2]    = 0x10100000;
    g_al_dts_params_etc[WLAN_CFG_DTS_DAC_LPF_GAIN_5G_11AC_HT80_3]    = 0x00000000;

    /* FCC认证 */
    g_al_dts_params_etc[WLAN_CFG_DTS_BAND_EDGE_LIMIT_2G_11G_TXPWR]                      = 150;
    g_al_dts_params_etc[WLAN_CFG_DTS_BAND_EDGE_LIMIT_2G_11N_HT20_TXPWR]                 = 150;
    g_al_dts_params_etc[WLAN_CFG_DTS_BAND_EDGE_LIMIT_2G_11N_HT40_TXPWR]                 = 150;
    g_al_dts_params_etc[WLAN_CFG_DTS_BAND_EDGE_LIMIT_5G_11A_HT20_VHT20_TXPWR]           = 150;
    g_al_dts_params_etc[WLAN_CFG_DTS_BAND_EDGE_LIMIT_5G_HT40_VHT40_TXPWR]               = 150;
    g_al_dts_params_etc[WLAN_CFG_DTS_BAND_EDGE_LIMIT_5G_VHT80_TXPWR]                    = 150;
    g_al_dts_params_etc[WLAN_CFG_DTS_BAND_EDGE_LIMIT_2G_11G_DBB_SCALING]                = 0x68;
    g_al_dts_params_etc[WLAN_CFG_DTS_BAND_EDGE_LIMIT_2G_11N_HT20_DBB_SCALING]           = 0x62;
    g_al_dts_params_etc[WLAN_CFG_DTS_BAND_EDGE_LIMIT_2G_11N_HT40_DBB_SCALING]           = 0x62;
    g_al_dts_params_etc[WLAN_CFG_DTS_BAND_EDGE_LIMIT_5G_11A_HT20_VHT20_DBB_SCALING]     = 0x68;/* 待定 */
    g_al_dts_params_etc[WLAN_CFG_DTS_BAND_EDGE_LIMIT_5G_HT40_VHT40_DBB_SCALING]         = 0x68;/* 待定 */
    g_al_dts_params_etc[WLAN_CFG_DTS_BAND_EDGE_LIMIT_5G_VHT80_DBB_SCALING]              = 0x68;/* 待定 */

    /* RF PLL */
    g_al_dts_params_etc[WLAN_CFG_DTS_RF_PLL_PPM_2G]                                     = -57;
    g_al_dts_params_etc[WLAN_CFG_DTS_RF_PLL_PPM_5G]                                     = -57;
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
    /* AP开tx amsdu */
    g_al_host_init_params_etc[WLAN_CFG_INIT_AP_TX_AMSDU]                       = 0;
     /* AP开rx amsdu */
    g_al_host_init_params_etc[WLAN_CFG_INIT_AP_RX_AMSDU]                       = 0;
    /* 低功耗 */
    g_al_host_init_params_etc[WLAN_CFG_INIT_POWERMGMT_SWITCH]                  = 0;
    /* 可维可测 */
    /* 日志级别 */
    g_al_host_init_params_etc[WLAN_CFG_INIT_LOGLEVEL]                          = OAM_LOG_LEVEL_WARNING;
    /* 2G RF前端 */
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_2G_BAND1_MULT4]    = -12;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_2G_BAND1_MULT10]   = -30;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_2G_BAND2_MULT4]    = -12;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_2G_BAND2_MULT10]   = -30;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_2G_BAND3_MULT4]    = -12;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_2G_BAND3_MULT10]   = -30;
    /* 5G RF前端 */
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_5G_BAND1_MULT4]    = -8;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_5G_BAND1_MULT10]   = -20;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_5G_BAND2_MULT4]    = -8;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_5G_BAND2_MULT10]   = -20;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_5G_BAND3_MULT4]    = -8;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_5G_BAND3_MULT10]   = -20;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_5G_BAND4_MULT4]    = -8;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_5G_BAND4_MULT10]   = -20;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_5G_BAND5_MULT4]    = -8;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_5G_BAND5_MULT10]   = -20;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_5G_BAND6_MULT4]    = -8;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_5G_BAND6_MULT10]   = -20;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_5G_BAND7_MULT4]    = -8;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_LINE_TXRX_GAIN_DB_5G_BAND7_MULT10]   = -20;

    /* fem */
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_LINE_RX_GAIN_DB_2G]             = -12;
    g_al_host_init_params_etc[WLAN_CFG_INIT_LNA_GAIN_DB_2G]                    = 20;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_LINE_TX_GAIN_DB_2G]             = -12;
    g_al_host_init_params_etc[WLAN_CFG_INIT_EXT_SWITCH_ISEXIST_2G]             = 0;
    g_al_host_init_params_etc[WLAN_CFG_INIT_EXT_PA_ISEXIST_2G]                 = 0;
    g_al_host_init_params_etc[WLAN_CFG_INIT_EXT_LNA_ISEXIST_2G]                = 0;
    g_al_host_init_params_etc[WLAN_CFG_INIT_LNA_ON2OFF_TIME_NS_2G]             = 630;
    g_al_host_init_params_etc[WLAN_CFG_INIT_LNA_OFF2ON_TIME_NS_2G]             = 320;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_LINE_RX_GAIN_DB_5G]             = -12;
    g_al_host_init_params_etc[WLAN_CFG_INIT_LNA_GAIN_DB_5G]                    = 20;
    g_al_host_init_params_etc[WLAN_CFG_INIT_RF_LINE_TX_GAIN_DB_5G]             = -12;
    g_al_host_init_params_etc[WLAN_CFG_INIT_EXT_SWITCH_ISEXIST_5G]             = 1;
    g_al_host_init_params_etc[WLAN_CFG_INIT_EXT_PA_ISEXIST_5G]                 = 1;
    g_al_host_init_params_etc[WLAN_CFG_INIT_EXT_LNA_ISEXIST_5G]                = 1;
    g_al_host_init_params_etc[WLAN_CFG_INIT_LNA_ON2OFF_TIME_NS_5G]             = 630;
    g_al_host_init_params_etc[WLAN_CFG_INIT_LNA_OFF2ON_TIME_NS_5G]             = 320;

    /* SCAN */
    g_al_host_init_params_etc[WLAN_CFG_INIT_RANDOM_MAC_ADDR_SCAN]              = 0;
    /* 11AC2G */
    g_al_host_init_params_etc[WLAN_CFG_INIT_11AC2G_ENABLE]                     = 1;
    g_al_host_init_params_etc[WLAN_CFG_INIT_DISABLE_CAPAB_2GHT40]              = 0;
    g_al_host_init_params_etc[WLAN_CFG_INIT_DUAL_ANTENNA_ENABLE]               = 0;
    /* sta keepalive cnt th*/
    g_al_host_init_params_etc[WLAN_CFG_INIT_STA_KEEPALIVE_CNT_TH]              = 3;
    g_al_host_init_params_etc[WLAN_CFG_INIT_FAR_DIST_POW_GAIN_SWITCH]          = 1;
    g_al_host_init_params_etc[WLAN_CFG_INIT_FAR_DIST_DSSS_SCALE_PROMOTE_SWITCH]     = 1;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    /* beacon country ie switch */
    g_al_host_init_params_etc[WLAN_CFG_INIT_BEACON_COUNTRY_IE_SWITCH]          = 1;
#endif
    g_al_host_init_params_etc[WLAN_CFG_INIT_DELTA_CCA_ED_HIGH_20TH_2G]         = 0;
    g_al_host_init_params_etc[WLAN_CFG_INIT_DELTA_CCA_ED_HIGH_40TH_2G]         = 0;
    g_al_host_init_params_etc[WLAN_CFG_INIT_DELTA_CCA_ED_HIGH_20TH_5G]         = 0;
    g_al_host_init_params_etc[WLAN_CFG_INIT_DELTA_CCA_ED_HIGH_40TH_5G]         = 0;
    g_al_host_init_params_etc[WLAN_CFG_INIT_OBSS_RSSI_TH]                      = 0;
    g_al_host_init_params_etc[WLAN_CFG_INIT_DYN_BW_ENABLE]                     = 0;
}


regdomain_enum hwifi_get_regdomain_from_country_code(const countrycode_t country_code, oal_bool_enum_uint8 en_real_regdomain)
{
    regdomain_enum  en_regdomain = REGDOMAIN_COMMON;
    oal_uint32      ul_table_idx = 0;

    while (g_ast_country_info_table[ul_table_idx].en_regdomain != REGDOMAIN_COUNT)
    {
        if (0 == oal_memcmp(country_code, g_ast_country_info_table[ul_table_idx].auc_country_code, COUNTRY_CODE_LEN))
        {
            if (en_real_regdomain)
            {
                /* HERA产品的HILINK接口需要返回真实的区域码 */
                en_regdomain = g_ast_country_info_table[ul_table_idx].en_regdomain;
            }
            else
            {
                /* ONT产品只区分FCC和非FCC */
                en_regdomain = (g_ast_country_info_table[ul_table_idx].en_regdomain == REGDOMAIN_FCC) ? REGDOMAIN_FCC : REGDOMAIN_COMMON;
            }
            break;
        }
        ++ul_table_idx;
    }

    return en_regdomain;
}


int32 hwifi_is_regdomain_changed_etc(const countrycode_t country_code_old, const countrycode_t country_code_new)
{
    return hwifi_get_regdomain_from_country_code(country_code_old, OAL_FALSE) != hwifi_get_regdomain_from_country_code(country_code_new, OAL_FALSE);
}


OAL_STATIC int32 hwifi_get_plat_tag_from_country_code(const countrycode_t country_code)
{
    regdomain_enum  en_regdomain;
    int32           table_idx = 0;

    en_regdomain = hwifi_get_regdomain_from_country_code(country_code, OAL_FALSE);

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


int32 hwifi_fetch_ori_caldata_etc(uint8* auc_caldata, int32 l_nvm_len)
{
    int32 l_ret = INI_FAILED;
    int32 l_cfg_id;
    int32 aul_nvram_params[NVRAM_PARAMS_INDEX_BUTT]={0};

    if (l_nvm_len != HISI_CUST_NVRAM_LEN)
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "hwifi_fetch_ori_caldata_etc atcmd[nv_len:%d] and plat_ini[nv_len:%d] model have different nvm lenth!!",
                        l_nvm_len, HISI_CUST_NVRAM_LEN);
        return INI_FAILED;
    }

    oal_memset(auc_caldata, 0x00, HISI_CUST_NVRAM_LEN);

    for (l_cfg_id = NVRAM_PARAMS_INDEX_0; l_cfg_id < NVRAM_PARAMS_INDEX_BUTT; l_cfg_id++)
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


OAL_STATIC int32 hwifi_config_init_nvram(void)
{
    int32 l_ret = INI_FAILED;
    int32 l_cfg_id;
    int32 aul_nvram_params[NVRAM_PARAMS_INDEX_BUTT]={0};
    int32 l_plat_tag;

    oal_memset(g_aus_nv_params, 0x00, sizeof(g_aus_nv_params));
    l_plat_tag = hwifi_get_plat_tag_from_country_code((uint8*)hwifi_get_country_code_etc());
    OAM_WARNING_LOG1(0, OAM_SF_ANY, "hwifi_config_init_nvram plat_tag:0x%2x!", l_plat_tag);

    /* read nvm failed or data not exist or country_code updated, read ini:cust_spec > cust_common > default */
    /* find plat tag */

    for (l_cfg_id = NVRAM_PARAMS_INDEX_0; l_cfg_id < NVRAM_PARAMS_INDEX_BUTT; l_cfg_id++)
    {
        l_ret = get_cust_conf_int32_etc(l_plat_tag, g_ast_nvram_config_ini[l_cfg_id].name, &aul_nvram_params[l_cfg_id]);
        //OAM_WARNING_LOG2(0, OAM_SF_ANY, "{hwifi_config_init_nvram::l_cfg_id[%d], aul_nvram_params[l_cfg_id]=0x%x!}\r\n", l_cfg_id, aul_nvram_params[l_cfg_id]);
        if(INI_FAILED == l_ret)
        {
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "hwifi_config_init_nvram read id[%d] from ini failed!", l_cfg_id);

            /* 读取失败时将数组置零，防止下发至device */
            oal_memcopy(g_aus_nv_params, g_al_nvram_init_params, sizeof(g_aus_nv_params));
            return INI_FAILED;
        }
    }

    OAM_INFO_LOG0(0, OAM_SF_ANY, "hwifi_config_init_nvram read from ini success!");
    oal_memcopy(g_aus_nv_params, aul_nvram_params, sizeof(g_aus_nv_params));

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

        OAM_WARNING_LOG2(0, OAM_SF_CFG, "hwifi_config_init_private_custom got priv_var id[%d]val[%d]!", l_cfg_id, g_al_priv_cust_params[l_cfg_id].l_val);
    }

    OAM_WARNING_LOG0(0, OAM_SF_CFG, "hwifi_config_init_private_custom read from ini success!");

    return INI_SUCC;
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
        default:
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "hwifi_config_init_etc tag number[0x%x] not correct!", cus_tag);
            return INI_FAILED;
    }

    for(l_cfg_id = 0; l_cfg_id < l_wlan_cfg_butt; l_cfg_id++)
    {
        /* 获取ini的配置值 */
        l_ret = get_cust_conf_int32_etc(INI_MODU_WIFI, pgast_wifi_config[l_cfg_id].name, &l_cfg_value);

        if (INI_FAILED == l_ret)
        {
            OAM_WARNING_LOG2(0, OAM_SF_ANY, "hwifi_config_init_etc read ini file failed cfg_id[%d]tag[%d]!", l_cfg_id, cus_tag);
            continue;
        }

        l_ori_val = pgal_params[l_cfg_id];
        pgal_params[l_cfg_id] = l_cfg_value;
        OAM_WARNING_LOG4(0, OAM_SF_ANY, "hwifi_config_init_etc [id:%d tag:%d] changed from [%d]to[%d]", l_cfg_id, cus_tag, l_ori_val, pgal_params[l_cfg_id]);
    }

    return INI_SUCC;
}
#if 0

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
#endif

int32 hwifi_get_mac_addr_etc(uint8 *puc_buf)
{
    return INI_SUCC;
}

/*lint -e661*/ /*lint -e662*/
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
/*lint +e661*/ /*lint +e662*/

oal_int32 hwifi_get_init_priv_value(oal_int32 l_cfg_id, oal_int32 *pl_priv_value)
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


int8* hwifi_get_country_code_etc(void)
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

    return;
}

uint16 *hwifi_get_nvram_params_etc(void)
{
    return g_aus_nv_params;
}


int hwifi_get_cfg_params(void)
{
    int32                  l_cfg_id = 0;

    OAL_IO_PRINT("\nhwifi_get_cfg_params\n");

    //CUS_TAG_INI
    for(l_cfg_id = 0; l_cfg_id < WLAN_CFG_INIT_BUTT; ++l_cfg_id)
    {
        OAL_IO_PRINT("%s \t [config:%d]\n", g_ast_wifi_config_cmds[l_cfg_id].name,
            g_al_host_init_params_etc[l_cfg_id]);
    }

    //CUS_TAG_NV
    for(l_cfg_id = 0; l_cfg_id < NUM_OF_NV_MAX_TXPOWER; ++l_cfg_id)
    {
        OAL_IO_PRINT("%s%d \t [config:0x%x  0x%x]\n", "nvram_params",l_cfg_id,
            g_aus_nv_params[2*l_cfg_id], g_aus_nv_params[2*l_cfg_id + 1]);
    }
    OAL_IO_PRINT("%s%d \t [config:0x%x]\n", "nvram_params", NUM_OF_NV_MAX_TXPOWER,
        g_aus_nv_params[NUM_OF_NV_PARAMS-1]);

    //CUS_TAG_DTS
    for(l_cfg_id = 0; l_cfg_id < WLAN_CFG_DTS_BUTT; ++l_cfg_id)
    {
        OAL_IO_PRINT("%s \t [config:%d]\n", g_ast_wifi_config_dts[l_cfg_id].name,
            g_al_dts_params_etc[l_cfg_id]);
    }
    return INI_SUCC;
}



char* hwifi_get_chip_id_cfg(oal_uint8 uc_chip_id)
{
    /*Note:declaration of symbol "l_ret" hides symbol "l_ret"*/
    /*lint -e578*/
    int32 l_ret;
    char  auc_var_name[16]={0};
    if(uc_chip_id >= MAX_CHIPID_COUNT)
    {
        return OAL_PTR_NULL;
    }

    OAL_SPRINTF(auc_var_name, 16, "chip%d_cfg", uc_chip_id);

    OAL_MEMZERO(g_chipid_cfg[uc_chip_id],CHIPID_CFG_LEN);

    /* 获取chip id配置信息 */
    l_ret = get_cust_conf_string_etc(INI_MODU_WIFI, auc_var_name, g_chipid_cfg[uc_chip_id], CHIPID_CFG_LEN-1);

    if(INI_FAILED == l_ret)
    {
        OAL_IO_PRINT("hwifi_get_chip_id_cfg  read chip%d_cfg failed, check if it exists\r\n", uc_chip_id);
        return OAL_PTR_NULL;
    }
    return g_chipid_cfg[uc_chip_id];
}

#ifdef _PRE_WLAN_FEATURE_TPC_OVER_TEMP_PROTECT

oal_int32 hwifi_get_custom_over_temp(oal_uint8 *puc_temp)
{
#ifdef _PRE_WLAN_PRODUCT_1151V200
    oal_int32   l_ret;
    oal_int32   l_temp;

    if (OAL_PTR_NULL == puc_temp)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "hwifi_get_custom_over_temp :NULL  para !!");
        return OAL_FAIL;
    }
    l_ret = get_cust_conf_int32_etc(INI_MODU_WIFI, "software_over_temp_protect", &l_temp);
    if (0 == l_ret)
    {
        if ((l_temp > 0) && (l_temp < 255))
        {
            *puc_temp = (oal_uint8)l_temp;
        }
        else
        {
            *puc_temp = 0;
            l_ret = OAL_FAIL;
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "hwifi_get_custom_over_temp :invalid software over_temp[%d]!", l_temp);
        }
        puc_temp++;


        if (0 == l_ret)
        {
            l_ret = get_cust_conf_int32_etc(INI_MODU_WIFI, "hardware_over_temp_protect", &l_temp);
            if ((l_temp > 0) && (l_temp < 255))
            {
                *puc_temp = (oal_uint8)l_temp;
            }
            else
            {
                *puc_temp = 0;
                l_ret = OAL_FAIL;
                OAM_WARNING_LOG1(0, OAM_SF_ANY, "hwifi_get_custom_over_temp :invalid hardware over_temp[%d]!", l_temp);
            }
        }
    }

    return l_ret;
#else
    return OAL_SUCC;
#endif
}
#endif

#if defined (_PRE_WLAN_FEATURE_RX_AGGR_EXTEND) || defined (_PRE_FEATURE_WAVEAPP_CLASSIFY)

oal_void hwifi_get_wavap_mac_cfg(oal_void)
{
    oal_int32   l_ret;
    oal_uint8   i;
    char        auc_tmp_name[IXIA_MAC_ADDR_CFG_LEN]={0};
    char        auc_mac_name[IXIA_MAC_ADDR_CFG_LEN]={0};
    oal_uint8   uc_index = 0;

    //1. 获取mac地址  string
    for (i=0; i<MAX_IXIA_MAC_ADDR_COUNT; i++)
    {
        OAL_SPRINTF(auc_tmp_name, IXIA_MAC_ADDR_CFG_LEN, "ixia_mac_addr%d", i);
        l_ret = get_cust_conf_string_etc(INI_MODU_WIFI, auc_tmp_name, auc_mac_name, IXIA_MAC_ADDR_CFG_LEN-1);
        if(INI_FAILED == l_ret)
        {
            continue;
        }

        /* 将带冒号的mac地址转化为数字 */
        oal_strtoaddr(auc_mac_name, g_ixia_mac_addr_cfg[uc_index].auc_user_mac_addr);

        OAL_IO_PRINT("hwifi_get_wavap_mac_cfg:: g_ixia_mac_addr_cfg[%d]=[%02X:XX:XX:XX:%02X:%02X]\n", uc_index,
            g_ixia_mac_addr_cfg[uc_index].auc_user_mac_addr[0],g_ixia_mac_addr_cfg[uc_index].auc_user_mac_addr[4],g_ixia_mac_addr_cfg[uc_index].auc_user_mac_addr[5]);

        uc_index++;

    }

    g_us_ixia_mac_addr_num = uc_index;

    OAL_IO_PRINT("hwifi_get_wavap_mac_cfg:: g_us_mac_addr_num[%d]\n", g_us_ixia_mac_addr_num);

    return;
}


oal_bool_enum_uint8 hwifi_wavap_classify(oal_uint8 *puc_mac_addr)
{
    oal_uint8   i;
    oal_uint8   auc_user_mac_addr[WLAN_MAC_ADDR_LEN] = {0,0,0,0,0,0};

    if (!oal_compare_mac_addr(puc_mac_addr, auc_user_mac_addr))
    {
        return OAL_FALSE;
    }

    for (i=0; i<g_us_ixia_mac_addr_num; i++)
    {
        if (!oal_compare_mac_addr(puc_mac_addr, g_ixia_mac_addr_cfg[i].auc_user_mac_addr))
        {
            return OAL_TRUE;
        }
    }

    return OAL_FALSE;

}


#endif

#ifdef _PRE_WLAN_MAC_ADDR_EDCA_FIX

oal_void hwifi_get_edca_fix_cfg(oal_void)
{
    oal_int32   l_ret;
    oal_uint8   i;
    char        auc_tmp_name[MAC_ADDR_CFG_LEN]={0};
    char        auc_mac_name[MAC_ADDR_CFG_LEN]={0};
    oal_int8   *pc_str_1;
    oal_int8   *pc_str_2;
    oal_uint8   uc_up_edca = 0;
    oal_uint8   uc_down_edca = 0;
    oal_uint8   uc_index = 0;
    oal_uint32  ul_tmp = 0;

    //1. 获取mac地址  string
    for (i=0; i<MAX_EDCA_MAC_ADDR_COUNT; i++)
    {
        OAL_SPRINTF(auc_tmp_name, MAC_ADDR_CFG_LEN, "edca_mac_addr%d", i);
        l_ret = get_cust_conf_string_etc(INI_MODU_WIFI, auc_tmp_name, auc_mac_name, MAC_ADDR_CFG_LEN-1);
        if(INI_FAILED == l_ret)
        {
            continue;
        }

        //将逗号分隔的分离
        pc_str_1 = OAL_STRSTR(auc_mac_name, ",");
        if(OAL_PTR_NULL != pc_str_1)
        {
            *pc_str_1 = '\0';

            /* 前面已对分隔符赋值 \0, 可以直接通过auc_mac_name取分隔符之前的部分 */
            oal_strtoaddr(auc_mac_name, g_edca_mac_addr_cfg[uc_index].auc_user_mac_addr);
            //取分隔符后面的edca
            if(OAL_PTR_NULL == (pc_str_1 + 1))
            {
                return;
            }
            pc_str_2 = OAL_STRSTR(pc_str_1 + 1, ",");
            if(OAL_PTR_NULL == pc_str_2 || OAL_PTR_NULL == (pc_str_2 + 1))
            {
                return;
            }

            *pc_str_2 = '\0';

            //读取uplink edca值
            l_ret = OAL_SSCANF(pc_str_1 + 1, "%x", &ul_tmp);
            uc_up_edca = ul_tmp;
            if(l_ret < 0)
            {
                OAL_IO_PRINT("hwifi_get_edca_fix_cfg:: [%s] trans to int failed", pc_str_1 + 1);
                return;
            }

            //读取downlink edca值
            l_ret = OAL_SSCANF(pc_str_2 + 1, "%x", &ul_tmp);
            uc_down_edca = ul_tmp;
            if(l_ret < 0)
            {
                OAL_IO_PRINT("hwifi_get_edca_fix_cfg:: [%s] trans to int failed", pc_str_2 + 1);
                return;
            }

            g_edca_mac_addr_cfg[uc_index].uc_up_edca = uc_up_edca;
            g_edca_mac_addr_cfg[uc_index].uc_down_edca = uc_down_edca;

            OAL_IO_PRINT("hwifi_get_edca_fix_cfg:: g_edca_mac_addr_cfg[%d]=[%02X:XX:XX:XX:%02X:%02X] up[0x%x] down[0x%x]\n", uc_index,
                g_edca_mac_addr_cfg[uc_index].auc_user_mac_addr[0],g_edca_mac_addr_cfg[uc_index].auc_user_mac_addr[4],g_edca_mac_addr_cfg[uc_index].auc_user_mac_addr[5],
                g_edca_mac_addr_cfg[uc_index].uc_up_edca, g_edca_mac_addr_cfg[uc_index].uc_down_edca);

            uc_index++;

        }

    }

    g_us_mac_addr_num = uc_index;

    OAL_IO_PRINT("hwifi_get_edca_fix_cfg:: g_us_mac_addr_num[%d]\n", g_us_mac_addr_num);

    return;
}


oal_void* hwifi_get_up_down_edca(oal_uint8 *puc_mac_addr)
{
    oal_uint8   i;
    oal_uint8   auc_user_mac_addr[WLAN_MAC_ADDR_LEN] = {0,0,0,0,0,0};

    if (!oal_compare_mac_addr(puc_mac_addr, auc_user_mac_addr))
    {
        return 0;
    }

    for (i=0; i<g_us_mac_addr_num; i++)
    {
        if (!oal_compare_mac_addr(puc_mac_addr, g_edca_mac_addr_cfg[i].auc_user_mac_addr))
        {
            return (oal_void*)(&g_edca_mac_addr_cfg[i]);
        }
    }

    return OAL_PTR_NULL;

}
#endif

EXPORT_SYMBOL_GPL(g_st_cust_country_code_ignore_flag);
EXPORT_SYMBOL_GPL(g_st_wlan_customize_etc);
EXPORT_SYMBOL_GPL(hwifi_config_init_etc);
EXPORT_SYMBOL_GPL(hwifi_get_mac_addr_etc);
EXPORT_SYMBOL_GPL(hwifi_get_init_value_etc);
EXPORT_SYMBOL_GPL(hwifi_get_country_code_etc);
EXPORT_SYMBOL_GPL(hwifi_get_nvram_params_etc);
EXPORT_SYMBOL_GPL(hwifi_fetch_ori_caldata_etc);
EXPORT_SYMBOL_GPL(hwifi_set_country_code_etc);
EXPORT_SYMBOL_GPL(hwifi_get_chip_id_cfg);
#ifdef _PRE_WLAN_MAC_ADDR_EDCA_FIX
EXPORT_SYMBOL_GPL(hwifi_get_up_down_edca);
EXPORT_SYMBOL_GPL(hwifi_get_edca_fix_cfg);
#endif
#if defined (_PRE_WLAN_FEATURE_RX_AGGR_EXTEND) || defined (_PRE_FEATURE_WAVEAPP_CLASSIFY)
EXPORT_SYMBOL_GPL(hwifi_get_wavap_mac_cfg);
EXPORT_SYMBOL_GPL(hwifi_wavap_classify);
#endif

#ifdef _PRE_WLAN_FEATURE_TPC_OVER_TEMP_PROTECT
EXPORT_SYMBOL_GPL(hwifi_get_custom_over_temp);
#endif

EXPORT_SYMBOL_GPL(hwifi_get_regdomain_from_country_code);
EXPORT_SYMBOL_GPL(hwifi_is_regdomain_changed_etc);
EXPORT_SYMBOL_GPL(hwifi_get_cfg_params);
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
EXPORT_SYMBOL_GPL(hwifi_custom_host_read_cfg_init);
#endif
EXPORT_SYMBOL_GPL(hwifi_get_init_priv_value);

#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */

/*lint -save -e578 -e19 */
DEFINE_GET_BUILD_VERSION_FUNC(plat);
/*lint -restore*/

int plat_main_init(void)
{
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    oal_int32                       l_val = 0;

    OAL_RET_ON_MISMATCH(plat, -OAL_EFAIL);

    ini_cfg_init_etc();
    hwifi_config_init_etc(CUS_TAG_DTS);
    hwifi_config_init_etc(CUS_TAG_NV);
    hwifi_config_init_etc(CUS_TAG_INI);

    /******************************************** 性能 ********************************************/
    l_val = hwifi_get_init_value_etc(CUS_TAG_INI, WLAN_CFG_INIT_AMPDU_TX_MAX_NUM);
    g_st_wlan_customize_etc.ul_ampdu_tx_max_num = (WLAN_AMPDU_TX_MAX_NUM >= l_val && 1 <= l_val) ? (oal_uint32)l_val : g_st_wlan_customize_etc.ul_ampdu_tx_max_num;

    /******************************************** 扫描 ********************************************/
    l_val = hwifi_get_init_value_etc(CUS_TAG_INI, WLAN_CFG_INIT_RANDOM_MAC_ADDR_SCAN);
    g_st_wlan_customize_etc.uc_random_mac_addr_scan = !!l_val;

    /******************************************** CAPABILITY ********************************************/
    l_val = hwifi_get_init_value_etc(CUS_TAG_INI, WLAN_CFG_INIT_DISABLE_CAPAB_2GHT40);
    g_st_wlan_customize_etc.uc_disable_capab_2ght40 = (oal_uint8)!!l_val;
#endif

    return OAL_SUCC;
}

void plat_main_exit(void)
{
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    ini_cfg_exit_etc();
#endif
}
/* 导出符号 */

oal_module_init(plat_main_init);
oal_module_exit(plat_main_exit);
oal_module_license("GPL");

#endif //#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)


#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

