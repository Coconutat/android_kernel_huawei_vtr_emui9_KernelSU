#ifndef __SOUNDTRIGGER_SOCDSP_PCM_H
#define __SOUNDTRIGGER_SOCDSP_PCM_H

void soundtrigger_socdsp_pcm_flag_init(void);
int soundtrigger_socdsp_pcm_fastbuffer_filled(unsigned int fast_len);
int soundtrigger_socdsp_pcm_init(void);
int soundtrigger_socdsp_pcm_deinit(void);
int soundtrigger_socdsp_pcm_elapsed(int start, int buffer_len);
#endif
