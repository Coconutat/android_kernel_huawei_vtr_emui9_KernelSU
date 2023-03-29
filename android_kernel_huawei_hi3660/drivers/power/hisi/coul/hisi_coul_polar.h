#ifndef _HISI_COUL_POLAR_H_
#define _HISI_COUL_POLAR_H_

extern void stop_polar_sample(void);
extern void start_polar_sample(void);
extern void get_resume_polar_info(int eco_ibat, int curr, int duration, int sample_time, int temp, int soc);
extern void sync_sample_info(void);
extern int polar_params_calculate(struct polar_calc_info* polar,
                           int ocv_soc_mv, int vol_now, int cur, bool update_a);
extern int polar_ocv_params_calc(struct polar_calc_info* polar,
                                int batt_soc_real, int temp, int cur);
extern bool is_polar_list_ready(void);
extern void polar_clear_flash_data(void);
extern void clear_polar_err_b(void);

#endif
