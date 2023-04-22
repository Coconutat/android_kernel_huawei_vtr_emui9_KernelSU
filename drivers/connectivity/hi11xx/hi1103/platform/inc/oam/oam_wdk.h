
#ifndef __OAM_WDK_H__
#define __OAM_WDK_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "oal_ext_if.h"

/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define OAM_SOFTWARE_VERSION            "Hi1151 V100R001C01B200_0515"

/*****************************************************************************
  3 枚举定义
*****************************************************************************/
typedef enum
{
    /* ALG */
    OAM_FILE_ID_ALG_EXT_IF_H            = 1100,
    OAM_FILE_ID_ALG_TRANSPLANT_H        = 1101,
    OAM_FILE_ID_ALG_MAIN_C              = 1102,
    OAM_FILE_ID_ALG_MAIN_H              = 1103,
    OAM_FILE_ID_ALG_SCHEDULE_C          = 1104,
    OAM_FILE_ID_ALG_SCHEDULE_H          = 1105,
    OAM_FILE_ID_ALG_SCHEDULE_LOG_C      = 1106,
    OAM_FILE_ID_ALG_AUTORATE_C          = 1107,
    OAM_FILE_ID_ALG_AUTORATE_H          = 1108,
    OAM_FILE_ID_ALG_AUTORATE_LOG_C      = 1109,
    OAM_FILE_ID_ALG_AUTORATE_LOG_H      = 1110,
    OAM_FILE_ID_ALG_SMARTANT_C          = 1111,
    OAM_FILE_ID_ALG_SMARTANT_H          = 1112,
    OAM_FILE_ID_ALG_DBAC_C              = 1113,
    OAM_FILE_ID_ALG_DBAC_H              = 1114,
    OAM_FILE_ID_ALG_TXBF_H              = 1115,
    OAM_FILE_ID_ALG_TXBF_C              = 1116,
    OAM_FILE_ID_ALG_SIMPLE_SCHEDULE_C   = 1117,
    OAM_FILE_ID_ALG_SIMPLE_SCHEDULE_H   = 1118,
    OAM_FILE_ID_ALG_SCHEDULE_TRAFFIC_CTL_C  = 1119,
    OAM_FILE_ID_ALG_SCHEDULE_TRAFFIC_CTL_H  = 1120,
    OAM_FILE_ID_ALG_SCHEDULE_LOG_H          = 1121,
    OAM_FILE_ID_ALG_ANTI_INTERFERENCE_C     = 1122,
    OAM_FILE_ID_ALG_ANTI_INTERFERENCE_H     = 1123,
    OAM_FILE_ID_ALG_TPC_C                   = 1124,
    OAM_FILE_ID_ALG_TPC_H                   = 1125,
    OAM_FILE_ID_ALG_TPC_LOG_C               = 1126,
    OAM_FILE_ID_ALG_TPC_LOG_H               = 1127,
    OAM_FILE_ID_ALG_TXBF_TEST_C             = 1128,
    OAM_FILE_ID_ALG_EDCA_OPT_C              = 1129,
    OAM_FILE_ID_ALG_EDCA_OPT_H              = 1130,
    OAM_FILE_ID_ALG_MWO_DETECTION_C         = 1131,
    OAM_FILE_ID_ALG_MWO_DETECTION_H         = 1132,
    OAM_FILE_ID_ALG_CCA_OPTIMIZE_C          = 1133,
    OAM_FILE_ID_ALG_CCA_OPTIMIZE_H          = 1134,
    OAM_FILE_ID_ALG_CCA_OPTIMIZE_LOG_C      = 1135,
    OAM_FILE_ID_ALG_CCA_OPTIMIZE_LOG_H      = 1136,
    OAM_FILE_ID_ALG_INTF_DET_C              = 1137,
    OAM_FILE_ID_ALG_INTF_DET_H              = 1138,

    /* WAL */
    OAM_FILE_ID_WAL_EXT_IF_H            = 1200,
    OAM_FILE_ID_WAL_MAIN_C              = 1201,
    OAM_FILE_ID_WAL_MAIN_H              = 1202,
    OAM_FILE_ID_WAL_LINUX_BRIDGE_C      = 1203,
    OAM_FILE_ID_WAL_LINUX_BRIDGE_H      = 1204,
    OAM_FILE_ID_WAL_LINUX_CFG80211_C    = 1205,
    OAM_FILE_ID_WAL_LINUX_CFG80211_H    = 1206,
    OAM_FILE_ID_WAL_LINUX_IOCTL_C       = 1207,
    OAM_FILE_ID_WAL_LINUX_IOCTL_H       = 1208,
    OAM_FILE_ID_WAL_DATA_C              = 1209,
    OAM_FILE_ID_WAL_DATA_H              = 1210,
    OAM_FILE_ID_WAL_CONFIG_C            = 1211,
    OAM_FILE_ID_WAL_CONFIG_H            = 1212,
    OAM_FILE_ID_WAL_LINUX_SCAN_C        = 1213,
    OAM_FILE_ID_WAL_LINUX_SCAN_H        = 1214,
    OAM_FILE_ID_WAL_LINUX_EVENT_C       = 1215,
    OAM_FILE_ID_WAL_LINUX_EVENT_H       = 1216,
    OAM_FILE_ID_WAL_LINUX_RX_RSP_C      = 1217,
    OAM_FILE_ID_WAL_LINUX_RX_RSP_H      = 1218,
    OAM_FILE_ID_WAL_LINUX_NETLINK_C     = 1219,
    OAM_FILE_ID_WAL_LINUX_NETLINK_H     = 1220,
    OAM_FILE_ID_WAL_CONFIG_ACS_C        = 1221,
    OAM_FILE_ID_WAL_CONFIG_ACS_H        = 1222,
    OAM_FILE_ID_WAL_LINUX_FLOWCTL_C     = 1223,
    OAM_FILE_ID_WAL_LINUX_FLOWCTL_H     = 1224,
    OAM_FILE_ID_WAL_REGDB_H             = 1225,
    OAM_FILE_ID_HMAC_PROXYSTA_C         = 1226,
    OAM_FILE_ID_HMAC_PROXYSTA_H         = 1227,
    OAM_FILE_ID_WAL_REGDB_C             = 1228,
    OAM_FILE_ID_WAL_LINUX_ATCMDSRV_C    = 1229,
    OAM_FILE_ID_WAL_LINUX_ATCMDSRV_H    = 1230,
    OAM_FILE_ID_WAL_DFX_H               = 1231,
    OAM_FILE_ID_WAL_DFX_C               = 1232,
    OAM_FILE_ID_WAL_LINUX_IOCTL_DEBUG_C = 1233,
    OAM_FILE_ID_WAL_CONFIG_DEBUG_C      = 1234,
    OAM_FILE_ID_WAL_LINUX_CFGVENDOR_C   = 1235,
    OAM_FILE_ID_WAL_LINUX_CFGVENDOR_H   = 1236,

    /* HMAC */
    OAM_FILE_ID_HMAC_ISOLATION_C        = 1299,
    OAM_FILE_ID_HMAC_EXT_IF_H           = 1300,
    OAM_FILE_ID_HMAC_MAIN_C             = 1301,
    OAM_FILE_ID_HMAC_MAIN_H             = 1302,
    OAM_FILE_ID_HMAC_VAP_C              = 1303,
    OAM_FILE_ID_HMAC_VAP_H              = 1304,
    OAM_FILE_ID_HMAC_USER_C             = 1305,
    OAM_FILE_ID_HMAC_USER_H             = 1306,
    OAM_FILE_ID_HMAC_TX_DATA_C          = 1307,
    OAM_FILE_ID_HMAC_TX_DATA_H          = 1308,
    OAM_FILE_ID_HMAC_TX_BSS_COMMON_C    = 1309,
    OAM_FILE_ID_HMAC_TX_BSS_COMMON_H    = 1310,
    OAM_FILE_ID_HMAC_TX_AMSDU_C         = 1311,
    OAM_FILE_ID_HMAC_TX_AMSDU_H         = 1312,
    OAM_FILE_ID_HMAC_RX_DATA_C          = 1313,
    OAM_FILE_ID_HMAC_RX_DATA_H          = 1314,
    OAM_FILE_ID_HMAC_MGMT_CLASSIFIER_C  = 1315,
    OAM_FILE_ID_HMAC_MGMT_CLASSIFIER_H  = 1316,
    OAM_FILE_ID_HMAC_MGMT_BSS_COMM_C    = 1317,
    OAM_FILE_ID_HMAC_MGMT_BSS_COMM_H    = 1318,
    OAM_FILE_ID_HMAC_MGMT_STA_C         = 1319,
    OAM_FILE_ID_HMAC_MGMT_STA_H         = 1320,
    OAM_FILE_ID_HMAC_MGMT_AP_C          = 1321,
    OAM_FILE_ID_HMAC_MGMT_AP_H          = 1322,
    OAM_FILE_ID_HMAC_FSM_C              = 1323,
    OAM_FILE_ID_HMAC_FSM_H              = 1324,
    OAM_FILE_ID_HMAC_11i_C              = 1325,
    OAM_FILE_ID_HMAC_11i_H              = 1326,
    OAM_FILE_ID_HMAC_UAPSD_C            = 1327,
    OAM_FILE_ID_HMAC_UAPSD_H            = 1328,
    OAM_FILE_ID_HMAC_CONFIG_C           = 1329,
    OAM_FILE_ID_HMAC_CONFIG_H           = 1330,
    OAM_FILE_ID_HMAC_FRAG_C             = 1331,
    OAM_FILE_ID_HMAC_FRAG_H             = 1332,
    OAM_FILE_ID_HMAC_PROTECTION_C       = 1333,
    OAM_FILE_ID_HMAC_PROTECTION_H       = 1334,
    OAM_FILE_ID_HMAC_CHAN_MGMT_C        = 1335,
    OAM_FILE_ID_HMAC_CHAN_MGMT_H        = 1336,
    OAM_FILE_ID_HMAC_SMPS_H             = 1337,
    OAM_FILE_ID_HMAC_SMPS_C             = 1338,
    OAM_FILE_ID_HMAC_DATA_ACQ_H         = 1339,
    OAM_FILE_ID_HMAC_DATA_ACQ_C         = 1340,
    OAM_FILE_ID_HMAC_RX_FILTER_H        = 1341,
    OAM_FILE_ID_HMAC_RX_FILTER_C        = 1342,
    OAM_FILE_ID_HMAC_ENCAP_FRAME_STA_C  = 1343,
    OAM_FILE_ID_HMAC_ENCAP_FRAME_AP_C   = 1344,
    OAM_FILE_ID_HMAC_BLACKLIST_C        = 1345,
	OAM_FILE_ID_HMAC_M2U_C              = 1346,
    OAM_FILE_ID_HMAC_M2U_H              = 1347,
    OAM_FILE_ID_HMAC_PROXYARP_C         = 1348,
    OAM_FILE_ID_HMAC_HCC_ADAPT_H        = 1349,
    OAM_FILE_ID_HMAC_HCC_ADAPT_C        = 1350,
    OAM_FILE_ID_HMAC_SME_STA_C          = 1351,
    OAM_FILE_ID_HMAC_SME_STA_H          = 1352,
    OAM_FILE_ID_HMAC_DFS_C              = 1353,
    OAM_FILE_ID_HMAC_DFS_H              = 1354,
    OAM_FILE_ID_HMAC_RESET_H            = 1355,
    OAM_FILE_ID_HMAC_RESET_C            = 1356,
    OAM_FILE_ID_HMAC_SCAN_H             = 1357,
    OAM_FILE_ID_HMAC_SCAN_C             = 1358,
    OAM_FILE_ID_HMAC_P2P_C              = 1359,
    OAM_FILE_ID_HMAC_P2P_H              = 1360,
    OAM_FILE_ID_HMAC_EDCA_OPT_H         = 1361,
    OAM_FILE_ID_HMAC_EDCA_OPT_C         = 1362,
    OAM_FILE_ID_HMAC_BLOCKACK_H         = 1363,
    OAM_FILE_ID_HMAC_BLOCKACK_C         = 1364,
    OAM_FILE_ID_HMAC_PSM_AP_H           = 1365,
    OAM_FILE_ID_HMAC_PSM_AP_C           = 1366,
	OAM_FILE_ID_HMAC_RESOURCE_C         = 1367,
    OAM_FILE_ID_HMAC_RESOURCE_H         = 1368,
    OAM_FILE_ID_HMAC_DEVICE_C           = 1369,
    OAM_FILE_ID_HMAC_DEVICE_H           = 1370,
    OAM_FILE_ID_HMAC_WAPI_SMS4_C        = 1371,
    OAM_FILE_ID_HMAC_WAPI_SMS4_H        = 1372,
    OAM_FILE_ID_HMAC_WAPI_WPI_C         = 1373,
    OAM_FILE_ID_HMAC_WAPI_WPI_H         = 1374,
    OAM_FILE_ID_HMAC_ARP_OFFLOAD_C      = 1375,
    OAM_FILE_ID_HMAC_ARP_OFFLOAD_H      = 1376,
    OAM_FILE_ID_HMAC_ACS_C              = 1377,
    OAM_FILE_ID_HMAC_ACS_H              = 1378,
    OAM_FILE_ID_HMAC_TCP_OPT_C          = 1379,
    OAM_FILE_ID_HMAC_TCP_OPT_H          = 1380,
    OAM_FILE_ID_HMAC_TCP_OPT_STRUC_H    = 1381,
/* #ifdef _PRE_WLAN_FEATURE_ROAM */
    OAM_FILE_ID_HMAC_ROAM_ALG_C         = 1382,
    OAM_FILE_ID_HMAC_ROAM_ALG_H         = 1383,
    OAM_FILE_ID_HMAC_ROAM_MAIN_C        = 1384,
    OAM_FILE_ID_HMAC_ROAM_MAIN_H        = 1385,
    OAM_FILE_ID_HMAC_ROAM_CONNECT_C     = 1386,
    OAM_FILE_ID_HMAC_ROAM_CONNECT_H     = 1387,
/* #endif  //_PRE_WLAN_FEATURE_ROAM */
    OAM_FILE_ID_HMAC_ENCAP_FRAME_C      = 1388,

    OAM_FILE_ID_HMAC_WAPI_C             = 1389,
    OAM_FILE_ID_HMAC_WAPI_H             = 1390,
    OAM_FILE_ID_HMAC_CALI_MGMT_C        = 1391,
    OAM_FILE_ID_HMAC_CALI_MGMT_H        = 1392,

    OAM_FILE_ID_HISI_CUSTOMIZE_WIFI_HI110X_C   = 1393,

    OAM_FILE_ID_HMAC_DFX_C              = 1394,
    OAM_FILE_ID_HMAC_DFX_H              = 1395,

    OAM_FILE_ID_HMAC_TRAFFIC_CLASSIFY_C = 1396,
    OAM_FILE_ID_HMAC_TRAFFIC_CLASSIFY_H = 1397,
#ifdef _PRE_WLAN_FEATURE_HILINK
    /* HILINK */
    OAM_FILE_ID_HMAC_FBT_C              = 1398,
    OAM_FILE_ID_HMAC_FBT_H              = 1399,
    /* HILINK */
#endif
    OAM_FILE_ID_HMAC_OPMODE_C           = 1400,
    OAM_FILE_ID_HMAC_OPMODE_H           = 1401,
    OAM_FILE_ID_HMAC_M2S_C              = 1402,
    OAM_FILE_ID_HMAC_M2S_H              = 1403,
    OAM_FILE_ID_HMAC_BTCOEX_C           = 1404,
    OAM_FILE_ID_HMAC_BTCOEX_H           = 1405,
    OAM_FILE_ID_HMAC_WDS_C              = 1406,
    OAM_FILE_ID_HMAC_WDS_H              = 1407,
#ifdef _PRE_WLAN_FEATURE_WMMAC
    OAM_FILE_ID_HMAC_WMMAC_C            = 1408,
    OAM_FILE_ID_HMAC_WMMAC_H            = 1409,
#endif

    OAM_FILE_ID_HMAC_11R_C              = 1410,
    OAM_FILE_ID_HMAC_11R_H              = 1411,

#ifdef _PRE_WLAN_FEATURE_11K_EXTERN
    OAM_FILE_ID_HMAC_11k_C              = 1412,
    OAM_FILE_ID_HMAC_11k_H              = 1413,
#endif

    OAM_FILE_ID_HMAC_11V_C              = 1414,
    OAM_FILE_ID_HMAC_11V_H              = 1415,

#ifdef _PRE_WLAN_FEATURE_CAR
    OAM_FILE_ID_HMAC_CAR_C              = 1416,
    OAM_FILE_ID_HMAC_CAR_H              = 1417,
#endif
    OAM_FILE_ID_HMAC_SINGLE_PROXYSTA_C  = 1418,
    OAM_FILE_ID_HMAC_SINGLE_PROXYSTA_H  = 1419,

    /* DMAC */
    OAM_FILE_ID_DMAC_CSA_STA_C          = 1440,
    OAM_FILE_ID_DMAC_CSA_STA_H          = 1441,
#ifdef _PRE_WLAN_FEATURE_11V
    OAM_FILE_ID_DMAC_11V_H              = 1442,
    OAM_FILE_ID_DMAC_11V_C              = 1443,
#endif
    OAM_FILE_ID_DMAC_POWER_C            = 1444,
    OAM_FILE_ID_DMAC_POWER_H            = 1445,
    OAM_FILE_ID_DMAC_BSD_C              = 1446,
    OAM_FILE_ID_DMAC_BSD_H              = 1447,
#if defined (_PRE_WLAN_FEATURE_DFS_OPTIMIZE) || defined (_PRE_WLAN_FEATURE_DFS_ENABLE)
    OAM_FILE_ID_DMAC_RADAR_C            = 1448,
    OAM_FILE_ID_DMAC_RADAR_H            = 1449,
#endif
    OAM_FILE_ID_DMAC_EXT_IF_H           = 1450,
    OAM_FILE_ID_DMAC_ALG_IF_H           = 1451,
    OAM_FILE_ID_DMAC_MAIN_C             = 1452,
    OAM_FILE_ID_DMAC_MAIN_H             = 1453,
    OAM_FILE_ID_DMAC_VAP_C              = 1454,
    OAM_FILE_ID_DMAC_VAP_H              = 1455,
    OAM_FILE_ID_MAC_RESOURCE_C          = 1456,
    OAM_FILE_ID_MAC_RESOURCE_H          = 1457,
    OAM_FILE_ID_MAC_DEVICE_C            = 1458,
    OAM_FILE_ID_MAC_DEVICE_H            = 1459,
    OAM_FILE_ID_MAC_VAP_C               = 1460,
    OAM_FILE_ID_MAC_VAP_H               = 1461,
    OAM_FILE_ID_MAC_USER_C              = 1462,
    OAM_FILE_ID_MAC_USER_H              = 1463,
    OAM_FILE_ID_MAC_IE_C                = 1464,
    OAM_FILE_ID_MAC_IE_H                = 1465,
    OAM_FILE_ID_MAC_FRAME_C             = 1466,
    OAM_FILE_ID_MAC_REGDOMAIN_C         = 1467,
    OAM_FILE_ID_DMAC_USER_C             = 1468,
    OAM_FILE_ID_DMAC_USER_H             = 1469,
    OAM_FILE_ID_DMAC_RX_DATA_C          = 1470,
    OAM_FILE_ID_DMAC_RX_DATA_H          = 1471,
    OAM_FILE_ID_DMAC_TID_C              = 1472,
    OAM_FILE_ID_DMAC_TID_H              = 1473,
    OAM_FILE_ID_DMAC_TX_DSCR_QUEUE_C    = 1474,
    OAM_FILE_ID_DMAC_TX_DSCR_QUEUE_H    = 1475,
    OAM_FILE_ID_DMAC_TX_BSS_COMM_C      = 1476,
    OAM_FILE_ID_DMAC_TX_BSS_COMM_H      = 1477,
    OAM_FILE_ID_DMAC_PSM_AP_C           = 1478,
    OAM_FILE_ID_DMAC_PSM_AP_H           = 1479,
    OAM_FILE_ID_DMAC_TX_COMPLETE_H      = 1480,
    OAM_FILE_ID_DMAC_TX_COMPLETE_C      = 1481,
    OAM_FILE_ID_DMAC_MGMT_CLASSIFIER_C  = 1482,
    OAM_FILE_ID_DMAC_MGMT_CLASSIFIER_H  = 1483,
    OAM_FILE_ID_DMAC_MGMT_BSS_COMM_C    = 1484,
    OAM_FILE_ID_DMAC_MGMT_BSS_COMM_H    = 1485,
    OAM_FILE_ID_DMAC_MGMT_AP_C          = 1486,
    OAM_FILE_ID_DMAC_MGMT_AP_H          = 1487,
    OAM_FILE_ID_DMAC_MGMT_STA_C         = 1488,
    OAM_FILE_ID_DMAC_MGMT_STA_H         = 1489,
    OAM_FILE_ID_DMAC_UAPSD_C            = 1490,
    OAM_FILE_ID_DMAC_UAPSD_H            = 1491,
    OAM_FILE_ID_DMAC_ALG_C              = 1492,
    OAM_FILE_ID_DMAC_ALG_H              = 1493,
    OAM_FILE_ID_DMAC_BLOCKACK_C         = 1494,
    OAM_FILE_ID_DMAC_BLOCKACK_H         = 1495,
    OAM_FILE_ID_DMAC_BEACON_C           = 1496,
    OAM_FILE_ID_DMAC_BEACON_H           = 1497,
    OAM_FILE_ID_DMAC_WEP_C              = 1498,
    OAM_FILE_ID_DMAC_WEP_H              = 1499,
    OAM_FILE_ID_DMAC_11I_C              = 1500,
    OAM_FILE_ID_DMAC_11I_H              = 1501,
    OAM_FILE_ID_DMAC_SCAN_C             = 1502,
    OAM_FILE_ID_DMAC_SCAN_H             = 1503,
    OAM_FILE_ID_DMAC_ACS_C              = 1504,
    OAM_FILE_ID_DMAC_ACS_H              = 1505,
    OAM_FILE_ID_DMAC_DFS_C              = 1506,
    OAM_FILE_ID_DMAC_DFS_H              = 1507,
    OAM_FILE_ID_DMAC_RESET_C            = 1508,
    OAM_FILE_ID_DMAC_RESET_H            = 1509,
    OAM_FILE_ID_MAC_FCS_C               = 1510,
    OAM_FILE_ID_MAC_FCS_H               = 1511,
    OAM_FILE_ID_DMAC_CONFIG_C           = 1512,
    OAM_FILE_ID_DMAC_CONFIG_H           = 1513,
    OAM_FILE_ID_DMAC_STAT_C             = 1514,
    OAM_FILE_ID_DMAC_STAT_H             = 1515,
    OAM_FILE_ID_ALG_SCH_TEST_H          = 1516,
    OAM_FILE_ID_ALG_SCH_TEST_C          = 1517,
    OAM_FILE_ID_DMAC_CHAN_MGMT_C        = 1518,
    OAM_FILE_ID_DMAC_CHAN_MGMT_H        = 1519,
    OAM_FILE_ID_DMAC_DATA_ACQ_H         = 1520,
    OAM_FILE_ID_DMAC_DATA_ACQ_C         = 1521,
    OAM_FILE_ID_DMAC_USER_TRACK_H       = 1522,
    OAM_FILE_ID_DMAC_USER_TRACK_C       = 1523,
    OAM_FILE_ID_DMAC_RX_FILTER_H        = 1524,
    OAM_FILE_ID_DMAC_RX_FILTER_C        = 1525,
    OAM_FILE_ID_MAC_REGDOMAIN_H         = 1526,
    OAM_FILE_ID_MAC_FRAME_H             = 1527,
    OAM_FILE_ID_DMAC_11W_C              = 1528,
    OAM_FILE_ID_DMAC_TXOPPS_C           = 1529,
    OAM_FILE_ID_DMAC_TXOPPS_H           = 1530,
    OAM_FILE_ID_DMAC_DFT_H              = 1531,
    OAM_FILE_ID_DMAC_DFT_C              = 1532,
    OAM_FILE_ID_DMAC_AP_PM_C            = 1533,
    OAM_FILE_ID_DMAC_AP_PM_H            = 1534,
    OAM_FILE_ID_MAC_PM_C                = 1535,
    OAM_FILE_ID_MAC_PM_H                = 1536,
    OAM_FILE_ID_DMAC_SIMPLE_SCHEDULE_C  = 1537,
    OAM_FILE_ID_DMAC_SIMPLE_SCHEDULE_H  = 1538,
    OAM_FILE_ID_DMAC_HCC_ADAPT_C        = 1539,
    OAM_FILE_ID_DMAC_HCC_ADAPT_H        = 1540,
    OAM_FILE_ID_MAC_BOARD_C             = 1541,
    OAM_FILE_ID_MAC_BOARD_H             = 1542,
    OAM_FILE_ID_DMAC_P2P_C              = 1543,
    OAM_FILE_ID_DMAC_P2P_H              = 1544,
    OAM_FILE_ID_DMAC_BTCOEX_C           = 1545,
    OAM_FILE_ID_DMAC_BTCOEX_H           = 1546,


    OAM_FILE_ID_DMAC_STA_PM_C           = 1547,
    OAM_FILE_ID_DMAC_STA_PM_H           = 1548,
    OAM_FILE_ID_DMAC_UAPSD_STA_C        = 1549,
    OAM_FILE_ID_DMAC_UAPSD_STA_H        = 1550,
    OAM_FILE_ID_DMAC_PSM_STA_C          = 1551,
    OAM_FILE_ID_DMAC_PSM_STA_H          = 1552,
    OAM_FILE_ID_DMAC_PM_STA_C           = 1553,
    OAM_FILE_ID_DMAC_PM_STA_H           = 1554,

    OAM_FILE_ID_MAC_DATA_C              = 1555,
    OAM_FILE_ID_DMAC_LTECOEX_C          = 1556,
    OAM_FILE_ID_DMAC_LTECOEX_H          = 1557,

    OAM_FILE_ID_MAC_11I_C               = 1558,
    OAM_FILE_ID_MAC_11I_H               = 1559,
    OAM_FILE_ID_DMAC_PROFILING_H        = 1560,
    OAM_FILE_ID_DMAC_DFX_C              = 1561,
    OAM_FILE_ID_DMAC_DFX_H              = 1562,
    OAM_FILE_ID_DMAC_DEVICE_C           = 1563,
    OAM_FILE_ID_DMAC_DEVICE_H           = 1564,
    OAM_FILE_ID_DMAC_ARP_OFFLOAD_C      = 1565,
    OAM_FILE_ID_DMAC_ARP_OFFLOAD_H      = 1566,

    OAM_FILE_ID_DMAC_RESOURCE_C         = 1567,
    OAM_FILE_ID_DMAC_RESOURCE_H         = 1568,
    OAM_FILE_ID_DMAC_AUTO_ADJUST_FREQ_C = 1569,
    OAM_FILE_ID_DMAC_AUTO_ADJUST_FREQ_H = 1570,
    OAM_FILE_ID_DMAC_CONFIG_DEBUG_C     = 1571,

    OAM_FILE_ID_DMAC_GREEN_AP_C         = 1572,
    OAM_FILE_ID_DMAC_GREEN_AP_H         = 1573,

    OAM_FILE_ID_DMAC_11K_H              = 1574,
    OAM_FILE_ID_DMAC_11K_C              = 1575,
    OAM_FILE_ID_DMAC_FTM_H              = 1576,
    OAM_FILE_ID_DMAC_FTM_C              = 1577,
    OAM_FILE_ID_DMAC_SMPS_C             = 1578,
    OAM_FILE_ID_DMAC_SMPS_H             = 1579,
    OAM_FILE_ID_DMAC_OPMODE_C           = 1580,
    OAM_FILE_ID_DMAC_OPMODE_H           = 1581,
    OAM_FILE_ID_DMAC_M2S_C              = 1582,
    OAM_FILE_ID_DMAC_M2S_H              = 1583,

    OAM_FILE_ID_DMAC_AUTO_CALI_C        = 1584,
    OAM_FILE_ID_DMAC_AUTO_CALI_H        = 1585,
    OAM_FILE_ID_DMAC_USER_EXTEND_C      = 1586,
    OAM_FILE_ID_DMAC_USER_EXTEND_H      = 1587,

    OAM_FILE_ID_DMAC_PKT_CAPTURE_C      = 1588,
    OAM_FILE_ID_DMAC_PKT_CAPTURE_H      = 1589,


#ifdef _PRE_WLAN_FEATURE_WMMAC
    OAM_FILE_ID_DMAC_WMMAC_C            = 1590,
    OAM_FILE_ID_DMAC_WMMAC_H            = 1591,
#endif
    OAM_FILE_ID_DMAC_CRYPTO_COMM_C      = 1592,
    OAM_FILE_ID_DMAC_CRYPTO_COMM_H      = 1593,
    OAM_FILE_ID_DMAC_CRYPTO_WEP_C       = 1594,
    OAM_FILE_ID_DMAC_CRYPTO_WEP_H       = 1595,
    OAM_FILE_ID_DMAC_CRYPTO_TKIP_C      = 1596,
    OAM_FILE_ID_DMAC_CRYPTO_TKIP_H      = 1597,
    OAM_FILE_ID_DMAC_CRYPTO_AES_CCM_C   = 1598,
    OAM_FILE_ID_DMAC_CRYPTO_AES_CCM_H   = 1599,

    /* HAL */
    OAM_FILE_ID_UT_HAL_TX_PROC_PROFILING_C  = 1600,
    OAM_FILE_ID_HAL_EXT_IF_H                = 1601,
    OAM_FILE_ID_HAL_PA_C                    = 1602,
    OAM_FILE_ID_HAL_PA_H                    = 1603,
    OAM_FILE_ID_HAL_BASE_C                  = 1604,
    OAM_FILE_ID_HAL_BASE_H                  = 1605,
    OAM_FILE_ID_HAL_MAIN_C                  = 1606,
    OAM_FILE_ID_HAL_MAIN_H                  = 1607,
    OAM_FILE_ID_HAL_IRQ_C                   = 1608,
    OAM_FILE_ID_HAL_IRQ_H                   = 1609,
    OAM_FILE_ID_HAL_TO_DMAC_IF_C            = 1610,
    OAM_FILE_ID_HAL_CHIP_C                  = 1611,
    OAM_FILE_ID_HAL_DEVICE_C                = 1612,
    OAM_FILE_ID_HAL_DEVICE_H                = 1613,
    OAM_FILE_ID_HAL_SIM_C                   = 1614,
    OAM_FILE_ID_HAL_WITP_MAC_C              = 1615,
    OAM_FILE_ID_HAL_WITP_RF_C               = 1616,
    OAM_FILE_ID_HAL_WITP_RF_H               = 1617,
    OAM_FILE_ID_HAL_WITP_CALI_C             = 1618,
    OAM_FILE_ID_HAL_WITP_CALI_H             = 1619,
    OAM_FILE_ID_HAL_WITP_MAC_H              = 1620,
    OAM_FILE_ID_HAL_RESET_C                 = 1621,
    OAM_FILE_ID_HAL_RESET_H                 = 1622,
    OAM_FILE_ID_HAL_WITP_PHY_REG_H          = 1623,
    OAM_FILE_ID_HAL_WITP_DMT_IF_C           = 1624,
    OAM_FILE_ID_HAL_WITP_DMT_IF_H           = 1625,
    OAM_FILE_ID_WITP_DEBUG_C                = 1626,
    OAM_FILE_ID_HAL_WITP_SOC_H              = 1627,
    OAM_FILE_ID_HAL_WITP_PHY_H              = 1628,
    OAM_FILE_ID_HAL_WITP_PHY_C              = 1629,
    OAM_FILE_ID_HAL_HW_MAC_H                = 1630,

    OAM_FILE_ID_HAL_HI1102_PHY_REG_H          = 1631,
    OAM_FILE_ID_HAL_HI1102_MAC_REG_H          = 1632,
    OAM_FILE_ID_HAL_HI1102_SOC_H              = 1633,
    OAM_FILE_ID_HAL_HI1102_PHY_H              = 1634,
    OAM_FILE_ID_HAL_HI1102_PHY_C              = 1635,
    OAM_FILE_ID_HAL_HI1102_MAC_H              = 1636,
    OAM_FILE_ID_HAL_HI1102_MAC_C              = 1637,
    OAM_FILE_ID_HAL_DBB_REG_RW_H              = 1638,
    OAM_FILE_ID_HAL_HI1102_DSCR_H             = 1639,
    OAM_FILE_ID_HAL_HI1102_DSCR_C             = 1640,
    OAM_FILE_ID_HAL_HI1102_RF_H               = 1641,
    OAM_FILE_ID_HAL_HI1102_RF_C               = 1642,
    OAM_FILE_ID_HAL_HI1102_RESET_H            = 1643,
    OAM_FILE_ID_HAL_HI1102_RESET_C            = 1644,
    OAM_FILE_ID_HAL_HI1102_IRQ_H              = 1645,
    OAM_FILE_ID_HAL_HI1102_IRQ_C              = 1646,
    OAM_FILE_ID_HAL_HI1102_SOC_REG_H          = 1647,
    OAM_FILE_ID_HAL_HI1102_COEX_REG_H         = 1648,
    OAM_FILE_ID_HAL_HI1102_COEX_REG_C         = 1649,
    OAM_FILE_ID_HI1102_RF_CORIDC_C            = 1650,
    OAM_FILE_ID_HI1102_RF_CORIDC_H            = 1651,
    OAM_FILE_ID_HAL_HI1102_PM_C               = 1652,
    OAM_FILE_ID_HAL_HI1102_PM_H               = 1653,
    OAM_FILE_ID_HAL_HI1102_CALI_DPD_C         = 1654,
    OAM_FILE_ID_HAL_HI1102_CALI_DPD_H         = 1655,

    /*1103*/
    OAM_FILE_ID_HAL_HI1103_PHY_REG_H          = 1659,
    OAM_FILE_ID_HAL_HI1103_MAC_REG_H          = 1660,
    OAM_FILE_ID_HAL_HI1103_SOC_H              = 1661,
    OAM_FILE_ID_HAL_HI1103_PHY_H              = 1662,
    OAM_FILE_ID_HAL_HI1103_PHY_C              = 1663,
    OAM_FILE_ID_HAL_HI1103_MAC_H              = 1664,
    OAM_FILE_ID_HAL_HI1103_MAC_C              = 1665,
    OAM_FILE_ID_HAL_HI1103_DSCR_H             = 1666,
    OAM_FILE_ID_HAL_HI1103_DSCR_C             = 1667,
    OAM_FILE_ID_HAL_HI1103_RF_H               = 1668,
    OAM_FILE_ID_HAL_HI1103_RF_C               = 1669,
    OAM_FILE_ID_HAL_HI1103_RESET_H            = 1670,
    OAM_FILE_ID_HAL_HI1103_RESET_C            = 1671,
    OAM_FILE_ID_HAL_HI1103_IRQ_H              = 1672,
    OAM_FILE_ID_HAL_HI1103_IRQ_C              = 1673,
    OAM_FILE_ID_HAL_HI1103_SOC_REG_H          = 1674,
    OAM_FILE_ID_HAL_HI1103_COEX_REG_H         = 1675,
    OAM_FILE_ID_HAL_HI1103_COEX_REG_C         = 1676,
    OAM_FILE_ID_HI1103_RF_CORIDC_C            = 1677,
    OAM_FILE_ID_HI1103_RF_CORIDC_H            = 1678,
    OAM_FILE_ID_HAL_HI1103_PM_C               = 1679,
    OAM_FILE_ID_HAL_HI1103_PM_H               = 1680,
    OAM_FILE_ID_HAL_HI1103_CALI_DPD_C         = 1681,
    OAM_FILE_ID_HAL_HI1103_CALI_DPD_H         = 1682,
    OAM_FILE_ID_HAL_WITP_PHY_REG_51V2_H       = 1683,
    OAM_FILE_ID_HAL_WITP_SOC_51V2_H           = 1684,
    /* 1151v200 */
#ifdef _PRE_WLAN_RF_CALI_1151V2
    OAM_FILE_ID_HAL_WITP_CALI_1151V2_C        = 1685,
    OAM_FILE_ID_HAL_WITP_CALI_1151V2_H        = 1686,
#endif  /* _PRE_WLAN_RF_CALI_1151V2 */
    OAM_FILE_ID_HAL_WITP_CORIDC_C             = 1687,
    OAM_FILE_ID_HAL_WITP_CORIDC_H             = 1688,

    /* OAM */
    OAM_FILE_ID_OAM_LINUX_NETLINK_C         = 1700,
    OAM_FILE_ID_OAM_LINUX_NETLINK_H         = 1701,
    OAM_FILE_ID_OAM_LOG_C                   = 1702,
    OAM_FILE_ID_OAM_LOG_H                   = 1703,
    OAM_FILE_ID_OAM_TRACE_C                 = 1704,
    OAM_FILE_ID_OAM_TRACE_H                 = 1705,
    OAM_FILE_ID_OAM_STATISTICS_C            = 1706,
    OAM_FILE_ID_OAM_STATISTICS_H            = 1707,
    OAM_FILE_ID_OAM_EXT_IF_H                = 1708,
    OAM_FILE_ID_OAM_MAIN_C                  = 1709,
    OAM_FILE_ID_OAM_MAIN_H                  = 1710,
    OAM_FILE_ID_REGISTER_C                  = 1711,
    OAM_FILE_ID_REGISTER_H                  = 1712,
    OAM_FILE_ID_OAM_EVENT_C                 = 1713,
    OAM_FILE_ID_OAM_EVENT_H                 = 1714,

    /* OAL */
    OAM_FILE_ID_OAL_EXT_IF_H                = 1800,
    OAM_FILE_ID_OAL_MAIN_C                  = 1801,
    OAM_FILE_ID_OAL_MAIN_H                  = 1802,
    OAM_FILE_ID_OAL_TYPES_H                 = 1803,
    OAM_FILE_ID_OAL_MEM_C                   = 1804,
    OAM_FILE_ID_OAL_MEM_H                   = 1805,
    OAM_FILE_ID_OAL_QUEUE_H                 = 1806,
    OAM_FILE_ID_OAL_GPIO_H                  = 1807,
    OAM_FILE_ID_OAL_NET_C                   = 1808,
    OAM_FILE_ID_OAL_PCI_C                   = 1809,
    OAM_FILE_ID_OAL_KERNEL_FILE_C           = 1810,
    OAM_FILE_ID_OAL_HARDWARE_C              = 1811,
    OAM_FILE_ID_OAL_PCI_IF_H                = 1812,
    OAM_FILE_ID_OAL_WORKQUEUE_H             = 1813,
#ifdef _PRE_WLAN_FEATURE_SMP_SUPPORT
    OAM_FILE_ID_OAL_THREAD_H                = 1814,
#endif
    OAM_FILE_ID_OAL_PROFILING_C             = 1815,
    OAM_FILE_ID_OAL_CFG80211_C              = 1816,

    OAM_FILE_ID_OAL_WORKQUEUE_C             = 1817,

    OAM_FILE_ID_OAL_FSM_C                   = 1818,
    OAM_FILE_ID_OAL_FSM_H                   = 1819,

    /* FRW */
    OAM_FILE_ID_FRW_EXT_IF_H                = 1900,
    OAM_FILE_ID_FRW_MAIN_C                  = 1901,
    OAM_FILE_ID_FRW_MAIN_H                  = 1902,
    OAM_FILE_ID_FRW_IPC_MSGQUEUE_H          = 1903,
    OAM_FILE_ID_FRW_IPC_MSGQUEUE_C          = 1904,
    OAM_FILE_ID_FRW_EVENT_QUEUE_H           = 1905,
    OAM_FILE_ID_FRW_EVENT_QUEUE_C           = 1906,
    OAM_FILE_ID_FRW_EVENT_SCHED_H           = 1907,
    OAM_FILE_ID_FRW_EVENT_SCHED_C           = 1908,
    OAM_FILE_ID_FRW_EVENT_MAIN_H            = 1909,
    OAM_FILE_ID_FRW_EVENT_MAIN_C            = 1910,
    OAM_FILE_ID_FRW_EVENT_DEPLOY_H          = 1911,
    OAM_FILE_ID_FRW_EVENT_DEPLOY_C          = 1912,
    OAM_FILE_ID_FRW_TASK_H                  = 1913,
    OAM_FILE_ID_FRW_TASK_C                  = 1914,
    OAM_FILE_ID_FRW_TIMER_C                 = 1915,
    OAM_FILE_ID_FRW_TIMER_H                 = 1916,

    /* BUILDER */
    OAM_FILE_ID_BLD_RAW_C                   = 1916,
    OAM_FILE_ID_BLD_RAW_H                   = 1917,
    OAM_FILE_ID_FRW_SDIO_TEST_C             = 1918,
    OAM_FILE_ID_FRW_SDIO_TEST_H             = 1919,

    OAM_FILE_ID_OAL_HCC_HOST_C              = 1920,
    OAM_FILE_ID_OAL_SDIO_HOST_C              = 1921,
    OAM_FILE_ID_OAL_PCIE_HOST_C              = 1922,
    OAM_FILE_ID_OAL_PCIE_LINUX_C             = 1923,
    OAM_FILE_ID_OAL_HCC_BUS_C                = 1924,
    OAM_FILE_ID_OAL_PCIE_FIRMWARE_C          = 1925,

    /*chip test*/
    OAM_FILE_ID_DMAC_TEST_MAIN_C            = 2000,
    OAM_FILE_ID_DMAC_TEST_MAIN_H            = 2001,
    OAM_FILE_ID_HMAC_TEST_MAIN_C            = 2002,
    OAM_FILE_ID_HMAC_TEST_MAIN_H            = 2003,
    OAM_FILE_ID_DMAC_LPM_TEST_C             = 2004,
    OAM_FILE_ID_DMAC_LPM_TEST_H             = 2005,
    OAM_FILE_ID_DMAC_FRAME_FILTER_TEST_C    = 2006,
    OAM_FILE_ID_DMAC_FRAME_FILTER_TEST_H    = 2007,
    OAM_FILE_ID_ALG_DBAC_TEST_C             = 2008,
    OAM_FILE_ID_ALG_DBAC_TEST_H             = 2009,
    OAM_FILE_ID_DMAC_TEST_SCH_C             = 2010,
    OAM_FILE_ID_DMAC_TEST_SCH_H             = 2011,
    OAM_FILE_ID_DMAC_WMM_TEST_C             = 2012,
    OAM_FILE_ID_DMAC_WMM_TEST_H             = 2013,
    OAM_FILE_ID_ALG_TEST_MAIN_C             = 2014,
    OAM_FILE_ID_ALG_TEST_MAIN_H             = 2015,
    OAM_FILE_ID_ALG_RSSI_TEST_C             = 2016,
    OAM_FILE_ID_ALG_RSSI_TEST_H             = 2017,
    OAM_FILE_ID_ALG_TPC_TEST_C              = 2018,
    OAM_FILE_ID_ALG_TPC_TEST_H              = 2019,

    OAM_FILE_ID_HAL_TEST_MAIN_C             = 2020,
    OAM_FILE_ID_HAL_TEST_MAIN_H             = 2021,
    OAM_FILE_ID_HAL_LPM_TEST_C              = 2022,
    OAM_FILE_ID_HAL_LPM_TEST_H              = 2023,

    OAM_FILE_ID_DMAC_DFS_TEST_C             = 2024,
    OAM_FILE_ID_DMAC_DFS_TEST_H             = 2025,
    OAM_FILE_ID_DMAC_ACS_TEST_C             = 2026,
    OAM_FILE_ID_DMAC_ACS_TEST_H             = 2027,
    OAM_FILE_ID_DMAC_SCAN_TEST_C            = 2028,
    OAM_FILE_ID_DMAC_SCAN_TEST_H            = 2029,

    OAM_FILE_ID_ALG_SMARTANT_TEST_C         = 2030,
    OAM_FILE_ID_ALG_SMARTANT_TEST_H         = 2031,
    OAM_FILE_ID_ALG_AUTORATE_TEST_C         = 2032,
    OAM_FILE_ID_ALG_AUTORATE_TEST_H         = 2033,

    OAM_FILE_ID_TEST_MAIN_C                 = 2034,
    OAM_FILE_ID_TEST_MAIN_H                 = 2035,

    /*sdt*/
    OAM_FILE_ID_SDT_DRV_C                   = 2100,
    OAM_FILE_ID_SDT_DRV_H                   = 2101,

    /* HUT */
    OAM_FILE_ID_HUT_MAIN_C                  = 2102,
    OAM_FILE_ID_HUT_MAIN_H                  = 2103,

    /* HSIMU */
	OAM_FILE_ID_HSIMU_REG_C                 = 2104,
    OAM_FILE_ID_HSIMU_REG_H                 = 2105,

    /* builder */
    OAM_FILE_ID_MAIN_C                      = 2106,
    OAM_FILE_ID_MAIN_H                      = 2107,

    /* plat main */
    OAM_FILE_ID_PLAT_MAIN_C                 = 2108,
    OAM_FILE_ID_PLAT_MAIN_H                 = 2109,

    OAM_FILE_ID_HAL_HI1102_CALI_C           = 2110,
    OAM_FILE_ID_HAL_HI1102_CALI_H           = 2111,

    OAM_FILE_ID_HI1102_RF_BT_CALI_C         = 2112,
    OAM_FILE_ID_HI1102_RF_BT_CALI_H         = 2113,

    OAM_FILE_ID_HAL_HI1103_CALI_C           = 2114,
    OAM_FILE_ID_HAL_HI1103_CALI_H           = 2115,

    OAM_FILE_ID_HI1103_RF_BT_CALI_C         = 2116,
    OAM_FILE_ID_HI1103_RF_BT_CALI_H         = 2117,
    OAM_FILE_ID_HAL_DEVICE_FSM_C            = 2118,
    OAM_FILE_ID_HAL_DEVICE_FSM_H            = 2119,

    OAM_FILE_ID_DMAC_DEVICE_C_ADD           = 2120,

    OAM_FILE_ID_HAL_WITP_POWER_C            = 2121,
    OAM_FILE_ID_HAL_WITP_POWER_H            = 2122,
    OAM_FILE_ID_HAL_HI1102_POWER_C          = 2123,
    OAM_FILE_ID_HAL_HI1102_POWER_H          = 2124,
    OAM_FILE_ID_HAL_HI1103_POWER_C          = 2125,
    OAM_FILE_ID_HAL_HI1103_POWER_H          = 2126,

    OAM_FILE_ID_HAL_HI1103_ATE_TEST_C       = 2127,
    OAM_FILE_ID_HAL_HI1103_ATE_TEST_H       = 2128,

    /* plat */
    OAM_FILE_ID_PLAT_PM_WLAN_C              = 2500,

    OAM_FILE_ID_MAC_AUTO_ADJUST_FREQ_C             = 2501,

    OAM_FILE_ID_HAL_DPD_C                   = 2502,
    OAM_FILE_ID_HAL_DPD_H                   = 2503,

    OAM_FILE_ID_HISI_CUSTOMIZE_WIFI_HI115X_C   = 2504,

    /* HAL rom化文件 */
    OAM_FILE_ID_HAL_HI1103_MAC_ROM_C        = 2600,
    OAM_FILE_ID_HAL_CHIP_ROM_C              = 2601,
    OAM_FILE_ID_HAL_DEVICE_ROM_C            = 2602,
    OAM_FILE_ID_HAL_MAIN_ROM_C              = 2603,
    OAM_FILE_ID_HAL_TO_DMAC_IF_ROM_C        = 2604,
    OAM_FILE_ID_HAL_HI1103_DSCR_ROM_C       = 2605,
    OAM_FILE_ID_HAL_DEVICE_FSM_ROM_C        = 2606,
    OAM_FILE_ID_HAL_HI1103_CALI_ROM_C       = 2607,
    OAM_FILE_ID_HI1103_RF_CORIDC_ROM_C      = 2608,
    OAM_FILE_ID_HAL_HI1103_RF_ROM_C         = 2609,
    OAM_FILE_ID_HAL_HI1103_COEX_REG_ROM_C   = 2610,


    /* DMAC rom化文件 */
#if defined (_PRE_WLAN_FEATURE_DFS_OPTIMIZE) || defined (_PRE_WLAN_FEATURE_DFS_ENABLE)
    OAM_FILE_ID_DMAC_RADAR_ROM_C            = 2702,
#endif
    OAM_FILE_ID_DMAC_MAIN_ROM_C             = 2703,
    OAM_FILE_ID_DMAC_VAP_ROM_C              = 2704,
    OAM_FILE_ID_MAC_RESOURCE_ROM_C          = 2705,
    OAM_FILE_ID_MAC_DEVICE_ROM_C            = 2706,
    OAM_FILE_ID_MAC_VAP_ROM_C               = 2707,
    OAM_FILE_ID_MAC_USER_ROM_C              = 2708,
    OAM_FILE_ID_MAC_IE_ROM_C                = 2709,
    OAM_FILE_ID_MAC_FRAME_ROM_C             = 2710,

    OAM_FILE_ID_MAC_REGDOMAIN_ROM_C         = 2711,
    OAM_FILE_ID_DMAC_USER_ROM_C             = 2712,
    OAM_FILE_ID_DMAC_RX_DATA_ROM_C          = 2713,
    OAM_FILE_ID_DMAC_TID_ROM_C              = 2714,
    OAM_FILE_ID_DMAC_TX_DSCR_QUEUE_ROM_C    = 2715,
    OAM_FILE_ID_DMAC_TX_BSS_COMM_ROM_C      = 2716,
    OAM_FILE_ID_DMAC_PSM_AP_ROM_C           = 2717,
    OAM_FILE_ID_DMAC_TX_COMPLETE_ROM_C      = 2718,
    OAM_FILE_ID_DMAC_MGMT_CLASSIFIER_ROM_C  = 2719,
    OAM_FILE_ID_DMAC_MGMT_BSS_COMM_ROM_C    = 2720,

    OAM_FILE_ID_DMAC_MGMT_AP_ROM_C          = 2721,
    OAM_FILE_ID_DMAC_MGMT_STA_ROM_C         = 2722,
    OAM_FILE_ID_DMAC_UAPSD_ROM_C            = 2723,
    OAM_FILE_ID_DMAC_ALG_ROM_C              = 2724,
    OAM_FILE_ID_DMAC_BLOCKACK_ROM_C         = 2725,
    OAM_FILE_ID_DMAC_BEACON_ROM_C           = 2726,
    OAM_FILE_ID_DMAC_WEP_ROM_C              = 2727,
    OAM_FILE_ID_DMAC_11I_ROM_C              = 2728,
    OAM_FILE_ID_DMAC_SCAN_ROM_C             = 2729,
    OAM_FILE_ID_DMAC_ACS_ROM_C              = 2730,

    OAM_FILE_ID_DMAC_DFS_ROM_C              = 2731,
    OAM_FILE_ID_DMAC_RESET_ROM_C            = 2732,
    OAM_FILE_ID_MAC_FCS_ROM_C               = 2733,
    OAM_FILE_ID_DMAC_CONFIG_ROM_C           = 2734,
    OAM_FILE_ID_DMAC_STAT_ROM_C             = 2735,
    OAM_FILE_ID_ALG_SCH_TEST_ROM_C          = 2736,
    OAM_FILE_ID_DMAC_CHAN_MGMT_ROM_C        = 2737,
    OAM_FILE_ID_DMAC_DATA_ACQ_ROM_C         = 2738,
    OAM_FILE_ID_DMAC_USER_TRACK_ROM_C       = 2739,
    OAM_FILE_ID_DMAC_RX_FILTER_ROM_C        = 2740,

    OAM_FILE_ID_DMAC_11W_ROM_C              = 2741,
    OAM_FILE_ID_DMAC_TXOPPS_ROM_C           = 2742,
    OAM_FILE_ID_DMAC_DFT_ROM_C              = 2743,
    OAM_FILE_ID_DMAC_AP_PM_ROM_C            = 2744,
    OAM_FILE_ID_MAC_PM_ROM_C                = 2745,
    OAM_FILE_ID_DMAC_SIMPLE_SCHEDULE_ROM_C  = 2746,
    OAM_FILE_ID_DMAC_HCC_ADAPT_ROM_C        = 2747,
    OAM_FILE_ID_MAC_BOARD_ROM_C             = 2748,
    OAM_FILE_ID_DMAC_P2P_ROM_C              = 2749,
    OAM_FILE_ID_DMAC_BTCOEX_ROM_C           = 2750,

    OAM_FILE_ID_DMAC_STA_PM_ROM_C           = 2751,
    OAM_FILE_ID_DMAC_UAPSD_STA_ROM_C        = 2752,
    OAM_FILE_ID_DMAC_PSM_STA_ROM_C          = 2753,
    OAM_FILE_ID_DMAC_PM_STA_ROM_C           = 2754,
    OAM_FILE_ID_MAC_DATA_ROM_C              = 2755,
    OAM_FILE_ID_DMAC_LTECOEX_ROM_C          = 2756,
    OAM_FILE_ID_MAC_11I_ROM_C               = 2757,
    OAM_FILE_ID_DMAC_DFX_ROM_C              = 2758,
    OAM_FILE_ID_DMAC_DEVICE_ROM_C           = 2759,
    OAM_FILE_ID_DMAC_ARP_OFFLOAD_ROM_C      = 2760,

    OAM_FILE_ID_DMAC_RESOURCE_ROM_C         = 2761,
    OAM_FILE_ID_DMAC_AUTO_ADJUST_FREQ_ROM_C = 2762,
    OAM_FILE_ID_DMAC_CONFIG_DEBUG_ROM_C     = 2763,
    OAM_FILE_ID_DMAC_GREEN_AP_ROM_C         = 2764,
    OAM_FILE_ID_DMAC_11K_ROM_C              = 2765,
    OAM_FILE_ID_DMAC_FTM_ROM_C              = 2766,
    OAM_FILE_ID_DMAC_SMPS_ROM_C             = 2767,
    OAM_FILE_ID_DMAC_OPMODE_ROM_C           = 2768,
    OAM_FILE_ID_DMAC_M2S_ROM_C              = 2769,
    OAM_FILE_ID_DMAC_AUTO_CALI_ROM_C        = 2770,

    OAM_FILE_ID_DMAC_USER_EXTEND_ROM_C      = 2771,
    OAM_FILE_ID_DMAC_PKT_CAPTURE_ROM_C      = 2772,
#ifdef _PRE_WLAN_FEATURE_WMMAC
    OAM_FILE_ID_DMAC_WMMAC_ROM_C            = 2773,
#endif
    OAM_FILE_ID_DMAC_CRYPTO_COMM_ROM_C      = 2774,
    OAM_FILE_ID_DMAC_CRYPTO_WEP_ROM_C       = 2775,
    OAM_FILE_ID_DMAC_CRYPTO_TKIP_ROM_C      = 2776,
    OAM_FILE_ID_DMAC_CRYPTO_AES_CCM_ROM_C   = 2777,
    OAM_FILE_ID_DMAC_CSA_STA_ROM_C          = 2778,

    /* alg rom */
    OAM_FILE_ID_ALG_MAIN_ROM_C              = 3000,
    OAM_FILE_ID_ALG_AUTORATE_LOG_ROM_C      = 3001,
    OAM_FILE_ID_ALG_AUTORATE_ROM_C          = 3002,

#ifdef _PRE_WLAN_FEATURE_QOS_ENHANCE
    OAM_FILE_ID_DMAC_TX_QOS_ENHANCE_C       = 3003,
    OAM_FILE_ID_DMAC_TX_QOS_ENHANCE_H       = 3004,
#endif
#ifdef _PRE_WLAN_FEATURE_APF
    OAM_FILE_ID_DMAC_APF_C                  = 3005,
    OAM_FILE_ID_DMAC_APF_H                  = 3006,
#endif
    OAM_FILE_ID_BUTT
}oam_file_id_enum;
typedef oal_uint32 oam_file_id_enum_uint32;


/*****************************************************************************
  4 全局变量声明
*****************************************************************************/


/*****************************************************************************
  5 消息头定义
*****************************************************************************/


/*****************************************************************************
  6 消息定义
*****************************************************************************/


/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/


/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/


#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of oam_wdk.h */

