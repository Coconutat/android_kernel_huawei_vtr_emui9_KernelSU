#ifndef __HIFIALGOINTERFACE_H__
#define __HIFIALGOINTERFACE_H__

/************************************************ SOUNDTRIGGER COMMON BEGIN ************************************************/
struct MlibParameterST
{
    int key;
    union
    {
        int value;
        struct
        {
            unsigned int index;
            unsigned int length;
            char piece[0];
        };
    };
};

enum ST_PARA_ENUM
{
    MLIB_ST_PARA_ENABLE = 0,
    MLIB_ST_PARA_MODEL, // 1
    MLIB_ST_PARA_MODEL_SIZE,    // 2
    MLIB_ST_PARA_THRESHOLD, // 3
    MLIB_ST_PARA_PRINT_MCPS,    // 4
    MLIB_ST_PARA_MODEL_GRAM,    // 5
    MLIB_ST_PARA_MODEL_GRAM_SIZE,   // 6
    MLIB_ST_PARA_NO_SUSPEND,        // 7
    MLIB_ST_PARA_MODEL_CMD,     // 8
    MLIB_ST_PARA_MODEL_CMD_SIZE,    // 9
    MLIB_ST_PARA_MODEL_CMD_GRAM,    // 10
    MLIB_ST_PARA_MODEL_CMD_GRAM_SIZE,   // 11
    MLIB_ST_PARA_LANGUAGE,   // 12
    MLIB_ST_PARA_TRIGGER_TYPE,  // 13
    MLIB_ST_PARA_MODEL_UNLOAD, // 14
    MLIB_ST_PARA_MODEL_HANDLE, // 15
    MLIB_ST_PARA_MODEL_SCENE, //16
    MLIB_ST_PARA_MODEL_LOAD, //17
    MLIB_ST_PARA_MODEL_SENSIBILITY,

    MLIB_ST_PARA_TRANSACTION = 1000,
    MLIB_ST_PARA_DEBUG,
    MLIB_ST_PARA_CUSTOMCMD,
};

enum ST_LANGUAGE_ENUM
{
    ST_LANGUAGE_CN,
    ST_LANGUAGE_EN,
    ST_LANGUAGE_MAX,
};

enum ST_TRIGGER_TYPE_ENUM
{
    ST_TRIGGER_TYPE_FT,
    ST_TRIGGER_TYPE_FTSV,
    ST_TRIGGER_TYPE_UDT,
    ST_TRIGGER_TYPE_MAX,
};

enum ST_EVENT_ENUM
{
    ST_EVENT_TRIGGER_EMY = 0,
    ST_EVENT_COMMAND_FINDPHONE,
    ST_EVENT_TRIGGER_OKGOOGLE,

    ST_EVENT_MAX = 5,
};


/************************************************ SOUNDTRIGGER COMMON END ************************************************/

/************************************************ DSM PARA START************************************************/
struct MlibParameterDsm
{
    short extra_outputinfo;
    short key;
    union
    {
        short value;
        struct
        {
            unsigned short paraSize;
            char paraData[0];
        };
    };
};

enum DSM_PARA_ENUM
{
    MLIB_DSM_PARA_CONFIG,
    MLIB_DSM_PARA_DEBUG,
    MLIB_DSM_PARA_PRINT_MCPS,
    MLIB_DSM_PARA_DATA_HOOK,
};
enum EXTRA_OUTPUTINFO
{
    OUTPUT_MIXER_LEFT,
    OUTPUT_MIXER_RIGHT,
    OUTPUT_MIXER_INDEPENDNCE,
    OUTPUT_STEREO,
    OUTPUT_STEREO_SWITCH,
    OUTPUT_SINGLE_LEFT_BEG,
    OUTPUT_SINGLE_LEFT_END,
    OUTPUT_PLAY_STOP_FADEOUT,
    OUTPUT_MIXER_LEFT_NO_FADEINOUT,
    OUTPUT_SCENE_CHANGE_FADEOUT,
    OUTPUT_NONE,
};
/************************************************ DSM  PARA END************************************************/
struct fade_param
{
    unsigned short fadeout_time;// multiple of 5ms, can be 0
    unsigned short valley_time;// multiple of 5ms, can be 0
    unsigned short fadein_time;// multiple of 5ms, can be 0
    unsigned short fade_curve_type[2];// select fade curve type, default=0, [0] for fadeout, [1] for fadein
};

struct MlibParameterTfadsp
{
    short key;
    union
    {
        short value;
        struct
        {
            short algo_enable;
            short output_flag;
            short reserved[5];
            char param[0];
        };
        struct fade_param tfa_fade_param;
    };
};
enum TFADSP_KEY
{
    TFADSP_KEY_CONFIG,
    TFADSP_SATFETY_STRATEGY_CONFIG,
    TFADSP_CALIBRATION_CONFIG,
    TFADSP_ENABLE,
    TFADSP_PRINT_MCPS,
    TFADSP_DEBUG,
    TFADSP_FADE_CONFIG,
};
enum TFADSP_OUTPUT_FLAG
{
    TFADSP_OUTPUT_FLAG_MONO,
    TFADSP_OUTPUT_FLAG_STEREO_LANDSCAPE,
    TFADSP_OUTPUT_FLAG_STEREO_PORTRAIT,
};
#endif /* end of hi64xx_algo_interface.h */
