#ifndef __LCDKIT_PANEL_H_
#define __LCDKIT_PANEL_H_

#include "linux/kernel.h"
#include <linux/console.h>
#include <linux/uaccess.h>
#include <linux/leds.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/fb.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/raid/pq.h>

#include <linux/time.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/version.h>
#include <linux/backlight.h>
#include <linux/pwm.h>
#include <linux/pm_runtime.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/memblock.h>

#include <linux/spi/spi.h>

#include <linux/ion.h>
//#include <linux/hisi/hisi_ion.h>
#include <linux/gpio.h>

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>

#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/pinctrl/consumer.h>
#include <linux/file.h>
#include <linux/dma-buf.h>
#include <linux/genalloc.h>

#define strict_strtoul        kstrtoul

/* dcs read/write */
#define DTYPE_DCS_WRITE     0x05/* short write, 0 parameter */
#define DTYPE_DCS_WRITE1    0x15/* short write, 1 parameter */
#define DTYPE_DCS_READ      0x06/* read */
#define DTYPE_DCS_LWRITE    0x39/* long write */

/* generic read/write */
#define DTYPE_GEN_WRITE     0x03/* short write, 0 parameter */
#define DTYPE_GEN_WRITE1    0x13    /* short write, 1 parameter */
#define DTYPE_GEN_WRITE2    0x23/* short write, 2 parameter */
#define DTYPE_GEN_LWRITE    0x29/* long write */
#define DTYPE_GEN_READ      0x04/* long read, 0 parameter */
#define DTYPE_GEN_READ1     0x14/* long read, 1 parameter */
#define DTYPE_GEN_READ2     0x24/* long read, 2 parameter */

#define MAX_REG_READ_COUNT 32
#define LCDKIT_DTYPE_MAX_PKTSIZE    0x37/* set max packet size */
#define BIS_CHECK_COUNT 512
#define LCDKIT_CHECKSUM_SIZE   (8)
#define LCDKIT_CHECKSUM_PIC_NUM  (10)
#define LCDKIT_CMD_NAME_MAX 100
#define SENCE_ARRAY_SIZE 100

#define CHECK_REG_MAX 5
#define BITS(x)  (1<<(x))

/*panel type
 *0x01:LCD
 *0x02:OLED
 *
*/
#define  PANEL_TYPE_LCD  BITS(0)
#define  PANEL_TYPE_OLED BITS(1)

/* GPIO label defination  */
#define GPIO_LCDKIT_PCD_NAME	                  "gpio_lcdkit_pcd"
#define GPIO_LCDKIT_ERRFLAG_NAME                  "gpio_lcdkit_errflag"

/*
*power ctrl mode
*0x01: regulator
*0x02: gpio
*/
#define  POWER_CTRL_BY_REGULATOR  BITS(0)
#define  POWER_CTRL_BY_GPIO BITS(1)
#define  POWER_CTRL_BY_IC BITS(2)
/* lcd fps scence */
#define LCDKIT_FPS_SCENCE_NORMAL    (0)
#define LCDKIT_FPS_SCENCE_IDLE          BIT(0)
#define LCDKIT_FPS_SCENCE_VIDEO       BIT(1)
#define LCDKIT_FPS_SCENCE_GAME        BIT(2)
#define LCDKIT_FPS_SCENCE_WEB          BIT(3)
#define LCDKIT_FPS_SCENCE_EBOOK      BIT(4)
#define LCDKIT_FPS_SCENCE_FORCE_30FPS    BIT(5)
#define LCDKIT_FPS_SCENCE_FUNC_DEFAULT_ENABLE    BIT(6)
#define LCDKIT_FPS_SCENCE_FUNC_DEFAULT_DISABLE    BIT(7)
/* lcd fps value */
#define LCDKIT_FPS_30 (30)
#define LCDKIT_FPS_55 (55)
#define LCDKIT_FPS_60 (60)

#define LCDKIT_TS_RESUME_AFTER_DIS_ON   BIT(0)
#define LCDKIT_TS_RESUME_BEFORE_LCD_RST  BIT(1)

#define LCDKIT_MAX_PANEL_NAME_LEN (128)
#define MAX_BUF 60
#define LCD_REG_LENGTH_MAX 200
#define LCD_REG_NUM 50

#define	TEST_PIC_0	0
#define	TEST_PIC_1	1
#define	TEST_PIC_2	2

#define	CHECKSUM_CHECKCOUNT	5

#define LCDKIT_BL_NORMAL_MODE 1
#define LCDKIT_BL_ENHANCE_MODE 2

#define LCDKIT_COLOR_INFO_SIZE 8
#define LCDKIT_SERIAL_INFO_SIZE 16

#define GPIO_LOW_PCDERRFLAG    0
#define GPIO_HIGH_PCDERRFLAG   1

#define PCD_ERR_FLAG_SUCCESS   0
#define PCD_FAIL               1
#define ERRFLAG_FAIL           2
#define PCD_ERRFLAG_FAIL       3

#define ACL_MAX_SIZE           10
#define ACL_SET_MODE           1

enum
{
    LCDKIT_CHECKSUM_START = 0,
    LCDKIT_CHECKSUM_END = 1,
};

enum
{
    GET_BLMAXNIT_FROM_TP = 0,
    GET_BLMAXNIT_FROM_DDIC = 1,
};

typedef void (* dcs_level_trans_func_t)(struct lcdkit_dsi_panel_cmds *, int, int);
#define REG51_MODE_NUM 2
#define REG90_MODE_NUM 1
#define SET_CABC_PWM_CMD_51 0x51
#define SET_CABC_PWM_CMD_90 0x90
#define BL_LEVEL_MAX_8_BIT 256
#define BL_LEVEL_MAX_10_BIT 1024
#define BL_LEVEL_MAX_11_BIT 2048


/*parse dirtyregion info node*/
#define OF_PROPERTY_READ_DIRTYREGION_INFO_RETURN(np, propname, ptr_out_value) \
    do { \
        if (of_property_read_u32(np, propname, ptr_out_value)) { \
			LCDKIT_DEBUG("of_property_read_u32: %s, fail\n", propname); \
			*ptr_out_value = -1; \
        } \
        if( 0xffff == *ptr_out_value ) { \
            *ptr_out_value = -1; \
        } \
    } while (0)

#define OF_PROPERTY_READ_U64_RETURN(np, propname, ptr_out_value) \
    do { \
        u32 temp = 0; \
        if( of_property_read_u32(np, propname, &temp) ) { \
            LCDKIT_DEBUG("of_property_read: %s, fail\n", propname); \
            temp = 0; \
        } \
        *ptr_out_value = (u64)temp; \
    } while (0)

/*parse dts node*/
#define OF_PROPERTY_READ_U32_RETURN(np, propname, ptr_out_value) \
    do { \
        if( of_property_read_u32(np, propname, ptr_out_value) ) { \
            LCDKIT_DEBUG("of_property_read_u32: %s, fail\n", propname); \
            *ptr_out_value = 0; \
        } \
    } while (0)

#define OF_PROPERTY_READ_S8_RETURN(np, propname, ptr_out_value) \
    do { \
        if( of_property_read_u32(np, propname, ptr_out_value) ) { \
            LCDKIT_DEBUG("of_property_read_u32: %s, fail\n", propname); \
            *ptr_out_value = 0; \
        } \
        if((*ptr_out_value & 0xff00) == 0xff00){ \
            *ptr_out_value = 0-(*ptr_out_value & 0xff); \
        } \
    } while (0)


/*parse dts node*/
#define OF_PROPERTY_READ_U8_RETURN(np, propname, ptr_out_value) \
    do { \
        int temp = 0; \
        if( of_property_read_u32(np, propname, &temp) ) { \
            LCDKIT_DEBUG("of_property_read: %s, fail\n", propname); \
            temp = 0; \
        } \
        *ptr_out_value = (char)temp; \
    } while (0)

/*parse dts node*/
#define OF_PROPERTY_READ_U32_DEFAULT(np, propname, ptr_out_value, default) \
    do { \
        if( of_property_read_u32(np, propname, ptr_out_value) ) { \
            LCDKIT_DEBUG("of_property_read_u32: %s, fail, use default: %d\n", propname, default); \
            *ptr_out_value = default;  \
        } \
    } while (0)

/*parse dts node*/
#define OF_PROPERTY_READ_U8_DEFAULT(np, propname, ptr_out_value, default) \
    do { \
        int temp = 0; \
        if( of_property_read_u32(np, propname, &temp) ) { \
            LCDKIT_DEBUG("of_property_read: %s, fail, use default: %d\n", propname, default); \
            temp = default;  \
        } \
        *ptr_out_value = (char)temp; \
    } while (0)

static char* sence_array[SENCE_ARRAY_SIZE] = {
    "LCD_INCOME0",   "MMI0",   "RUNNINGTEST0", "PROJECT_MENU0",
    "LCD_INCOME1",   "MMI1",   "RUNNINGTEST1",  "PROJECT_MENU1",
    "LCD_INCOME2",   "MMI2",   "RUNNINGTEST2",  "PROJECT_MENU2",
    "LCD_INCOME3",   "MMI3",   "RUNNINGTEST3",  "PROJECT_MENU3",
    "LCD_INCOME4",   "MMI4",   "RUNNINGTEST4",  "PROJECT_MENU4",
    "LCD_INCOME5",   "MMI5",   "RUNNINGTEST5",  "PROJECT_MENU5",
    "LCD_INCOME6",   "MMI6",   "RUNNINGTEST6",  "PROJECT_MENU6",
    "LCD_INCOME7",   "MMI7",   "RUNNINGTEST7",  "PROJECT_MENU7",
    "LCD_INCOME8",   "MMI8",   "RUNNINGTEST8",  "PROJECT_MENU8",
    "LCD_INCOME9",   "MMI9",   "RUNNINGTEST9",  "PROJECT_MENU9",
    "LCD_INCOME10",  "MMI10",  "RUNNINGTEST10",  "PROJECT_MENU10",
    "LCD_INCOME11",  "MMI11",  "RUNNINGTEST11",  "PROJECT_MENU11",
    "LCD_INCOME12",  "MMI12",  "RUNNINGTEST12",  "PROJECT_MENU12",
    "LCD_INCOME13",  "MMI13",  "RUNNINGTEST13",  "PROJECT_MENU13",
    "LCD_INCOME14",  "MMI14",  "RUNNINGTEST14",  "PROJECT_MENU14",
    "LCD_INCOME15",  "MMI15",  "RUNNINGTEST15",  "PROJECT_MENU15",
    "LCD_INCOME16",  "MMI16",  "RUNNINGTEST16",  "PROJECT_MENU16",
    "LCD_INCOME17",  "MMI17",  "RUNNINGTEST17",  "PROJECT_MENU17",
    "CURRENT1_0",    "CURRENT1_1", "CURRENT1_2",  "CURRENT1_3",
    "CURRENT1_4",    "CURRENT1_5", "CHECKSUM1",  "CHECKSUM2",
    "CHECKSUM3",     "CHECKSUM4", "BL_OPEN_SHORT",  "PCD_ERRORFLAG",
    "DOTINVERSION",  "CHECKREG", "COLUMNINVERSION",   "POWERONOFF",
    "BLSWITCH",};

static char* cmd_array[SENCE_ARRAY_SIZE] = {
    "CURRENT1_0",   "CURRENT1_0",  "CURRENT1_0",  "CURRENT1_0",//current test0
    "CURRENT1_1",   "CURRENT1_1",  "CURRENT1_1",  "CURRENT1_1",//current test1
    "CURRENT1_2",   "CURRENT1_2",  "CURRENT1_2",  "CURRENT1_2",//current test2
    "CURRENT1_3",   "CURRENT1_3",  "CURRENT1_3",  "CURRENT1_3",//current test3
    "CURRENT1_4",   "CURRENT1_4",  "CURRENT1_4",  "CURRENT1_4",//current test4
    "CURRENT1_5",   "CURRENT1_5",  "CURRENT1_5",  "CURRENT1_5",//current test5
    "CHECKSUM1",   "CHECKSUM1",   "CHECKSUM1", "CHECKSUM1",//checksum1
    "CHECKSUM2",   "CHECKSUM2",   "CHECKSUM2", "CHECKSUM2",//checksum2
    "CHECKSUM3",    "CHECKSUM3",   "CHECKSUM3", "CHECKSUM3",//checksum3
    "CHECKSUM4",   "CHECKSUM4",   "CHECKSUM4", "CHECKSUM4",//checksum4
    "BL_OPEN_SHORT",   "BL_OPEN_SHORT",   "BL_OPEN_SHORT", "BL_OPEN_SHORT",//backlight open short test
    "PCD_ERRORFLAG",   "PCD_ERRORFLAG",  "PCD_ERRORFLAG", "PCD_ERRORFLAG", // PCD and errorflag test
    "DOTINVERSION",    "DOTINVERSION",  "DOTINVERSION", "DOTINVERSION",// dot inversion test
    "CHECKREG",    "CHECKREG",  "CHECKREG", "CHECKREG",// check ic status reg
    "COLUMNINVERSION", "COLUMNINVERSION", "COLUMNINVERSION", "COLUMNINVERSION", //column inversion test
    "POWERONOFF",   "POWERONOFF",  "POWERONOFF",  "POWERONOFF",// power on/off test
    "BLSWITCH",    "BLSWITCH",  "BLSWITCH", "BLSWITCH",// backlight switch test
    "GPU_TEST",   "GPU_TEST",  "GPU_TEST", "GPU_TEST", //GPU SLT test
    "/sys/class/ina231/ina231_0/ina231_set," \
    "/sys/class/ina231/ina231_0/ina231_value," \
    "1,9999999,1,9999999,1,99999",
    "/sys/class/ina231/ina231_0/ina231_set," \
    "/sys/class/ina231/ina231_0/ina231_value," \
    "1,9999999,1,9999999,1,99999",
    "/sys/class/ina231/ina231_0/ina231_set," \
    "/sys/class/ina231/ina231_0/ina231_value," \
    "1,9999999,1,9999999,1,99999",
    "/sys/class/ina231/ina231_0/ina231_set," \
    "/sys/class/ina231/ina231_0/ina231_value," \
    "1,9999999,1,9999999,1,99999",
    "/sys/class/ina231/ina231_0/ina231_set," \
    "/sys/class/ina231/ina231_0/ina231_value," \
    "1,9999999,1,9999999,1,99999",
    "/sys/class/ina231/ina231_0/ina231_set," \
    "/sys/class/ina231/ina231_0/ina231_value," \
    "1,9999999,1,9999999,1,99999",
    "/sys/class/graphics/fb0/lcd_checksum",
    "/sys/class/graphics/fb0/lcd_checksum",
    "/sys/class/graphics/fb0/lcd_checksum",
    "/sys/class/graphics/fb0/lcd_checksum",
    "/sys/class/lm36923/lm36923/self_test",
    "/sys/class/graphics/fb0/amoled_pcd_errflag_check",
    "/sys/class/graphics/fb0/lcd_inversion_mode",
    "/sys/class/graphics/fb0/lcd_check_reg",
    "/sys/class/graphics/fb0/lcd_inversion_mode",
    "/sys/class/graphics/fb0/lcd_check_reg",
    "/sys/class/graphics/fb0/lcd_check_reg",};

/* gpio desc */
struct lcdkit_gpio_desc
{
    int dtype;
    int waittype;
    int wait;
    char* label;
    int* gpio;
    int value;
};

/* vcc desc */
struct lcdkit_regulator_desc
{
    int dtype;
    char* id;
    struct regulator** regulator;
    int min_uV;
    int max_uV;
    int waittype;
    int wait;
};

/***********************************************************
*struct definition
***********************************************************/
//dsi_cmd_desc
struct lcdkit_dsi_cmd_desc
{
    uint32_t dtype;/* data type */
    uint32_t last;/* last in chain */
    uint32_t vc;/* virtual chan */
    uint32_t ack;/* ask ACK from peripheral */
    uint32_t wait;/* ms */
    uint32_t waittype;
    uint32_t dlen;/* 8 bits */
    char* payload;
} __packed;

struct lcdkit_dsi_cmd_desc_header
{
    uint32_t dtype;/* data type */
    uint32_t last;/* last in chain */
    uint32_t vc;/* virtual chan */
    uint32_t ack;/* ask ACK from peripheral */
    uint32_t wait;/* ms */
    uint32_t waittype;
    uint32_t dlen;/* 8 bits */
};


//dsi_panel_cmds
/*dsi cmd struct*/
struct lcdkit_dsi_panel_cmds
{
    char* buf;
    int blen;
    struct lcdkit_dsi_cmd_desc* cmds;
    int cmd_cnt;
    int link_state;
    u32 flags;
};

struct lcdkit_dsi_read_compare_data
{
    uint32_t* read_value;
    uint32_t* expected_value;
    uint32_t* read_mask;
    char** reg_name;
    int log_on;
    struct lcdkit_dsi_cmd_desc* cmds;
    int cnt;
};

struct lcdkit_array_data
{
    char* buf;
    int cnt;
};

struct lcdkit_int_array_data
{
    uint32_t* buf;
    int cnt;
};

struct lcdkit_arrays_data
{
    struct lcdkit_array_data* arry_data;
    int cnt;
};

struct lcdkit_array_ctrl
{
    struct lcdkit_dsi_panel_cmds cmds;
    struct lcdkit_array_data res_buf;
    bool support;
};

struct dirty_rect
{
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
};

struct whitepoint
{
    uint32_t white_coordinates[2];
    uint32_t white_luminance;
};

struct panelid
{
    uint32_t modulesn;
    uint32_t equipid;
    uint32_t modulemanufactdate;
    uint32_t vendorid;
};

struct coloruniformparams
{
    uint32_t c_lmt[3];
    uint32_t mxcc_matrix[3][3];
    uint32_t white_decay_luminace;
};

struct colormeasuredata
{
    uint32_t chroma_coordinates[4][2];
    uint32_t white_luminance;
};

struct lcdbrightnesscoloroeminfo
{
    uint32_t id_flag;
    uint32_t tc_flag;
    struct panelid  panel_id;
    struct coloruniformparams color_params;
    struct colormeasuredata color_mdata;
};

struct porch_param
{
    uint32_t    h_back_porch;
    uint32_t    h_front_porch;
    uint32_t    h_pulse_width;
    uint32_t    v_back_porch;
    uint32_t    v_front_porch;
    uint32_t    v_pulse_width;
};

enum
{
    LCDKIT_WAIT_TYPE_US = 0,
    LCDKIT_WAIT_TYPE_MS,
};

enum
{
    CABC_OFF = 0,
    CABC_UI_MODE = 1,
    CABC_STILL_MODE = 2,
    CABC_MOVING_MODE = 3,
    CABC_GAMING_MODE = 4,
};

enum
{
    RGBW_SET1_MODE = 1,
    RGBW_SET2_MODE = 2,
    RGBW_SET3_MODE = 3,
    RGBW_SET4_MODE = 4,
};

enum
{
	RGBW_PANEL_ID_MIN = 0,
	JDI_NT36860C_PANEL_ID = 1,
	LG_NT36870_PANEL_ID = 2,
	SHARP_NT36870_PANEL_ID = 3,
	JDI_HX83112C_PANLE_ID = 4,
	SHARP_HX83112C_PANEL_ID = 5,
	JDI_TD4336_PANEL_ID = 6,
	SHARP_TD4336_PANEL_ID = 7,
	LG_NT36772A_PANEL_ID = 8,
	BOE_HX83112E_PANEL_ID = 9,
	JDI_TD4336_HMA_PANEL_ID = 10,
	SHARP_TD4336_HMA_PANEL_ID = 11,
	LG_NT36772A_HMA_PANEL_ID = 12,
	BOE_HX83112E_HMA_PANEL_ID = 13,
	RGBW_PANEL_ID_MAX,
};

enum
{
    COLUMN_INVERSION = 0,
    DOT_INVERSION,
};

enum
{
    FORWORD_SCAN = 0,
    REVERT_SCAN,
};

enum
{
    HBM_OFF = 0,
    HBM_ON_MAX = 1,
    HBM_ON_MEDIUM = 2,
};

enum
{
    ACL_OFF = 0,
    ACL_ON_SET1 = 1,
    ACL_ON_SET2 = 2,
    ACL_ON_SET3 = 3,
};

enum
{
    LP2HS_MIPI_TEST_OFF = 0,
    LP2HS_MIPI_TEST_ON = 1,
};

enum
{
    VR_MODE_DISABLE = 0,
    VR_MODE_ENABLE = 1,
};

enum
{
    CE_OFF = 0,
    CE_SRGB_MODE = 1,
    CE_USER_MODE = 2,
    CE_VIVID_MODE = 3,
};

enum
{
    SE_OFF = 0,
    SE_HD_MODE = 1,
    SE_FHD_MODE = 2,
};

enum {
    BL_MODULE_LP8556 = 8556,
    BL_MODULE_LM36923 = 36923,
};

enum esd_judge_type {
    ESD_UNEQUAL,
    ESD_EQUAL,
    ESD_BIT_VALID,
};

struct lcdkit_panel_infos
{
    /*panel information*/
    char* lcd_compatible;
    /*panel name*/
    char* panel_name;
    char* panel_model;
    char* panel_default_project_id;
    u8 panel_type; //lcd or oled
    u8 lcd_disp_type; //video or cmd
    u8 iovcc_power_ctrl_mode;
    u8 vdd_power_ctrl_mode;
    u8 vci_power_ctrl_mode;
    u8 vbat_power_ctrl_mode;
    u8 bias_power_ctrl_mode;
    u8 lcd_backlight_power_ctrl_mode;
    u8 lcd_bklt_level_to_regvalue_mode;
    u8 lcd_bklt_dcs_reg;
    
    u8 lcd_bl_gpio_ctrl_mode;
    u8 lcd_suspend_bl_disable;
    u8 bl_type;
    u32 effect_support_mode;

    /*if need dsi1 send cmd to panel*/
    u8 dsi1_snd_cmd_panel_support;

    u32 lcd_id;
    u32 product_id;

    int xres;  //framebuffer resolution of horizental, before arsr1p
    int yres;  //framebuffer resolution of vertical, before arsr1p
    int lcd_xres; //lcd resolution of horizental, after arsr1p
    int lcd_yres;  //lcd resolution of vertical, after arsr1p
    int width; //mm
    int height; //mm

    int h_back_porch;
    int h_front_porch;
    int h_pulse_width;
    int v_back_porch;
    int v_front_porch;
    int v_pulse_width;

    /*backlight sem*/
    struct semaphore bl_sem;
    struct semaphore lcdkit_cmd_sem;

    int idle2_lcd_reset_low;
    int idle2_lcd_on_reset_low_to_high;

    int bl_level_max;
    int bl_level_min;
    int bl_level_def;
    int bl_is_shield_backlight;
    int bl_is_start_second_timer;

    /* bl work mode */
    u32 bl_support_mode;
    u32 bl_work_mode;
    u32 bl_enhance;
    u32 bl_normal;

    /*display parameter config*/
    bool mipi_clk_config_support;
    int mipi_clk_config_high;
    int mipi_clk_config_low;
    int mipi_clk_config_normal;

    /*lcd on command*/
    struct lcdkit_dsi_panel_cmds display_on_cmds;
     struct lcdkit_dsi_panel_cmds display_on_second_cmds;
    /*for lcd initcode debug*/
    struct lcdkit_dsi_panel_cmds g_panel_cmds;
    /*display on in backlight*/
    struct lcdkit_dsi_panel_cmds display_on_in_backlight_cmds;

    /*lcd off command*/
    struct lcdkit_dsi_panel_cmds display_off_cmds;
    struct lcdkit_dsi_panel_cmds display_off_second_cmds;
    /*backlight command*/
    struct lcdkit_dsi_panel_cmds backlight_cmds;

    /*cabc off command*/
    struct lcdkit_dsi_panel_cmds cabc_off_cmds;
    /*cabc ui command*/
    struct lcdkit_dsi_panel_cmds cabc_ui_cmds;
    /*cabc still command*/
    struct lcdkit_dsi_panel_cmds cabc_still_cmds;
    /*cabc moving command*/
    struct lcdkit_dsi_panel_cmds cabc_moving_cmds;
    /*cabc gaming command*/
    struct lcdkit_dsi_panel_cmds cabc_gaming_cmds;
    //rgbw setting commands
    /*rgbw lowper1 command*/
    struct lcdkit_dsi_panel_cmds rgbw_set1_cmds;
    /*rgbw lowper2 command*/
    struct lcdkit_dsi_panel_cmds rgbw_set2_cmds;
    /*rgbw lowper3 command*/
    struct lcdkit_dsi_panel_cmds rgbw_set3_cmds;
    /*rgbw outdoor command*/
    struct lcdkit_dsi_panel_cmds rgbw_set4_cmds;
    /*rgbw backlight command*/
    struct lcdkit_dsi_panel_cmds rgbw_backlight_cmds;
    /*rgbw saturation_control command*/
    struct lcdkit_dsi_panel_cmds rgbw_saturation_control_cmds;
    /*rgbw frame_gain_limit command*/
    struct lcdkit_dsi_panel_cmds frame_gain_limit_cmds;
    /*rgbw frame_gain_speed command*/
    struct lcdkit_dsi_panel_cmds frame_gain_speed_cmds;
    /*rgbw color_distortion_allowance command*/
    struct lcdkit_dsi_panel_cmds color_distortion_allowance_cmds;
    /*rgbw pixelgainlimit command*/
    struct lcdkit_dsi_panel_cmds pixel_gain_limit_cmds;
    /*rgbw pixel_gain_speed command*/
    struct lcdkit_dsi_panel_cmds pixel_gain_speed_cmds;
    /*rgbw pwm_duty_gain command*/
    struct lcdkit_dsi_panel_cmds pwm_duty_gain_cmds;
    /* enter hbm mode and set brightness level command*/
    struct lcdkit_dsi_panel_cmds enter_hbm_cmds;
    /* set hbm brightness level command*/
    struct lcdkit_dsi_panel_cmds hbm_level_cmds;
    /* set hbm mode command*/
    struct lcdkit_dsi_panel_cmds hbm_mode_cmds;
    /* Exit hbm mode command*/
    struct lcdkit_dsi_panel_cmds exit_hbm_cmds;

    u8 cabc_support;
    u8 cabc_switch_support;
    /*cabc mode*/
    int cabc_mode;
    u8 rgbw_support;
    u8 bias_change_lm36274_from_panel_support;
    u8 init_lm36923_after_panel_power_on_support;
    u8 hbm_support;

    /*mipi dsi upt command */
    u8 dsi_upt_snd_cmd_support;
    struct lcdkit_dsi_panel_cmds dsi_upt_snd_a_cmds;
    struct lcdkit_dsi_panel_cmds dsi_upt_snd_b_cmds;
    struct lcdkit_dsi_panel_cmds dsi_upt_snd_c_cmds;
    struct lcdkit_dsi_panel_cmds dsi_upt_snd_d_cmds;
    struct lcdkit_dsi_panel_cmds dsi_upt_snd_e_cmds;
    /*vesa setting flag*/
    u8 ifbc_vesa3x_set;
    /*mipi tr inversion*/
    struct lcdkit_dsi_panel_cmds dot_inversion_cmds;
    struct lcdkit_dsi_panel_cmds column_inversion_cmds;
    u8 inversion_support;
    /*inversion mode*/
    int inversion_mode;

    /*lcd forword/revert scan test*/
    struct lcdkit_dsi_panel_cmds forword_scan_cmds;
    struct lcdkit_dsi_panel_cmds revert_scan_cmds;
    u8 scan_support;
    u8 tp_lcd_reset_sync;
    /*scan mode*/
    int scan_mode;

    /*esd check commond*/
    struct lcdkit_dsi_panel_cmds esd_cmds;
    struct lcdkit_int_array_data esd_value;
    struct lcdkit_dsi_panel_cmds esd_dbg_cmds;
    struct lcdkit_dsi_panel_cmds esd_bl_ctrl_cmds;
    struct lcdkit_dsi_panel_cmds esd_curic_cmds;
    struct lcdkit_dsi_panel_cmds esd_oriic_cmds;
    u8 esd_support;
    u8 esd_check_num;
    u8 esd_expect_value_type;
    u8 use_second_ic;

    /*running test check reg*/
    struct lcdkit_dsi_panel_cmds check_reg_cmds;
    struct lcdkit_array_data check_reg_value;
    u8 check_reg_support;

    /*mipi check commond*/
    struct lcdkit_dsi_panel_cmds mipi_check_cmds;
    struct lcdkit_array_data mipi_check_value;
    u8 mipi_check_support;
    u8 mipi_error_report_threshold;
    struct timeval display_on_record_time;

    /*display region*/
    struct lcdkit_dsi_panel_cmds display_region_cmds;
    u8 display_region_support;

    /*checksum*/
    struct lcdkit_dsi_panel_cmds checksum_enter_cmds;
    struct lcdkit_dsi_panel_cmds checksum_cmds;
    struct lcdkit_dsi_panel_cmds checksum_exit_cmds;
    struct lcdkit_dsi_panel_cmds checksum_enable_cmds;
    struct lcdkit_dsi_panel_cmds checksum_disable_cmds;
    struct lcdkit_dsi_panel_cmds checksum_enter_second_part_cmds;
    struct lcdkit_arrays_data checksum_value;
    u8 checksum_pic_num;
    u8 checksum_second_part_support;
    u8 checksum_support;
    u8 checksum_read_again;

    /*dynamic sram check*/
    struct lcdkit_dsi_panel_cmds dynamic_sram_check_enter_cmds;
    struct lcdkit_dsi_panel_cmds dynamic_sram_check_cmds;
    struct lcdkit_dsi_panel_cmds dynamic_sram_check_exit_cmds;
    struct lcdkit_dsi_panel_cmds dynamic_sram_check_enable_cmds;
    struct lcdkit_dsi_panel_cmds dynamic_sram_check_disable_cmds;
    struct lcdkit_arrays_data dynamic_sram_check_value;
    u8 dynamic_sram_check_pic_num;
    u8 dynamic_sram_check_support;

    /*mipi detect*/
    struct lcdkit_dsi_panel_cmds mipi_detect_cmds;
    struct lcdkit_array_data mipi_detect_value;
    struct lcdkit_array_data mipi_detect_mask;
    u8 mipi_detect_support;

    /*fps ctrl*/
    struct lcdkit_dsi_panel_cmds fps_to_30_cmds;
    struct lcdkit_dsi_panel_cmds fps_to_55_cmds;
    struct lcdkit_dsi_panel_cmds fps_to_60_cmds;
    struct lcdkit_dsi_panel_cmds dfr_enable_cmds;
    struct lcdkit_dsi_panel_cmds dfr_disable_cmds;
    struct lcdkit_array_data fps_30_porch_param;
    struct lcdkit_array_data fps_55_porch_param;
    struct lcdkit_array_data fps_60_porch_param;
    u8 fps_func_switch;
    u8 fps_tscall_support;
    u8 snd_cmd_before_frame_support;
    struct timer_list fps_scence_timer;
    struct workqueue_struct *fps_scence_wq;
    struct work_struct fps_scence_work;
    struct lcdkit_dsi_panel_cmds snd_cmd_before_frame_cmds;
    u8 panel_chip_esd_disable;

    u8 ts_resume_ctrl_mode;

    /*lp2hs mipi test*/
    struct lcdkit_dsi_panel_cmds lp2hs_mipi_check_write_cmds;
    struct lcdkit_dsi_panel_cmds lp2hs_mipi_check_read_cmds;
    int g_lp2hs_mipi_check_result;
    int lp2hs_mipi_check_expected_value;
    int lp2hs_mipi_check_read_mask;
    u8 lp2hs_mipi_check_support;

    /*current detect*/
    struct lcdkit_dsi_panel_cmds current_detect_enter_cmds;
    struct lcdkit_dsi_panel_cmds current_detect_cmds;
    struct lcdkit_dsi_panel_cmds current_detect_exit_cmds;
    struct lcdkit_array_data current_mask;
    u8 current_detect_support;

    /*lv detect*/
    struct lcdkit_dsi_panel_cmds lv_detect_enter_cmds;
    struct lcdkit_dsi_panel_cmds lv_detect_cmds;
    struct lcdkit_dsi_panel_cmds lv_detect_exit_cmds;
    u8 lv_detect_support;
    u8 lcd_self_testing;

    /*test config*/
    char lcd_cmd_now[LCDKIT_CMD_NAME_MAX];

    /*PT current test*/
    int enable_PT_test;
    bool PT_test_support;
    u8 panel_ulps_support;

    /*ACL ctrl*/
    struct lcdkit_dsi_panel_cmds acl_enable_cmds;
    int acl_ctrl_mode;
    u8 acl_ctrl_support;
    /*ACL change brightness level cmd*/
    struct lcdkit_dsi_panel_cmds acl_off_cmds;
    struct lcdkit_dsi_panel_cmds acl_low_cmds;
    struct lcdkit_dsi_panel_cmds acl_middle_cmds;
    struct lcdkit_dsi_panel_cmds acl_high_cmds;

    /*HBM ctrl*/
    struct lcdkit_dsi_panel_cmds hbm_enable_cmds;
    int hbm_ctrl_mode;
    u8 hbm_ctrl_support;

    u32 panel_off_reset_high;
    u32 shutdown_sleep_support;
    struct lcdkit_dsi_panel_cmds shutdown_sleep_cmds;

    bool panel_down_reset;
    u32 frame_updated;
    struct delayed_work bkl_work;


    /*HKADC test*/
    int  hkadc_buf;
    u8 hkadc_support;

    /*vr mode ctrl*/
    int vr_mode;
    struct lcdkit_dsi_panel_cmds vr_mode_enable_cmds;
    struct lcdkit_dsi_panel_cmds vr_mode_disable_cmds;
    u8 vr_support;

    /*ic_color_enhancement*/
    u8 ic_color_enhancement_support;

    /*ce*/
    struct lcdkit_dsi_panel_cmds ce_off_cmds;
    struct lcdkit_dsi_panel_cmds ce_srgb_cmds;
    struct lcdkit_dsi_panel_cmds ce_user_cmds;
    struct lcdkit_dsi_panel_cmds ce_vivid_cmds;
    u8 ce_support;
    u8 ce_mode;
    u8 lcd_version_support;
    u8 lcd_otp_support;
    /*se*/
    u8 se_support;
    u8 se_mode;
    u8 pt_ulps_support;

    struct lcdkit_dsi_panel_cmds se_off_cmds;
    struct lcdkit_dsi_panel_cmds se_hd_cmds;
    struct lcdkit_dsi_panel_cmds se_fhd_cmds;
    /*bist check*/
    struct lcdkit_dsi_panel_cmds sram_test_pre_cmds;
    struct lcdkit_dsi_panel_cmds write_pattern_cmds;
    struct lcdkit_dsi_panel_cmds sram_test_post_cmds;
    u8 sram_test_feedback;
    u8 bist_check_support;

    /*pcd errflag check*/
    u8 pcd_errflag_check_support;

    /*support check mode*/
    char* support_check_mode;

	/*for fake panel*/
	u32 bl_enable_flag;

    /*max light*/
    u32 bl_max_nit;

    u32 get_blmaxnit_type;
    u32 get_blmaxnit_offset;
    struct lcdkit_dsi_panel_cmds bl_maxnit_cmds;
    struct lcdkit_dsi_panel_cmds bl_befreadconfig_cmds;
    struct lcdkit_dsi_panel_cmds bl_aftreadconfig_cmds;

    /*tp resume ctrl*/
    u8 tp_resume_no_sync;

    /*sleep out for backlight*/
    u8 display_on_in_backlight;
    /* display on new seq and reset time for synaptics only */
    u8 panel_display_on_new_seq;
    bool display_on_need_send;

    u8 dis_on_cmds_delay_margin_support;
    u32 dis_on_cmds_delay_margin_time;
    bool dis_on_cmds_panel_on_tag;
    struct timeval dis_on_cmds_record_tv;
    /*hiace*/
    char *hiace_chCfgName;

    /*display effect on cmds */
    struct lcdkit_dsi_panel_cmds display_effect_on_cmds;
    u8 display_effect_on_support;
    /***************/
   /*check reg in panel on*/
    u8 check_reg_on_support;
    struct lcdkit_array_data check_reg_on_value;
    struct lcdkit_array_data check_reg_expect_value;
    struct lcdkit_array_data check_reg_mask_value;
    /*for dynamic gamma*/
    u8 dynamic_gamma_support;
    u32 gama_cmd_page;
    u32 gama_reg_addr;
    u32 gama_reg_length;
    struct lcdkit_dsi_panel_cmds lcd_reg_check_enter_cmds;
    struct lcdkit_dsi_panel_cmds lcd_reg_check_exit_cmds;
    struct lcdkit_array_data gama_correct_reg;
    struct lcdkit_array_data gama_reg_len;

    u8 lcd_brightness_color_uniform_support;
    /*LcdanalogVcc*/
    u32 lcdanalog_vcc;
    /*LcdioVcc*/
    u32 lcdio_vcc;
    /*LcdBias*/
    u32 lcd_bias;
    /*LcdVsp*/
    u32 lcd_vsp;
    /*LcdVsn*/
    u32 lcd_vsn;
    /*LcdVdd*/
    u32 lcdvdd_vcc;

    /*reset hight*/
    u8 reset_step1_H;
    /*reset low*/
    u8 reset_L;
    /*reset hight*/
    u8 reset_step2_H;
    /*otm1906c ic is need reset after iovcc power on*/
    u8 first_reset;
    u8 second_reset;
    u8 reset_pull_high_flag;
    u8 tprst_before_lcdrst;
    u8 tpfw_early_lcdinit;
    u8 tp_before_lcdsleep;

    /*for tp_reset after lcd reset low*/
    u8 tp_after_lcd_reset;
    u8 tp_befor_vsn_low_delay;

    /*control reset shutdown timing*/
    u8 reset_shutdown_later;

    /***************
    **for gpio ctrl
    */
    /*ID0*/
    u32 gpio_lcd_id0;
    /*ID1*/
    u32 gpio_lcd_id1;

    /*VSP*/
    u32 gpio_lcd_iovcc;
    /*VSN*/
    u32 gpio_lcd_vci;
    /*VSP*/
    u32 gpio_lcd_vsp;
    /*VSN*/
    u32 gpio_lcd_vsn;
    /*BACKLIGHT*/
    u32 gpio_lcd_bl;
    u32 gpio_lcd_bl_power;
    /*VBAT*/
    u32 gpio_lcd_vbat;
    /*RESET*/
    u32 gpio_lcd_reset;
    /* Tp Reset*/
    u32 gpio_tp_reset;
    /*Te*/
    u32 gpio_lcd_te;
    u32 gpio_lcd_en;
    u32 gpio_lcd_mode;
    /*pcd errflag check gpio*/
    u32 gpio_pcd;
    u32 gpio_err_flag;
    u32 gpio_lcd_vddr;
    u32 gpio_lcd_vdd3;

    /* Qcom mipi power mode: 0--dcdc; 1--ldo */
    u8 mipi_regulator_mode;
    /*esd set backlight*/
    int esd_set_bl;

    /*bl chip init*/
    u32 bl_chip_init;
    u32 bl_dbc_set_boost_ctrl_flag;

    /*bl level limit of current for ic*/
    u32 bl_level_limit_percent;

    /* Hostprocessing : 0--No; 1--Yes */
    u8 is_hostprocessing;
    u8 host_info_all_in_ddic;
    /*Host 2D barcode support*/
    struct lcdkit_dsi_panel_cmds host_2d_barcode_enter_cmds;
    struct lcdkit_dsi_panel_cmds host_2d_barcode_cmds;
    struct lcdkit_dsi_panel_cmds host_2d_barcode_exit_cmds;
    u8 block_num_offset;
    /*Host project ID support*/
    u32 eml_read_reg_flag;
    struct lcdkit_dsi_panel_cmds host_project_id_enter_cmds;
    struct lcdkit_dsi_panel_cmds host_project_id_cmds;
    struct lcdkit_dsi_panel_cmds host_project_id_exit_cmds;

	/* color consistency support*/
    struct lcdkit_dsi_panel_cmds color_coordinate_enter_cmds;
    struct lcdkit_dsi_panel_cmds color_coordinate_cmds;
    struct lcdkit_dsi_panel_cmds color_coordinate_exit_cmds;

    /*panel info consistency support*/
    struct lcdkit_dsi_panel_cmds panel_info_consistency_enter_cmds;
    struct lcdkit_dsi_panel_cmds panel_info_consistency_cmds;
    struct lcdkit_dsi_panel_cmds panel_info_consistency_exit_cmds;
    /*different type white pictures  */
    struct lcdkit_dsi_panel_cmds rgbw_white_pixel_on_cmds;
    struct lcdkit_dsi_panel_cmds rgbw_rgb_pixel_on_cmds;
    /*AOD*/
    u8 aod_support;
    struct lcdkit_dsi_panel_cmds panel_enter_aod_cmds;
    struct lcdkit_dsi_panel_cmds panel_aod_low_brightness_cmds;
    struct lcdkit_dsi_panel_cmds panel_exit_aod_cmds;

    /*for delay ctrl*/
    uint32_t delay_af_vci_on;
    uint32_t delay_af_iovcc_on;
    uint32_t delay_af_vbat_on;
    uint32_t delay_af_bias_on;
    uint32_t delay_af_vdd_on;
    uint32_t delay_af_vsp_on;
    uint32_t delay_af_vsn_on;
    uint32_t delay_af_LP11;
    uint32_t delay_af_tp_reset;
    uint32_t delay_af_tp_after_resume;
    uint32_t delay_af_tp_before_suspend;
    u8 rst_after_vbat_flag;
    u8 lcdph_delay_set_flag;

    uint32_t delay_af_vsn_off;
    uint32_t delay_af_vsp_off;
    uint32_t delay_af_bias_off;
    uint32_t delay_af_vbat_off;
    uint32_t delay_af_iovcc_off;
    uint32_t delay_af_vci_off;
    uint32_t delay_af_rst_off;
    uint32_t delay_af_display_on;
    uint32_t delay_af_display_off;
    uint32_t delay_af_display_off_second;

    uint32_t delay_bf_bl;
    uint32_t delay_af_blic_init;

    uint32_t lp8556_bl_channel_config;
    uint32_t lp8556_bl_max_vboost_select;
    uint32_t backlight_ic_common;
    uint32_t iovcc_before_vci;
    uint32_t bl_byte_order;
    struct lcdbrightnesscoloroeminfo lcd_color_oeminfo;

    bool isbulcked;

    /* tp suspend*/
    u8 tp_suspend_after_lcd_sleep;
};

struct lcdkit_panel_data
{
    struct lcdkit_panel_infos panel_infos;
    void(*lcdkit_panel_on)(void* pdata);
    void(*lcdkit_panel_off)(void* pdata);
    void (*lcdkit_init)(struct device_node* np, void* pdata);
    void (*lcdkit_on_cmd)(void* pdata, struct lcdkit_dsi_panel_cmds* cmds);
    void (*lcdkit_off_cmd)(void* pdata, struct lcdkit_dsi_panel_cmds* cmds);
    void (*lcdkit_fps_timer_init)(void);
    ssize_t (*lcdkit_model_show)(char* buf);
    ssize_t (*lcdkit_display_type_show)(char* buf);
    ssize_t (*lcdkit_panel_info_show)(char* buf);
    ssize_t (*lcdkit_cabc_mode_show)(char* buf);
    ssize_t (*lcdkit_cabc_mode_store)(void* pdata, const char* buf);
    ssize_t (*lcdkit_inversion_mode_show)(char* buf);
    ssize_t (*lcdkit_inversion_mode_store)(void* pdata, const char* buf);
    ssize_t (*lcdkit_scan_mode_show)(char* buf);
    ssize_t (*lcdkit_scan_mode_store)(void* pdata, const char* buf);
    ssize_t (*lcdkit_check_reg_show)(void* pdata, char* buf);
    ssize_t (*lcdkit_gram_check_show)(void* pdata, char* buf);
    ssize_t (*lcdkit_gram_check_store)(void* pdata, const char* buf);
    ssize_t (*lcdkit_dynamic_sram_check_show)(void* pdata, char* buf);
    ssize_t (*lcdkit_dynamic_sram_check_store)(void* pdata, const char* buf);
    ssize_t (*lcdkit_ic_color_enhancement_mode_show)(char* buf);
    ssize_t (*lcdkit_ic_color_enhancement_mode_store)(void* pdata, const char* buf);
    ssize_t (*lcdkit_sleep_ctrl_show)(char* buf);
    ssize_t (*lcdkit_sleep_ctrl_store)(const char* buf);
    ssize_t (*lcdkit_lp2hs_mipi_check_show)(char* buf);
    ssize_t (*lcdkit_lp2hs_mipi_check_store)(void* pdata, const char* buf);
    ssize_t (*lcdkit_bist_check_show)(void* pdata, char* buf);
    ssize_t (*lcdkit_mipi_detect_show)(void* pdata, char* buf);
    ssize_t (*lcdkit_hkadc_debug_show)(char* buf);
    ssize_t (*lcdkit_hkadc_debug_store)(const char* buf);
    ssize_t (*lcdkit_voltage_enable_store)(void* pdata, const char* buf);
    ssize_t (*lcdkit_pcd_errflag_check)(char* buf );
    ssize_t (*lcdkit_amoled_acl_ctrl_show)(char* buf);
    ssize_t (*lcdkit_amoled_acl_ctrl_store)(void* pdata, const char* buf);
    ssize_t (*lcdkit_amoled_vr_mode_show)(char* buf);
    ssize_t (*lcdkit_amoled_vr_mode_store)(void* pdata, const char* buf);
    ssize_t (*lcdkit_amoled_hbm_ctrl_show)(char* buf);
    ssize_t (*lcdkit_amoled_hbm_ctrl_store)(void* pdata, const char* buf);
    ssize_t (*lcdkit_oem_info_show)(void* pdata, char* buf);
    ssize_t (*lcdkit_oem_info_store)(void* pdata, const char* buf);
    ssize_t (*lcdkit_support_mode_show)(char* buf);
    ssize_t (*lcdkit_support_mode_store)(const char* buf);
    ssize_t (*lcdkit_support_checkmode_show)(char* buf);
    ssize_t (*lcdkit_test_config_show)(void* pdata, char* buf);
    ssize_t (*lcdkit_test_config_store)(const char* buf);
    ssize_t (*lcdkit_ce_mode_show)(void* pdata, char* buf);
    ssize_t (*lcdkit_ce_mode_store)(void* pdata, const char* buf, size_t count);
    ssize_t (*lcdkit_reg_read_show)(void* pdata, char* buf);
    ssize_t (*lcdkit_reg_read_store)(void* pdata, const char *buf);

    ssize_t (*lcdkit_check_esd)(void* pdata);
    ssize_t (*lcdkit_set_display_region)(void* pdata, void* dirty);
    ssize_t (*lcdkit_fps_scence_handle)(struct platform_device* pdev, uint32_t scence);
    ssize_t (*lcdkit_fps_updt_handle)(void* pdata);
    ssize_t (*lcdkit_current_detect)(void* pdata);
    ssize_t (*lcdkit_lv_detect)(void* pdata);
    ssize_t (*lcdkit_bl_mode_show)(char* buf);
    ssize_t (*lcdkit_bl_mode_store)(void* pdata, const char* buf);
    ssize_t (*lcdkit_se_mode_show)(char* buf);
    ssize_t (*lcdkit_se_mode_store)(void* pdata, const char* buf);
    ssize_t (*lcdkit_support_bl_mode_show)(void* pdata, char* buf);
    ssize_t (*lcdkit_mipi_config_store)(void* pdata, const char* buf);
};

struct lcdkit_adapt_func
{
    char *name;
    ssize_t (*lcdkit_gram_check_show)(void* pdata, char* buf);
    ssize_t (*lcdkit_reg_read_show)(void* pdata, char* buf);
};

struct lcdkit_esd_error_info
{
	int esd_error_reg_num;
	int esd_reg_index[MAX_REG_READ_COUNT];
	int esd_expect_reg_val[MAX_REG_READ_COUNT];
	int esd_error_reg_val[MAX_REG_READ_COUNT];
};

ssize_t lcdkit_jdi_nt35696_5p5_gram_check_show(void* pdata, char* buf);
ssize_t lcdkit_jdi_nt35696_5p5_reg_read_show(void* pdata, char* buf);
ssize_t lcdkit_jdi_nt36860_5p88_reg_read_show(void* pdata, char* buf);

extern struct lcdkit_panel_data lcdkit_info;

#ifdef CONFIG_JDI_HOST_TS_KIT
extern int bu21150_otp_read(char *otp_data);
#endif

#define HOST_OEM_CHIP_TYPE_RESERVED 7
#define HOST_OEM_CHIP_TYPE_LEN_RESERVED  10

#define HOST_OEM_PRODUCT_ID_TYPE 0x00
#define HOST_OEM_2D_BARCODE_TYPE 0x01
#define HOST_OEM_BRIGHTNESS_TYPE 0x02
#define HOST_OEM_COLOROFWHITE_TYPE 0x03
#define HOST_OEM_BRIGHTNESSANDCOLOR_TYPE 0x04
#define HOST_OEM_REWORK_TYPE 0x05
#define HOST_OEM_BRIGHTNESS_COLOROFWHITE_TYPE 0x06
#define HOST_OEM_RGBW_WHITE_TYPE 0x07
#define LCD_OEM_BLOCK_NUM 1
#define LCD_OEM_2DBLOCK_NUM 3
#define LCD_OEM_BLOCK_LEN 16
#define LCD_OEM_INFO_LEN 64
#define TP_OEM_INFO_LEN 16
#define DDIC_PRODUCT_ID_OFFSET    10
#define DDIC_HW_PART_NUM_OFFSET   9
#define LCD_WHITECOLOR_LEN   7
#define DDIC_OEM_INFO_SIZE_MAX 500

#define LCD_DDIC_INFO_LEN 64

struct ddic_oem_cmd{
	unsigned char type;
	#ifndef CONFIG_ARCH_QCOM
	void (*func)(char *oem_data, struct hisi_fb_data_type *hisifd);
	#endif
};

struct lcdkit_panel_data* lcdkit_get_panel_info(void);
void lcdkit_init(struct device_node* np, void* pdata);
void lcdkit_on_cmd(void* pdata, struct lcdkit_dsi_panel_cmds* cmds);
void lcdkit_off_cmd(void* pdata, struct lcdkit_dsi_panel_cmds* cmds);
void lcdkit_dsi_tx(void* pdata, struct lcdkit_dsi_panel_cmds* cmds);
int lcdkit_dsi_rx(void* pdata, uint32_t* out, int len, struct lcdkit_dsi_panel_cmds*  cmds);
void lcdkit_switch_hs_lp(void* pdata, bool enable);
void lcdkit_gpio_init(struct device_node* np);
void lcdkit_info_init(void* pdata);
void lcdkit_mipi_dsi_max_return_packet_size(void* pdata, struct lcdkit_dsi_cmd_desc* cm);
bool lcdkit_is_cmd_panel(void);
void lcdkit_updt_porch(struct platform_device* pdev, int scence);
void lcdkit_lp2hs_mipi_test(void* pdata);
void lcdkit_effect_switch_ctrl(void* pdata, bool ctrl);
int adc_get_value(int channel);
void dfr_ctrl(struct platform_device* pdev, bool enable);
int lcdkit_fake_update_bl(void *pdata, uint32_t bl_level);
void lcdkit_fps_scence_adaptor_handle(struct platform_device* pdev, uint32_t scence);
void lcdkit_fps_updt_adaptor_handle(void* pdata);
#ifndef CONFIG_ARCH_QCOM
int lcdkit_rgbw_set_handle(struct hisi_fb_data_type* hisifd);
int lcdkit_hbm_set_handle(struct hisi_fb_data_type * hisifd);
#endif
void lcdkit_fps_timer_adaptor_init(void);
int lcdkit_lread_reg(void *pdata, uint32_t *out, struct lcdkit_dsi_cmd_desc* cmds, uint32_t len);
ssize_t host_panel_oem_info_show(void* pdata, char *buf);
ssize_t host_panel_oem_info_store(void* pdata, char *buf);
void lcdkit_fps_adaptor_ts_callback(void);
ssize_t lcdkit_get_bl_resume_timmer(void* pdata, int *buf);
ssize_t lcdkit_set_bl_normal_mode_reg(void* pdata);
ssize_t lcdkit_set_bl_enhance_mode_reg(void* pdata);
void lcdkit_update_lcd_brightness_info(void);
int lcdkit_check_mipi_fifo_empty(char __iomem *dsi_base);
int lcdkit_parse_array_data(struct device_node* np,
                                   char* name, struct lcdkit_array_data* out);
int lcdkit_parse_dcs_cmds(struct device_node* np, char* cmd_key,
                                 char* link_key, struct lcdkit_dsi_panel_cmds* pcmds);


static __maybe_unused inline int lcdkit_bias_is_gpio_ctrl_power(void)
{
    return (lcdkit_info.panel_infos.bias_power_ctrl_mode & POWER_CTRL_BY_GPIO) ? true : false;
}

static __maybe_unused inline int lcdkit_bias_is_regulator_ctrl_power(void)
{
    return (lcdkit_info.panel_infos.bias_power_ctrl_mode & POWER_CTRL_BY_REGULATOR) ? true : false;
}

static __maybe_unused inline int lcdkit_bias_is_ic_ctrl_power(void)
{
    return (lcdkit_info.panel_infos.bias_power_ctrl_mode & POWER_CTRL_BY_IC) ? true : false;
}

static __maybe_unused inline int lcdkit_bias_is_used_ctrl_power(void)
{
    return (lcdkit_bias_is_gpio_ctrl_power()||lcdkit_bias_is_regulator_ctrl_power()||lcdkit_bias_is_ic_ctrl_power()) ? true : false;
}

static __maybe_unused inline int lcdkit_vci_is_gpio_ctrl_power(void)
{
    return (lcdkit_info.panel_infos.vci_power_ctrl_mode & POWER_CTRL_BY_GPIO) ? true : false;
}

static __maybe_unused inline int lcdkit_vci_is_regulator_ctrl_power(void)
{
    return (lcdkit_info.panel_infos.vci_power_ctrl_mode & POWER_CTRL_BY_REGULATOR) ? true : false;
}

static __maybe_unused inline int lcdkit_iovcc_is_gpio_ctrl_power(void)
{
    return (lcdkit_info.panel_infos.iovcc_power_ctrl_mode & POWER_CTRL_BY_GPIO) ? true : false;
}

static __maybe_unused inline int lcdkit_vbat_is_gpio_ctrl_power(void)
{
    return (lcdkit_info.panel_infos.vbat_power_ctrl_mode & POWER_CTRL_BY_GPIO) ? true : false;
}

static __maybe_unused inline int lcdkit_iovcc_is_regulator_ctrl_power(void)
{
    return (lcdkit_info.panel_infos.iovcc_power_ctrl_mode & POWER_CTRL_BY_REGULATOR) ? true : false;
}

static __maybe_unused inline int lcdkit_vdd_is_regulator_ctrl_power(void)
{
    return (lcdkit_info.panel_infos.vdd_power_ctrl_mode & POWER_CTRL_BY_REGULATOR) ? true : false;
}


static __maybe_unused inline int lcdkit_is_lcd_panel(void)
{
    return (lcdkit_info.panel_infos.panel_type & PANEL_TYPE_LCD) ? true : false;
}

static __maybe_unused inline int lcdkit_is_oled_panel(void)
{
    return (lcdkit_info.panel_infos.panel_type & PANEL_TYPE_OLED) ? true : false;
}

#define LCDKIT_PANEL_CMD_REQUEST()  (down(&lcdkit_info.panel_infos.lcdkit_cmd_sem))
#define LCDKIT_PANEL_CMD_RELEASE()  (up(&lcdkit_info.panel_infos.lcdkit_cmd_sem))
#endif
