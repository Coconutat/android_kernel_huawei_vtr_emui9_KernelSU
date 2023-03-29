#ifndef _WIRED_CHANNEL_SWITCH_H_
#define _WIRED_CHANNEL_SWITCH_H_

#define WIRED_CHANNEL_CUTOFF          (1)
#define WIRED_CHANNEL_RESTORE         (0)

#define WIRED_REVERSE_CHANNEL_CUTOFF  (1)
#define WIRED_REVERSE_CHANNEL_RESTORE (0)


struct wired_chsw_device_ops {
	int (*set_wired_channel)(int);
	int (*get_wired_channel)(void);
	int (*set_wired_reverse_channel)(int);
};

extern int wired_chsw_ops_register(struct wired_chsw_device_ops *ops);

extern int wired_chsw_set_wired_reverse_channel(int flag);
extern int wired_chsw_set_wired_channel(int flag);
extern int wired_chsw_get_wired_channel(void);

#endif /* end of _WIRED_CHANNEL_SWITCH_H_ */
