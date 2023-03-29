#ifndef __USBAUDIO_SETINTERFACE_
#define __USBAUDIO_SETINTERFACE_

int usbaudio_ctrl_set_pipeout_interface(unsigned int running, unsigned int rate);
int usbaudio_ctrl_set_pipein_interface(unsigned int running, unsigned int rate);
void usbaudio_ctrl_nv_check(void);
#endif /* __USBAUDIO_DSP_CLIENT_ */
