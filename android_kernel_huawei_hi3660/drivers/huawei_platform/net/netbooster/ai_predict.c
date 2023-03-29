

#include "ai_predict.h"
#include <huawei_platform/log/hw_log.h>

#ifdef CONFIG_APP_QOE_AI_PREDICT
#define NETWORK_STATUS_APP_QOE_NORMAL  (4)
#define NETWORK_STATUS_APP_QOE_GENERAL_SLOW (5)

judge_rlt_info_int_calc_type judge_single_smp_using_clf_info_int_exp(classifier_info_int_exp_type clf_info, AI_INT x_smp[], AI_INT app_qoe_level)
{
	/*meaning: FALSE: not psSlow; 1.0: all as psNormal.*/
	judge_rlt_info_int_calc_type rlt_info1 = {FALSE, 0, 0.0, 1.0, 0, ADA_SUB_CLFS_NUM};
	AI_INT sub_clfs_num = 0;/*sub-clfs number.*/
	AI_INT i = 0;

	AI_INT is_ps_slow = FALSE;/*default is ps normal.*/
	AI_DOUBLE final_clf_value = 0;
	AI_DOUBLE fake_bts_agg_value = 0;
	AI_DOUBLE true_bts_agg_value = 0;
	AI_INT vote_negative = 0;/*ps normal: 0.*/
	AI_INT vote_positvie = 0;/*ps slow: 1.*/
	AI_DOUBLE tmp_int = 0;
	AI_INT slow_proba_threshold = 510;

	sub_clfs_num = clf_info.sub_clfs_num;
	for (i = 0; i < sub_clfs_num; i++) {
		AI_INT cur_thres = clf_info.pp_param[i][0];/*refer: json_parse.c: parse_json_string_for_adaboost().*/
        AI_INT weight = clf_info.pp_param[i][1];
		AI_INT sel_feat_idx = clf_info.pp_param[i][2];
        AI_INT left_child_is_fake = clf_info.pp_param[i][3];
		AI_INT cur_judge_value = 1;/*value: 1 or -1.*/

		/*compute final_clf_value.*/
		if (x_smp[sel_feat_idx] * FLOAT_TO_INT_ENLARGE_10E3 <= cur_thres) {/*left/right child class: <= or >=   [0, 65535].*/
			cur_judge_value = 1 * left_child_is_fake;
		} else {
			cur_judge_value = -1 * left_child_is_fake;
		}

		final_clf_value += (AI_DOUBLE)(cur_judge_value * weight);
		//printf("i: %3d, final_clf_value: %10d, cur_judge_value: %10d, weight: %10d\n", i, final_clf_value, cur_judge_value, weight);

		/*get clf vote result.*/
		if (cur_judge_value == 1) {
			fake_bts_agg_value += (AI_DOUBLE)(cur_judge_value * weight);/*+ positive.*/
			vote_positvie += 1;
		} else {
			true_bts_agg_value += (AI_DOUBLE)(cur_judge_value * weight);/*- negative.*/
			vote_negative += 1;
		}
	}

	/*get final judge result.*/
	tmp_int = fake_bts_agg_value - true_bts_agg_value;
	if (tmp_int == 0)
		tmp_int = 1;
	rlt_info1.ps_slow_proba = fake_bts_agg_value*1000/tmp_int;

	rlt_info1.final_clf_value = final_clf_value;
	rlt_info1.ps_norm_proba = -true_bts_agg_value*1000/tmp_int;
	rlt_info1.vote_positvie = vote_positvie;
	rlt_info1.vote_negative = vote_negative;

	pr_info("judge_single_smp_using_clf_info_int_exp final_clf_value=0x%x,fake_bts_agg_value=0x%x,ps_slow_proba=%d\n",
		final_clf_value, fake_bts_agg_value, rlt_info1.ps_slow_proba);

	if (app_qoe_level == NETWORK_STATUS_APP_QOE_NORMAL) {
		slow_proba_threshold = 540;//525;
	} else if (app_qoe_level == NETWORK_STATUS_APP_QOE_GENERAL_SLOW) {
		slow_proba_threshold = 500;
	} else 	{
		slow_proba_threshold = 510;
	}

    if(final_clf_value > 0 && rlt_info1.ps_slow_proba > slow_proba_threshold){
		is_ps_slow = TRUE;/*ps slow.*/
	} else {
		is_ps_slow = FALSE;/*ps normal.*/
	}
	rlt_info1.is_ps_slow = is_ps_slow;/*get final judge result.*/

	return rlt_info1;
}
#endif