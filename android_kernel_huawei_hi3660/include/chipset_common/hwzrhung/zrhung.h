#ifndef __ZRHUNG_H
#define __ZRHUNG_H

#include <linux/types.h>
#include <linux/ioctl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ZRHUNG_CAT_SYS,
    ZRHUNG_CAT_APPEYE,
    ZRHUNG_CAT_EVENT,
    ZRHUNG_CAT_NUM_MAX,
    //CATs below dont have config
    ZRHUNG_CAT_XCOLLIE
} zrhung_cat_type;

typedef enum {
    //system Zero Hung watchpoint
    ZRHUNG_WP_NONE = ZRHUNG_CAT_SYS << 8 | 0x0000,
    ZRHUNG_WP_HUNGTASK,
    ZRHUNG_WP_SCREENON,
    ZRHUNG_WP_SCREENOFF,
    ZRHUNG_WP_SR,
    ZRHUNG_WP_LMKD,
    ZRHUNG_WP_IO,
    ZRHUNG_WP_CPU,
    ZRHUNG_WP_GC,
    ZRHUNG_WP_TP,
    ZRHUNG_WP_FENCE,
    ZRHUNG_WP_SCRNONFWK,
    ZRHUNG_WP_SERVICE,
    ZRHUNG_WP_GPU,
    ZRHUNG_WP_TRANS_WIN,
    ZRHUNG_WP_FOCUS_WIN,
    ZRHUNG_WP_LCD,
    ZRHUNG_WP_RES_LEAK,
    ZRHUNG_WP_IPC_THREAD,
    ZRHUNG_WP_IPC_OBJECT,
    ZRHUNG_WP_UFS,
    ZRHUNG_WP_INIT,
    ZRHUNG_WP_VMWATCHDOG,
    ZRHUNG_WP_APPFREEZE,
    ZRHUNG_WP_APPFRZ_WARNING,
    ZRHUNG_WP_HTSK_WARNING,
    ZRHUNG_WP_FDLEAK,
    ZRHUNG_WP_CAMERA,
    ZRHUNG_WP_UPLOAD_BIGDATA,
    ZRHUNG_WP_TEMP1,
    ZRHUNG_WP_TEMP2,
    ZRHUNG_WP_TEMP3,
    ZRHUNG_WP_MAX,

    //application eye
    APPEYE_NONE = ZRHUNG_CAT_APPEYE << 8 | 0x0000,
    APPEYE_UIP_WARNING,
    APPEYE_UIP_FREEZE,
    APPEYE_UIP_SLOW,
    APPEYE_UIP_INPUT,
    APPEYE_MTO_FREEZE,
    APPEYE_MTO_SLOW,
    APPEYE_MTO_INPUT,
    APPEYE_BF,
    APPEYE_CL,
    APPEYE_CLA,
    APPEYE_ARN,
    APPEYE_MANR,
    APPEYE_CANR,
    APPEYE_FWB_WARNING,
    APPEYE_FWB_FREEZE,
    APPEYE_NFW,
    APPEYE_TWIN,
    APPEYE_FWE,
    APPEYE_SBF,
    APPEYE_RCV,
    APPEYE_OBS,
    APPEYE_RECOVER_RESULT,
    APPEYE_MT,
    APPEYE_UIP_RECOVER,
    APPEYE_BINDER_TIMEOUT,
    APPEYE_SBF_FREEZE,
    APPEYE_SBF_RECOVER,
    APPEYE_SUIP_WARNING,
    APPEYE_SUIP_FREEZE,
    APPEYE_SUIP_INPUT,
    APPEYE_CRASH,
    APPEYE_TEMP1,
    APPEYE_TEMP2,
    APPEYE_TEMP3,

    APPEYE_MAX,

    //system event watchpoint
    ZRHUNG_EVENT_NONE = ZRHUNG_CAT_EVENT << 8 | 0x0000,
    ZRHUNG_EVENT_LONGPRESS,
    ZRHUNG_EVENT_POWERKEY,
    ZRHUNG_EVENT_HOMEKEY,
    ZRHUNG_EVENT_BACKKEY,
    ZRHUNG_EVENT_MULTIPRESS,
    ZRHUNG_EVENT_TEMP1,
    ZRHUNG_EVENT_TEMP2,
    ZRHUNG_EVENT_TEMP3,

    ZRHUNG_XCOLLIE_NONE = ZRHUNG_CAT_XCOLLIE << 8 | 0x0000,
    XCOLLIE_FWK_SERVICE,

    ZRHUNG_XCOLLIE_MAX,

    ZRHUNG_EVENT_MAX
} zrhung_wp_id;

#define ZRHUNG_CMD_LEN_MAX (512)
#define ZRHUNG_MSG_LEN_MAX (15 * 1024)
#define ZRHUNG_CMD_INVALID (0xFF)

#define ZRHUNG_CFG_VAL_LEN_MAX (512)
#define ZRHUNG_CFG_CAT_NUM_MAX (200)
#define ZRHUNG_CFG_ENTRY_NUM (ZRHUNG_CAT_NUM_MAX * ZRHUNG_CFG_CAT_NUM_MAX)

#define ZRHUNG_WPID(wp) (wp & 0x00FF)
#define ZRHUNG_WPCAT(wp) (wp >> 8 & 0x00FF)

#define ZRHUNG_WP_TO_ENTRY(wp) (ZRHUNG_WPCAT(wp) * \
ZRHUNG_CFG_CAT_NUM_MAX + ZRHUNG_WPID(wp))

typedef struct {
	uint32_t magic;
	uint16_t len;
	uint16_t wp_id;
	uint16_t cmd_len; // end with '\0', = cmd actual len + 1
	uint16_t msg_len; // end with '\0', = msg actual len + 1
	char info[0];     // <cmd buf>0<buf>0
} zrhung_write_event;

int zrhung_is_id_valid(short wp_id);
int zrhung_send_event(zrhung_wp_id id, const char* cmd_buf, const char*  buf);
int zrhung_get_config(zrhung_wp_id id, char *data, uint32_t maxlen);
long zrhung_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
long zrhung_save_lastword(void);

int lmkwp_init(void);
void lmkwp_report(struct task_struct *selected, struct shrink_control *sc, long cache_size, long cache_limit, short adj, long free);
#ifdef __cplusplus
}
#endif
#endif /* __ZRHUNG_H */

