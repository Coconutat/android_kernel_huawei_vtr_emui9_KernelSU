#ifndef NETWORK_EVALUATION_H_INCLUDED
#define NETWORK_EVALUATION_H_INCLUDED

#include "evaluation_common.h"

/*****************************************************************************
  2 宏定义
*****************************************************************************/
/* misc values */
#define ADEQUATE_RTT          8

/*****************************************************************************
  3 结构体定义
*****************************************************************************/
typedef struct sub_indices_report
{
	int8_t traffic_index;
	int8_t rtt_stability_index;
	int8_t reserved_1;         // for alignment
	int8_t reserved_2;
}SUB_INDICES_REPORT;
/*****************************************************************************
  4 函数声明
*****************************************************************************/
void nweval_init(void);
void nweval_update_rtt(struct sock* sk, long rtt_us);
void nweval_start_rtt_report(int8_t network_type);
void nweval_stop_rtt_report(void);
void nweval_trigger_rtt_report(int32_t signal_strength);
void nweval_event_process(int32_t event, uint8_t *pdata, uint16_t len);
void nweval_on_dk_connected(void);

#endif // NETWORK_EVALUATION_H_INCLUDED
