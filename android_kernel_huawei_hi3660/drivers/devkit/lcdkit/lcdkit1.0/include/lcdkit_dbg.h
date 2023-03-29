#ifndef __LCDKIT_DBG_H_
#define __LCDKIT_DBG_H_

#include <linux/debugfs.h>
#include <linux/ctype.h>
#include <linux/syscalls.h>
#include "lcdkit_panel.h"

/* define macro */
#define LCDKIT_DCS_STR              ("dcs_")
#define LCDKIT_GEN_STR              ("gen_")
#define LCDKIT_READ_STR             ("read_")
#define LCDKIT_WRITE_STR            ("write_")

//#define LCD_DEBUG_BUF	            (1024)
//#define LCD_PARAM_BUF	            (256)
#define LCDKIT_MAX_PARAM_NUM        (256)
#define LCDKIT_MAX_PARAM_BUF        (256)

#define LCDKIT_OPER_READ            (1)
#define LCDKIT_OPER_WRITE           (2)
#define LCDKIT_MIPI_PATH_OPEN       (1)
#define LCDKIT_MIPI_PATH_CLOSE      (0)
#define LCDKIT_MIPI_DCS_COMMAND     (1<<0)
#define LCDKIT_MIPI_GEN_COMMAND     (4)

#define LCDKIT_HEX_BASE ((char)16)
#define LCDKIT_IS_VALID_CHAR(_ch) ((_ch >= '0' && _ch <= '9')?1:\
                                   (_ch >= 'a' && _ch <= 'f')?1:(_ch >= 'A' && _ch <= 'F'))?1:0

#define DMD_SET_BRIGHTNESS_FAIL_EXPIER_COUNT (6)
#define DMD_SET_VOLTAGE_VSP_FALI_EXPIRE_COUNT (6)
#define DMD_SET_VOLTAGE_VSN_FALI_EXPIRE_COUNT (6)

/*
 * Message printing priorities:
 * LEVEL 0 KERN_EMERG (highest priority)
 * LEVEL 1 KERN_ALERT
 * LEVEL 2 KERN_CRIT
 * LEVEL 3 KERN_ERR
 * LEVEL 4 KERN_WARNING
 * LEVEL 5 KERN_NOTICE
 * LEVEL 6 KERN_INFO
 * LEVEL 7 KERN_DEBUG (Lowest priority)
 */
#define LCDKIT_EMERG(msg, ...)    \
    do { if (lcdkit_msg_level > 0)  \
            printk(KERN_EMERG "[lcdkit]%s: "msg, __func__, ## __VA_ARGS__); } while (0)
#define LCDKIT_ALERT(msg, ...)    \
    do { if (lcdkit_msg_level > 1)  \
            printk(KERN_ALERT "[lcdkit]%s: "msg, __func__, ## __VA_ARGS__); } while (0)
#define LCDKIT_CRIT(msg, ...)    \
    do { if (lcdkit_msg_level > 2)  \
            printk(KERN_CRIT "[lcdkit]%s: "msg, __func__, ## __VA_ARGS__); } while (0)
#define LCDKIT_ERR(msg, ...)    \
    do { if (lcdkit_msg_level > 3)  \
            printk(KERN_ERR "[lcdkit]%s: "msg, __func__, ## __VA_ARGS__); } while (0)
#define LCDKIT_WARNING(msg, ...)    \
    do { if (lcdkit_msg_level > 4)  \
            printk(KERN_WARNING "[lcdkit]%s: "msg, __func__, ## __VA_ARGS__); } while (0)
#define LCDKIT_NOTICE(msg, ...)    \
    do { if (lcdkit_msg_level > 5)  \
            printk(KERN_NOTICE "[lcdkit]%s: "msg, __func__, ## __VA_ARGS__); } while (0)
#define LCDKIT_INFO(msg, ...)    \
    do { if (lcdkit_msg_level > 6)  \
            printk(KERN_INFO "[lcdkit]%s: "msg, __func__, ## __VA_ARGS__); } while (0)
#define LCDKIT_DEBUG(msg, ...)    \
    do { if (lcdkit_msg_level > 7)  \
            printk(KERN_INFO "[lcdkit]%s: "msg, __func__, ## __VA_ARGS__); } while (0)

#ifndef TRUE
#define TRUE            1
#endif

#ifndef FALSE
#define FALSE           0
#endif

//#define HW_LCD_DEBUG	1

#define VCC_LCDKIT_BIAS_NAME                "vcc_lcdbias"
#define VCC_LCDKIT_VSN_NAME                 "lcd_vsn"
#define VCC_LCDKIT_VSP_NAME                 "lcd_vsp"

#define LCDKIT_VREG_VDD_NAME                "vdd"
#define LCDKIT_VREG_LAB_NAME                "lab"
#define LCDKIT_VREG_IBB_NAME                "ibb"
#define LCDKIT_VREG_BIAS_NAME               "bias"

#ifdef CONFIG_ARCH_SDM660
#define LCDKIT_VREG_VDDIO_NAME              "wqhd-vddio"
#else
#define LCDKIT_VREG_VDDIO_NAME              "vddio"
#endif

#define LCDKIT_INIT_TEST_PARAM          "/data/lcdkit_init_param.txt"
#define LCDKIT_CONFIG_TABLE_MAX_NUM     (2 * PAGE_SIZE)

enum lcdkit_cmds_type
{
    LCDKIT_DBG_LEVEL_SET = 0,
    LCDKIT_DBG_MIPI_CLK,
    LCDKIT_DBG_REG_READ,
    LCDKIT_DBG_REG_WRITE,
    LCDKIT_DBG_INIT_CODE,
    LCDKIT_DBG_PANEL_VSP_VSN,
    LCDKIT_DBG_ESD_ENABLE,
    LCDKIT_DBG_ESD_RECOVER_TEST,
    LCDKIT_DBG_ESD_RESET,
    LCDKIT_DBG_ESD_BL_ENABLE,
    LCDKIT_DBG_ESD_BL_SET,
    LCDKIT_DBG_ESD_CHECK_REG,
    LCDKIT_DBG_ESD_CHECK_VALUE,
    LCDKIT_DBG_NUM_MAX,
};


struct lcdkit_dsi_ctrl_hdr
{
    char dtype;	/* data type */
    char last;	/* last in chain */
    char vc;	/* virtual chan */
    char ack;	/* ask ACK from peripheral */
    char wait;	/* ms */
    char waittype;
    char dlen;	/* 8 bits */
};

typedef struct
{
    char type;
    char pstr[100];
} lcdkit_dbg_cmds;

struct lcdkit_esd_debug
{
    int esd_enable;
    char esd_check_reg[8];
    char esd_reg_value[8];
    int esd_bl_enable;
    int esd_bl_set;
    int check_count;
    int esd_recover_test;
};

struct lcdkit_debug
{
    int lcdkit_g_mipiclk;
    int lcdkit_g_mipiclk_enable;
    int lcdkit_g_initcode_enable;
    int g_initcode_flag;
    int lcdkit_g_vsp_vsn_enable;
    int lcdkit_panel_bias;
    int lcdkit_panel_vsp;
    int lcdkit_panel_vsn;

    int lcdkit_ic_mipi_reg;    // read register
    int lcdkit_ic_mipi_value[LCDKIT_MAX_PARAM_BUF];  // read value
    int lcdkit_g_param_num;

    //struct platform_device*
    void* lcdkit_ctrl_pdev;
    void* lcdkit_ctrl_pdata;

    struct lcdkit_esd_debug g_esd_debug;

};

//for extern
extern int lcdkit_msg_level;
extern struct lcdkit_debug lcdkit_dbg;

int lcdkit_debugfs_init(void);
int is_lcdkit_initcode_enable(void);
int is_lcdkit_mipiclk_enable(void);
int is_lcdkit_vsp_vsn_enable(void);
int get_lcdkit_mipiclk_dbg(void);
void *lcdkit_get_dsi_ctrl_pdata(void);
int lcdkit_lock(void *pdata);
void lcdkit_release(void *pdata);
void lcdkit_debug_set_vsp_vsn(void* vcc_cmds, int cnt);
int lcdkit_esd_debug(void* pdata);

//for debug
void lcdkit_dump_buf(const char* buf, int cnt);
void lcdkit_dump_buf_32(const u32* buf, int cnt);
void lcdkit_dump_cmds_desc(struct lcdkit_dsi_cmd_desc* desc);
void lcdkit_dump_cmds(struct lcdkit_dsi_panel_cmds* cmds);
void lcdkit_dump_array_data(struct lcdkit_array_data* array);
void lcdkit_dump_arrays_data(struct lcdkit_arrays_data* arrays);
#endif
