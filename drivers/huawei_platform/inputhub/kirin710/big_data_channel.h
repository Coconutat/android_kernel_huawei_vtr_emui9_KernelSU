#ifndef __BIG_DATA_CHANNEL_H__
#define __BIG_DATA_CHANNEL_H__

typedef enum {
	INT_PARAM,
	STR_PARAM,
} big_data_param_type_t;

typedef enum {
	BIG_DATA_STR,
	//BIG_DATA_STR_2,
} big_data_str_tag_t;

typedef struct {
	const char *param_name;
	big_data_param_type_t param_type;
} big_data_param_detail_t;

typedef struct {
	int event_id;
	int param_num;
	big_data_param_detail_t *param_data;
} big_data_event_detail_t;

uint64_t iomcu_dubai_log_fetch(uint8_t event_type);

#endif
