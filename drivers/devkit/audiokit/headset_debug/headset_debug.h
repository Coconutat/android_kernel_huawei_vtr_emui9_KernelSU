#ifndef __HEADSET_DEBUG_H__
#define __HEADSET_DEBUG_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif
struct headset_debug {
	struct snd_soc_jack *jack;
	struct switch_dev *sdev;
	struct input_dev *input_dev;
	struct dentry * df_dir;
	atomic_t state;
};

enum headset_debug_jack_type {
	HEADSET_DEBUG_JACK_BIT_NONE = 0,
	HEADSET_DEBUG_JACK_BIT_HEADSET,
	HEADSET_DEBUG_JACK_BIT_HEADPHONE,
	HEADSET_DEBUG_JACK_BIT_HEADSET_NO_MIC,
	HEADSET_DEBUG_JACK_BIT_PLUGING,
	HEADSET_DEBUG_JACK_BIT_INVALID,
	HEADSET_DEBUG_JACK_BIT_LINEOUT,
};

enum headset_debug_key_type {
	HEADSET_DEBUG_KEY_0 = 10,
	HEADSET_DEBUG_KEY_1,
	HEADSET_DEBUG_KEY_2,
	HEADSET_DEBUG_KEY_3,
	HEADSET_DEBUG_KEY_4,
	HEADSET_DEBUG_KEY_5,
};

/* Debug info */
#define ERROR_LEVEL     1
#define INFO_LEVEL      1
#define DEBUG_LEVEL     0

#if INFO_LEVEL
#define logi(fmt, ...) pr_info(LOG_TAG"[I]:%s:%d: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define logi(fmt, ...)
#endif

#if DEBUG_LEVEL
#define logd(fmt, ...) pr_info(LOG_TAG"[D]:%s:%d: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define logd(fmt, ...)
#endif

#if ERROR_LEVEL
#define loge(fmt, ...) pr_err(LOG_TAG"[E]:%s:%d: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define loge(fmt, ...)
#endif

extern void headset_debug_init(struct snd_soc_jack *jack, struct switch_dev *sdev);
extern void headset_debug_set_state(int state, bool use_input);
extern void headset_debug_uninit(void);
extern void headset_debug_input_init(struct input_dev *accdet_input_dev);
extern void headset_debug_input_set_state(int state, bool use_input);
extern void headset_debug_input_uninit(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
