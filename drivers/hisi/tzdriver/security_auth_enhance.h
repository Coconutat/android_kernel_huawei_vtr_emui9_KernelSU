#ifndef _SECURITY_AUTH_ENHANCE_H_
#define _SECURITY_AUTH_ENHANCE_H_

#include "teek_ns_client.h"

#define INC    0x01
#define DEC    0x00
#define UN_SYNCED    0x55
#define IS_SYNCED    0xaa

TC_NS_DEV_File *tc_find_dev_file(unsigned int dev_file_id);
TC_NS_Session *tc_find_session2(unsigned int dev_file_id,
		unsigned int context_id, unsigned char *uuid);
int32_t update_timestamp(TC_NS_SMC_CMD *cmd);
int32_t update_chksum(TC_NS_SMC_CMD *cmd);
int32_t verify_chksum(TC_NS_SMC_CMD *cmd);
int32_t sync_timestamp(TC_NS_SMC_CMD *cmd, uint8_t *token, unsigned char *uuid, bool global);

#endif
