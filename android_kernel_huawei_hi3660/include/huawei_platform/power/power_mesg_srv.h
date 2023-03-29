#ifndef POWER_MESG_SRV_H
#define POWER_MESG_SRV_H

#define BATT_SHUTDOWN_MAGIC1                        "fee1dead"
#define BATT_SHUTDOWN_MAGIC2                        "gotodead"
#define BLIMSW_NV_NUMBER                            388
#define BLIMSW_NV_NAME                              "BLIMSW"
#define BBINFO_NV_NUMBER                            389
#define BBINFO_NV_NAME                              "BBINFO"

#define FACTORY_VERSION_MODE_FLAG                   0x00
#define COMMERCIAL_VERSION_MODE_FLAG                0x88

enum {
    ILLEGAL_BIND_VERSION = 0,
    RAW_BIND_VERSION,
};

enum {
    IC_STATUS_OFFSET = 0,
    KEY_STATUS_OFFSET,
    SN_STATUS_OFFSET,
    CHECK_MODE_OFFSET,
    MAGIC_NUM_OFFSET,
};
#define CHECK_RESULT_LEN                (CHECK_MODE_OFFSET + 1)
#define BATT_SN_OFFSET                  (MAGIC_NUM_OFFSET + strlen(BATT_SHUTDOWN_MAGIC1))
#define MAX_SN_LEN                      20
#define MAX_SN_BUFF_LENGTH              5

typedef struct {
    unsigned int version;
    char info[MAX_SN_BUFF_LENGTH][MAX_SN_LEN];
} binding_info;

enum SN_CR{
    SN_PASS = 0,
    SN_OBD_REMATCH,     //old board rematch new battery
    SN_OBT_REMATCH,     //old battery rematch new board
    SN_NN_REMATCH,      //new board & new battery rematch
    SN_FAIL_NV_TIMEOUT, //SN get from NV timeout
    SN_UNREADY,         //SN is under checking
    SN_FAIL_IC_TIMEOUT, //SN get from IC timeout
    SN_OO_UNMATCH,      //old board & old battery unmatch
};

enum CHECK_MODE {
    FACTORY_CHECK_MODE = 0,
    COMMERCIAL_CHECK_MODE = 15,
};

/* cmd: 00-49 is for battery
 *      50-99 is for wireless charging
 *      100-149 is for directly charging
 *      max is 255
 */
typedef enum {
    BATT_MAXIM_SECRET_MAC = 0,
//    BATT_MAXIM_EEPROM_MAC,
//    BATT_MAXIM_STATUS_MAC,
    BATT_SHUTDOWN_CMD = 40,
    POWER_CMD_ADAPTOR_ANTIFAKE_HASH = 50,
    POWER_CMD_WC_ANTIFAKE_HASH = 60,
    POWER_CMD_TOTAL_NUM = 256,
} power_genl_cmd_t;

#endif
