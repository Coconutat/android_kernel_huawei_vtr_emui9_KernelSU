#ifndef _H_HW_EXTERN_PMIC_
#define _H_HW_EXTERN_PMIC_
int hw_extern_pmic_config(int index, int voltage, int enable);
int hw_extern_pmic_query_state(int index, int *state);
#endif