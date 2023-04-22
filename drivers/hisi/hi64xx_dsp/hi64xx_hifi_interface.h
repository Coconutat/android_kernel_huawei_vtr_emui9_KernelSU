#ifndef __HI64XX_HIFI_INTERFACE_H__
#define __HI64XX_HIFI_INTERFACE_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#define DRV_SEND_DATA_STRU                                                      \
    unsigned long                           uwBufAddr;                          \
    unsigned long                           uwBufSize;                          \
    unsigned long                           uwDataSize;

#define HIFI_AUDIO_PCM_COMP_COEF_LEN_MAX    (128)
#define HOOK_PATH_MAX_LENGTH (200)
#define CODECDSP_FW_NAME_MAX_LENGTH (64)

typedef enum {
	/* codec dspif pos begin */
	HOOK_POS_IF0               = 0x00000001,
	HOOK_POS_IF1               = 0x00000002,
	HOOK_POS_IF2               = 0x00000004,
	HOOK_POS_IF3               = 0x00000008,
	HOOK_POS_IF4               = 0x00000010,
	HOOK_POS_IF5               = 0x00000020,
	HOOK_POS_IF6               = 0x00000040,
	HOOK_POS_IF7               = 0x00000080,
	HOOK_POS_IF8               = 0x00000100,
	/* codec dspif pos end */

	/* codec dsp inner data pos begin */
	HOOK_POS_LOG               = 0x00000200,
	HOOK_POS_ANC_CORE          = 0x00000400,
	HOOK_POS_ANC_PCM_RX_VOICE  = 0x00000800,
	HOOK_POS_ANC_PCM_REF       = 0x00001000,
	HOOK_POS_ANC_PCM_ERR       = 0x00002000,
	HOOK_POS_ANC_PCM_ANTI      = 0x00004000,
	HOOK_POS_ANC_PCM_TX_VOICE  = 0x00008000,
	HOOK_POS_ANC_ALG_INDICATE  = 0x00010000,
	HOOK_POS_ANC_COEF          = 0x00020000,
	HOOK_POS_PA_EFFECTIN       = 0x00040000,
	HOOK_POS_PA_EFFECTOUT      = 0x00080000,
	HOOK_POS_WAKEUP_MICIN      = 0x00100000,
	HOOK_POS_WAKEUP_EFFECTOUT  = 0x00200000,
	HOOK_POS_PA1_I             = 0x00400000,
	HOOK_POS_PA1_V             = 0x00800000,
	HOOK_POS_PA2_I             = 0x01000000,
	HOOK_POS_PA2_V             = 0x02000000,
	HOOK_POS_ANC_BETA_CSINFO   = 0x04000000,
	/* codec dsp inner data pos end */

	HOOK_POS_BUTT,
	HOOK_POS_NONE_TEST         = 0x10000000,
}hook_pos;

enum {
	OM_BANDWIDTH_6144 = 0,
	OM_BANDWIDTH_12288,
	OM_BANDWIDTH_BUTT,
};

enum {
	OM_SPONSOR_DEFAULT = 0,
	OM_SPONSOR_BETA,
	OM_SPONSOR_TOOL,
	OM_SPONSOR_BUTT,
};

enum  {
    ANC_MSG_START_HOOK = 0xAA00,
    ANC_MSG_STOP_HOOK = 0xAA01,
    ANC_MSG_TRIGGER_DFT = 0xAA02,
    DSM_MSG_DUAL_STATIC0 = 0xAA03,
    DSM_MSG_DUAL_STATIC1 = 0xAA04,
    DSM_MSG_MONO_STATIC0 = 0xAA05,
    DSM_MSG_PARAM = 0xAA06,
    PA_MSG_BUFFER_REVERSE = 0xAA07,
    OM_MSG_BUTT
};

enum {
    ANC_MODULE = 0,
    SMARTPA_MODULE,
};

typedef enum {
    NO_ERR                = 0,
    HOWLING_ERR           = 1,     //啸叫问题
    WIND_NOISE_ERR        = 2,     //风噪问题
    COEF_DIVERGENCE_ERR   = 3,     //系数发散问题
    LOW_ACTTIME_RATE      = 4,     //生效时间占比低
    PROCESS_PATH_ERR      = 5,     //模块通路问题
    OTD_ERR               = 6,     //芯片杂音
}ErrorClass;

enum {
    DSM_OM_ERR_TYPE_PROC = 1, //imonitor required this start from 1
    DSM_OM_ERR_TYPE_PARA_SET,
    DSM_OM_ERR_TYPE_MALLOC,
    DSM_OM_ERR_TYPE_STATUS
};

typedef enum{
    ERR_LEVEL_WARNING     = 0,
    ERR_LEVEL_ERROR,
}ErrorLevel ;

enum DSP_POWER_STATE_ENUM
{
    HIFI_POWER_ON = 0xA0,
    HIFI_POWER_CLK_ON = 0xB0,
    HIFI_POWER_CLK_OFF,
};

enum MLIB_PATH_ENUM
{
    MLIB_PATH_DATA_HOOK     = 0,
    MLIB_PATH_DEFAULT,
    MLIB_PATH_WAKEUP,
    MLIB_PATH_SMARTPA,
    MLIB_PATH_ANC,
    MLIB_PATH_ANC_DEBUG,
    MLIB_PATH_IR_LEARN,
    MLIB_PATH_IR_TRANS,
    MLIB_PATH_BUTT
};
typedef unsigned int MLIB_PATH_ENUM_UINT32;


enum MLIB_MODULE_ENUM
{
	MLIB_MODULE_DEFAULT = 0,
	MLIB_MODULE_IVW,         /* kedaxunfei */
	MLIB_MODULE_SWU,         /* zhongruan */
	MLIB_MODULE_DSM,         /* mono pa */
	MLIB_MODULE_ELVIS,       /* old nuance */
	MLIB_MODULE_TSLICE,      /* sensory */
	MLIB_MODULE_OEM,         /* new nuance */
	MLIB_MODULE_GOOGLE,      /* ok google */
	MLIB_MODULE_DSM_STEREO,  /* stereo pa */
	MLIB_MODULE_ANC,
	MLIB_MODULE_TFADSP,
	MLIB_MODULE_TFADSP_STEREO,
	MLIB_MODULE_PA_BYPASS,
	MLIB_MODULE_BUTT,

	MLIB_MODULE_BROADCAST   = 0xFFFFFFFF,
};
typedef unsigned int MLIB_MODULE_ENUM_UINT32;

enum APDSP_MSG_ENUM
{
	ID_AP_DSP_IF_OPEN          = 0xDD10,
	ID_AP_DSP_IF_CLOSE         = 0xDD11,
	ID_AP_DSP_PARAMETER_SET    = 0xDD12,
	ID_AP_DSP_PARAMETER_GET    = 0xDD13,
	ID_AP_DSP_OM               = 0xDD14,

	ID_AP_DSP_IF_ST_OPEN       = 0xDD15,
	ID_AP_DSP_IF_ST_CLOSE      = 0xDD16,

	ID_AP_DSP_FASTTRANS_OPEN   = 0XDD17,
	ID_AP_DSP_FASTTRANS_CLOSE  = 0XDD18,
	ID_AP_DSP_WAKEUP_TEST      = 0XDD19,

	ID_DSP_UPDATE_BUF          = 0xDD20,
	ID_DSP_LOG_HOOK            = 0xDD21,
	ID_DSP_UPDATE_FASTBUF      = 0XDD22,
	ID_DSP_UPDATE_RECORDBUF    = 0XDD23,
	ID_AP_DSP_MADTEST_START    = 0xDD31,
	ID_AP_DSP_MADTEST_STOP     = 0xDD32,
	ID_AP_DSP_UARTMODE         = 0xDD33,
	ID_AP_DSP_FASTMODE         = 0xDD34,
	ID_AP_DSP_ANCTEST_START    = 0xDD37,
	ID_AP_DSP_ANCTEST_STOP     = 0xDD38,
	ID_AP_DSP_ANCTEST_STRESS   = 0xDD39,
	ID_AP_IMGAE_DOWNLOAD       = 0xDD40,
	ID_AP_DSP_PLL_SWITCH       = 0xDD41, /*PLL切换*/
	ID_DSP_WAKE_UP_MAD_ISR     = 0xDD42, /*MAD*/

	ID_AP_DSP_HOOK_START       = 0xDD62,
	ID_AP_DSP_HOOK_STOP        = 0xDD63,
	ID_AP_DSP_SET_OM_BW        = 0xDD64,
	ID_AP_AP_SET_HOOK_SPONSOR  = 0xDD65,
	ID_AP_AP_SET_HOOK_PATH     = 0xDD66,
	ID_AP_AP_SET_DIR_COUNT     = 0xDD67,

	ID_AP_DSP_DEBUG_MSG        = 0xDD74,
	ID_AP_DSP_OCRAM_TCM_CHECK  = 0xDD75,
	ID_FAULTINJECT             = 0xDDA0,
};

enum UART_MODE_ENUM
{
    UART_MODE_115200 = 0, /* for pc uart */
    UART_MODE_500K   = 1, /* for phone in mad mode */
    UART_MODE_6M     = 2, /* for phone in normal mode */
    UART_MODE_OFF    = 3, /* force writing log to 36K om buffer*/
};

enum IR_PARA_ID_ENUM
{
    IR_PARA_ID_CODE      = 0,
    IR_PARA_ID_FREQUENCY = 1,
    IR_PARA_ID_STATUS    = 2,
};

struct om_hook_para {
	hook_pos pos;
	unsigned short resolution;
	unsigned short channel_num;
	unsigned int sample_rate;
};

struct om_set_dir_count_msg {
	unsigned short msg_id;
	unsigned int count;
};

struct om_set_hook_sponsor_msg {
	unsigned short msg_id;
	unsigned short sponsor;
};

struct om_set_hook_path_msg {
	unsigned short msg_id;
	char hook_path[HOOK_PATH_MAX_LENGTH];
	unsigned int size;
};

struct om_start_hook_msg {
	unsigned short msg_id;
	unsigned short para_size;
};

struct om_stop_hook_msg {
	unsigned short msg_id;
	unsigned short reserve;
};

struct om_set_bandwidth_msg {
	unsigned short msg_id;
	unsigned short bw;
};

struct om_anc_stress_msg {
	unsigned short msg_id;
	unsigned short stress_id;
};

struct pa_buffer_reverse_msg {
       unsigned int msg_id;
       unsigned int pa_count;
       unsigned int proc_interval;
};

typedef struct  {
    unsigned int    err_module;     //ANC or smartPa
    unsigned int    err_class;         //err classification
    unsigned int    err_level;         //err serious level
    unsigned char details[64];    //upload information
} MLIB_ANC_DFT_INFO;

typedef struct {
    unsigned int msgSize;
    unsigned int errModule;
    unsigned int errClass;
    unsigned int errLevel;
    int errCode;
    unsigned int errLineNum;
    unsigned char errInfo[96];
} MLIB_DSM_DFT_INFO;

typedef struct
{
    unsigned short  msgID;
    unsigned short  fastTransEnable;
} FAST_TRANS_MSG_STRU;

typedef struct _PCM_IF_MSG_STRU
{
    unsigned int    uwIFId;
    unsigned int    uwIFDirect;
    unsigned int    uwSampleRateIn;
    unsigned int    uwSampleRateOut;
    unsigned int    uwChannelCount;
    unsigned int    uwDMASetId;
    unsigned int    ucResolution;

} PCM_IF_MSG_STRU;

typedef struct _PCM_PROCESS_DMA_MSG_STRU
{
   unsigned int     uwProcessId;
   unsigned int     uwIFCount;
   unsigned int     uwModuleID;
   PCM_IF_MSG_STRU  stIFCfgList[0];
} PCM_PROCESS_DMA_MSG_STRU;

typedef struct
{
    unsigned short                          uhwMsgId;   /*ID_AP_DSP_IF_OPEN 0xDD10*/
    unsigned short                          uhwPerms;
    PCM_PROCESS_DMA_MSG_STRU                stProcessDMA;
} DSP_IF_OPEN_REQ_STRU;

typedef struct
{
    unsigned short                          uhwMsgId;   /*ID_AP_DSP_FASTTRANS_CONFIG 0xDD17*/
    unsigned short                          uhwReserve;
    int                                     swStatus;
} FAST_TRANS_CFG_REQ_STRU;


/* Mlib para control*/
typedef struct
{
    unsigned short  msgID;
    unsigned short  reserve;
    unsigned int    uwProcessID;
    unsigned int    uwModuleID;
    unsigned int    uwSize;
    unsigned char   aucData[0];
} MLIB_PARA_SET_REQ_STRU;

typedef struct
{
    unsigned short  msgID;
    unsigned short  reserve;
    unsigned int    uwProcessID;
    unsigned int    uwModuleID;
    unsigned int    uwSize;
    unsigned char   aucData[0];
} MLIB_PARA_GET_REQ_STRU;

typedef struct
{
    int result;
    unsigned char para_data[0];
} MLIB_PARA_GET_DATA;

/* OM control */
enum PCM_HOOK_POINT_ENUM{
    TRACK_LOG_NONE                 = 0x00000000,
    TRACK_LOG_MAINMIC              = 0x00000001,
    TRACK_LOG_SUBMIC               = 0x00000002,
    TRACK_LOG_THIRDMIC             = 0x00000004,
    TRACK_LOG_FORTHMIC             = 0x00000008,
    TRACK_LOG_UPLINK_AFTERPRO      = 0x00000010,
    TRACK_LOG_UPLINK_AFTERCODE     = 0x00000020,
    TRACK_LOG_DOWNLINK_AFTERPRO    = 0x00000100,
    TRACK_LOG_DOWNLINK_AFTERCODE   = 0x00000200,
    TRACK_LOG_DOWNLINK_BEFORECODE  = 0x00000400,
    TRACK_LOG_SMARTPA_IN           = 0x00001000,
    TRACK_LOG_SMARTPA_OUT          = 0x00002000,
    TRACK_LOG_SMARTPA_I            = 0x00004000,
    TRACK_LOG_SMARTPA_V            = 0x00008000,
    TRACK_LOG_AEC                  = 0x00010000,
    TRACK_LOG_SOUNDTRIGGER_KEYWORD = 0x00020000,
    TRACK_LOG_UPDATA_LEFT          = 0X00100000,
    TRACK_LOG_UPDATA_RIGHT         = 0X00200000,
    TRACK_LOG_ALGORITHM_FAILURE    = 0x00400000,
};

typedef struct
{
    unsigned short  uhwMsgId;
    unsigned short  uhwReserve;
    unsigned int    uwHookType;
    unsigned int    uwOutputMode;
    unsigned int    uwPortNum;
    unsigned int    uwForceOutPutLog;
    unsigned int    uwHookPoints;
    unsigned int    uwLogMode;
    unsigned int    uwLogLevel;
} OM_CTRL_BLOCK_STRU;

typedef struct
{
    unsigned short                           uhwMsgId;
    unsigned short                           uhwReserve;
    char                                     chwname[CODECDSP_FW_NAME_MAX_LENGTH];
} FW_DOWNLOAD_STRU;

typedef struct
{
    unsigned short  uhwMsgId;
    unsigned short  uhwReserve;
} MAD_TEST_STRU;

typedef struct
{
    unsigned short  uhwMsgId;
    unsigned short  uhwReserve;
} ANC_TEST_STRU;

typedef struct
{
    unsigned short  uhwMsgId;
    unsigned short  uhwReserve;
    unsigned int    uwUartMode; /*0--115200, 1--500K, 2--6M*/
} UARTMODE_STRU;

struct fault_inject {
    unsigned short  uhwMsgId;
    unsigned short  uhwReserve;
    unsigned int    fault_type;
};

struct sync_msg_no_params {
    unsigned short  msg_id;
    unsigned short  reserve;
};

typedef unsigned short  APDSP_MSG_ENUM_UINT16;
#define AUDIO_PLAY_DEBUG_OPEN_RESERVE (0x0000)

enum HOOK_TYPE_ENUM
{
    HOOK_OFF = 0,
    HOOK_LOG = 1,
    HOOK_LOG_PCM = 2,
};

enum HOOK_FORCE_OUTPUT_LOG_ENUM
{
    HOOK_DEFUALT = 0,
    HOOK_FORCE_OUTPUT_LOG = 1,
};

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of hi64xx_hifi_interface.h */
