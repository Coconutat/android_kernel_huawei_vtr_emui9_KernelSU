#ifndef __HISI_HISEE_AUTOTEST_H__
#define __HISI_HISEE_AUTOTEST_H__

/* channel test for hisee */
#define TEST_DIRECTORY_PATH     "/data/hisee_test"
#define TEST_SUCCESS_FILE       "/data/hisee_test/test_success.txt"
#define TEST_FAIL_FILE          "/data/hisee_test/test_fail.txt"
#define TEST_RESULT_FILE        "/data/hisee_test/test_result.img"

#define HISEE_CHANNEL_TEST_CMD_ERROR  (-6000)
#define HISEE_CHANNEL_TEST_RESULT_MALLOC_ERROR  (-6001)
#define HISEE_CHANNEL_TEST_PATH_ABSENT_ERROR  (-6002)
#define HISEE_CHANNEL_TEST_WRITE_RESULT_ERROR  (-6003)

#define bypass_space_char() \
do {\
	if (offset >= HISEE_CMD_NAME_LEN) {\
		pr_err("hisee_cmd is bad.\n");\
		pr_err("%s(): hisee channel test cmd is bad\n", __func__);\
		set_errno_and_return(HISEE_CHANNEL_TEST_CMD_ERROR);\
	}\
	if (0x20 != buff[offset])\
		break;\
	offset++;\
} while (1)

#define ALIGN_UP_4KB(x)     (((x) + 0xFFF)&(~0xFFF))
#define CHANNEL_TEST_RESULT_SIZE_DEFAULT    (0x40000)
#define TEST_RESULT_SIZE_DEFAULT     (0x40000)

int hisee_channel_test_func(void *buf, int para);

#endif
