

#ifndef ai_predict__h
#define ai_predict__h

//type definition for platform migration.
#define AI_DOUBLE      long
#define AI_INT         int
#define TRUE    1
#define FALSE   0

#define FEATURE_NUM  10    //features numbers in train data sets.
#define ADA_SUB_CLFS_NUM      300

/*use for float to int  transformation process.
attation:need avoid overflow. int is 32bit(4 billion) in Hisi.*/
#define FLOAT_TO_INT_ENLARGE_10E3   1000         //for 'threshold' param.  [0,65535]
#define FLOAT_TO_INT_ENLARGE_10E6   1000000      //for 'weight' param.  maybe a float number.

/* The ClassifierInfo structure using int expression. */
typedef struct classifier_info_int_exp {
	AI_INT     (*pp_param)[4];
	AI_INT     sub_clfs_num;        //sub-clfs number.
} classifier_info_int_exp_type;

/* The ClassifierInfo structure: */
typedef struct judge_rlt_info_int_calc {
	AI_INT     is_ps_slow;     //if it's a ps slow yes or not.  when TRUE, it's a psSlow. when FALSE, it's a psNormal.
	AI_INT     final_clf_value;//the total judge value. Generally, when > 0, regard as a ps slow.

	AI_DOUBLE  ps_slow_proba;  //the probability if if it's a ps slow.
	AI_DOUBLE  ps_norm_proba;  //the probability if if it's a ps normal.
	AI_INT     vote_positvie;  //the sub-clfs numbers that vote as ps slow.
	AI_INT     vote_negative;  //the sub-clfs numbers that vote as ps normal.
} judge_rlt_info_int_calc_type;

/* Returns the AI(algorithm module) judgement result with Adaboost Algorithm. */
judge_rlt_info_int_calc_type judge_single_smp_using_clf_info_int_exp(classifier_info_int_exp_type clf_info, AI_INT x_smp[], AI_INT app_qoe_level);


#endif
