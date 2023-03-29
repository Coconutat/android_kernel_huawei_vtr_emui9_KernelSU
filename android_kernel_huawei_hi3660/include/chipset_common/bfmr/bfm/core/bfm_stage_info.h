/**
    @copyright: Huawei Technologies Co., Ltd. 2016-xxxx. All rights reserved.

    @file: bfm_trace_boottime.h

    @brief: define the trace_boottime's external public enum/macros/interface for BFM (Boot Fail Monitor)

    @version: 2.0

    @author: YangJie ID: 00202466

    @date: 2018-04-12

    @history:
*/

#ifndef BFM_TRACE_STAGETIME_H
#define BFM_TRACE_STAGETIME_H


/*----includes-----------------------------------------------------------------------*/

#include <chipset_common/bfmr/public/bfmr_public.h>
#include <chipset_common/bfmr/common/bfmr_common.h>

/*----c++ support-------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif


/*----export macroes-----------------------------------------------------------------*/
#define BMFR_MAX_BOOTUP_STAGE_COUNT 256

/*----export prototypes---------------------------------------------------------------*/
typedef struct bfm_stage_info
{
    bfmr_detail_boot_stage_e stage;
    unsigned int             start_time;    /* seconds */
	char                     reserved[32];
}bfm_stage_info_t;

typedef struct bfm_stages
{
  unsigned int stage_count;
  bfm_stage_info_t  stage_info_list[BMFR_MAX_BOOTUP_STAGE_COUNT];
}bfm_stages_t;

/*----global variables----------------------------------------------------------------*/


/*----export function prototypes--------------------------------------------------------*/

/**
    @function: bfm_add_stage_info
    @brief: record the stage information into list

    @param: pStage[in], the stageinfo pointer

    @return:none

    @note:
*/
void bfm_add_stage_info(bfm_stage_info_t *pStage);

/**
    @function: bfm_save_stages_info_txt
    @brief: append the stages info into bs_data

    @param: buffer[inout], the buffer to record information
    @param: buffer_len[in], the size of buffer

    @return:the length to need print

    @note:
*/
size_t bfm_save_stages_info_txt(char *buffer, unsigned int buffer_len);

/**
    @function: int bfm_init_stage_info(void)
    @brief: init trace time module resource

    @param: none.

    @return: 0 - succeeded; -1 - failed.

    @note: it need be initialized in kernel.
*/
int bfm_init_stage_info(void);

/**
    @function: void bfm_deinit_stage_info(void)
    @brief: release trace time module resource

    @param: none.

    @return:none

    @note: it need be called after bootsuccess & save log.
*/
void bfm_deinit_stage_info(void);

#ifdef __cplusplus
}
#endif

#endif

