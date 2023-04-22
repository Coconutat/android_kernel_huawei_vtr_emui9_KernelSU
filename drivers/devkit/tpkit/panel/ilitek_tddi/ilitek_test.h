#ifndef __ILITEK_TEST_H__
#define __ILITEK_TEST_H__

#include "ilitek_mp_test.h"

#define ILITEK_TEST_DATA_LEN                   600
#define ILITEK_TEST_MODULE_LEN                 64
#define ILITEK_TEST_FAILED_REASON_LEN          24
#define ILITEK_TEST_ITEMS                      7
#define ILITEK_TEST_ITEM_RES_LEN               3

#define ILITEK_SWITCH_TEST_MODE_RETRY          3

struct ilitek_test {
    bool test_result;
    int orignal_data[ILITEK_TEST_ITEMS][ILITEK_TEST_DATA_LEN];
};

int ilitek_test_hash(mp_tests key);
int ilitek_get_raw_data(struct ts_rawdata_info *info,
    struct ts_cmd_node *out_cmd);
int ilitek_rawdata_print(struct seq_file *m, struct ts_rawdata_info *info,
    int range_size, int row_size);
int ilitek_test_init (void);
void ilitek_test_exit(void);
#endif
