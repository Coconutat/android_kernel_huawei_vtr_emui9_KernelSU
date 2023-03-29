#include "hisi_fb.h"

#ifndef MIPI_SHARP_TD4322_5P5
#define MIPI_SHARP_TD4322_5P5

enum {
	LCD_BIST_CHECK_FAIL = -1,
	LCD_BIST_CHECK_TIMEOUT = -2,
	LCD_BIST_CHECK_PASS = 0,
};

/******************************************
* LCD defect check code
*/
static char defect_check_rst0[] = {
	0xFF,
	0x10,
};

static char defect_check_rst1[] = {
	0x28,
	0x00,
};//delay 50

static char defect_check_rst2[] = {
	0x10,
	0x00,
};//delay 100

static char defect_step1_0[] = {
	0xFF,
	0xF0,
};

static char defect_step1_1[] = {
	0xB9,
	0xA0,
};

static char defect_step1_2[] = {
	0xFB,
	0x01,
};

static char defect_step1_3[] = {
	0xFF,
	0x24,
};

static char defect_step1_4[] = {
	0xE3,
	0x00,
};

static char defect_step1_5[] = {
	0x6E,
	0x10,
};

static char defect_step1_6[] = {
	0xC2,
	0x00,
};

static char defect_step1_7[] = {
	0xFB,
	0x01,
};

static char defect_step1_8[] = {
	0xFF,
	0x10,
};

static char defect_step1_9[] = {
	0x11,
	0x00,
};//delay 200

static char defect_step1_10[] = {
	0xB8,
	0XAD,
};

static char defect_step1_11[] = {
	0xB5,
	0x86,
};

static char defect_step1_12[] = {
	0xB6,
	0x77,
};

static char defect_step1_13[] = {
	0xFF,
	0xF0,
};

static char defect_step1_14[] = {
	0xBD,
	0x00,
};

static char defect_step1_15[] = {
	0xB3,
	0x55,
};

static char defect_step1_16[] = {
	0xB4,
	0xAA,
};

static char defect_step1_17[] = {
	0xB5,
	0x55,
};

static char defect_step1_18[] = {
	0xB6,
	0xAA,
};

static char defect_step1_19[] = {
	0xB7,
	0x55,
};

static char defect_step1_20[] = {
	0xB8,
	0xAA,
};

static char defect_step1_21[] = {
	0xB9,
	0xA8,
};

//delay 800
//REGR 0xD8,0x3F
//REGR 0xD9,0x00
//REGR 0xBC,0x70

static char defect_step2_0[] = {
	0xB9,
	0x80,
};

static char defect_step2_1[] = {
	0xB3,
	0x00,
};

static char defect_step2_2[] = {
	0xB4,
	0x00,
};

static char defect_step2_3[] = {
	0xB5,
	0x00,
};

static char defect_step2_4[] = {
	0xB6,
	0x00,
};

static char defect_step2_5[] = {
	0xB7,
	0x00,
};

static char defect_step2_6[] = {
	0xB8,
	0x00,
};

static char defect_step2_7[] = {
	0xB9,
	0xA4,
};

//delay 800
//REGR 0xD8,0x3F
//REGR 0xD9,0x00
//REGR 0xBC,0x70

static char defect_step3_0[] = {
	0xB9,
	0x80,
};//delay 40

//reset

static char defect_step3_1[] = {
	0xFF,
	0xF0,
};

static char defect_step3_2[] = {
	0xB9,
	0xA0,
};

static char defect_step3_3[] = {
	0xFB,
	0x01,
};

static char defect_step3_4[] = {
	0xFF,
	0x24,
};

static char defect_step3_5[] = {
	0xE3,
	0x00,
};

static char defect_step3_6[] = {
	0x6E,
	0x10,
};

static char defect_step3_7[] = {
	0xC2,
	0x00,
};

static char defect_step3_8[] = {
	0xFB,
	0x01,
};

static char defect_step3_9[] = {
	0xFF,
	0x10,
};

static char defect_step3_10[] = {
	0x11,
	0x00,
}; //delay 50

static char defect_step3_11[] = {
	0xB8,
	0xAD,
};

static char defect_step3_12[] = {
	0xB5,
	0x86,
};

static char defect_step3_13[] = {
	0xB6,
	0x77,
};

static char defect_step3_14[] = {
	0xFF,
	0xF0,
};

static char defect_step3_15[] = {
	0xBD,
	0x00,
};

static char defect_step3_16[] = {
	0xB3,
	0x55,
};

static char defect_step3_17[] = {
	0xB4,
	0xAA,
};

static char defect_step3_18[] = {
	0xB5,
	0x55,
};

static char defect_step3_19[] = {
	0xB6,
	0xAA,
};

static char defect_step3_20[] = {
	0xB7,
	0x55,
};

static char defect_step3_21[] = {
	0xB8,
	0xAA,
};

static char defect_step3_22[] = {
	0xB9,
	0xA4,
};

//delay 800
//REGR 0xD8,0x3F
//REGR 0xD9,0x00
//REGR 0xBC,0x70

static char defect_step4_0[] = {
	0xB9,
	0x80,
};

static char defect_step4_1[] = {
	0xB3,
	0x00,
};

static char defect_step4_2[] = {
	0xB4,
	0x00,
};

static char defect_step4_3[] = {
	0xB5,
	0x00,
};

static char defect_step4_4[] = {
	0xB6,
	0x00,
};

static char defect_step4_5[] = {
	0xB7,
	0x00,
};

static char defect_step4_6[] = {
	0xB8,
	0x00,
};

static char defect_step4_7[] = {
	0xB9,
	0xA8
};//delay 800

static char defect_step4_8[] = {
	0xB9,
	0xA8,
};

//delay 800
//REGR 0xD8,0x3F
//REGR 0xD9,0x00
//REGR 0xBC,0x70

//step 5 test1
static char defect_step5_0[] = {
	0xB9,
	0x80,
};

//reset

static char defect_step5_1[] = {
	0xFF,
	0xF0,
};

static char defect_step5_2[] = {
	0xB9,
	0xA0,
};

static char defect_step5_no_reload[] = {
	0xFB,
	0x01,
};

static char defect_step5_3[] = {
	0xFF,
	0x24,
};

static char defect_step5_4[] = {
	0xE3,
	0x00,
};

static char defect_step5_5[] = {
	0x6E,
	0x10,
};

static char defect_step5_6[] = {
	0xC2,
	0x00,
};

static char defect_step5_7[] = {
	0xFB,
	0x01,
};

static char defect_step5_8[] = {
	0xFF,
	0x10,
};

static char defect_step5_9[] = {
	0x11,
	0x00,
}; //delay 50

static char defect_step5_10[] = {
	0xB8,
	0XAD,
};

static char defect_step5_11[] = {
	0xB5,
	0x86,
};

static char defect_step5_12[] = {
	0xB6,
	0x77,
};

static char defect_step5_13[] = {
	0xFF,
	0xF0,
};

static char defect_step5_14[] = {
	0xBD,
	0x00,
};

static char defect_step5_15[] = {
	0xB3,
	0x55,
};

static char defect_step5_16[] = {
	0xB4,
	0xAA,
};

static char defect_step5_17[] = {
	0xB5,
	0x55,
};

static char defect_step5_18[] = {
	0xB6,
	0xAA,
};

static char defect_step5_19[] = {
	0xB7,
	0x55,
};

static char defect_step5_20[] = {
	0xB8,
	0xAA,
};

static char defect_step5_21[] = {
	0xB9,
	0xA2,
};

//delay 10

static char defect_step5_22[] = {
	0xB9,
	0xA1,
};

//delay 800
//REGR 0xD8,0x3F
//REGR 0xD9,0x00
//REGR 0xBC,0x68

//step 6 test1
static char defect_step6_0[] = {
	0xB9,
	0x80,
};

static char defect_step6_1[] = {
	0xB3,
	0xFF,
};

static char defect_step6_2[] = {
	0xB4,
	0x00,
};

static char defect_step6_3[] = {
	0xB5,
	0xFF,
};

static char defect_step6_4[] = {
	0xB6,
	0x00,
};

static char defect_step6_5[] = {
	0xB7,
	0xFF,
};

static char defect_step6_6[] = {
	0xB8,
	0x00,
};

static char defect_step6_7[] = {
	0xB9,
	0xA8,
};

//delay 800
//REGR 0xD8,0x3F
//REGR 0xD9,0x00
//REGR 0xBC,0x78

//step7 test1

static char defect_step7_0[] = {
	0xB9,
	0x80,
};

static char defect_step7_1[] = {
	0xB3,
	0xAA,
};

static char defect_step7_2[] = {
	0xB4,
	0xAA,
};

static char defect_step7_3[] = {
	0xB5,
	0xAA,
};

static char defect_step7_4[] = {
	0xB6,
	0x55,
};

static char defect_step7_5[] = {
	0xB7,
	0x55,
};

static char defect_step7_6[] = {
	0xB8,
	0x55,
};

static char defect_step7_7[] = {
	0xB9,
	0xA8,
};

//delay 800
//REGR 0xD8,0x3F
//REGR 0xD9,0x00
//REGR 0xBC,0x78

//step8 test1
static char defect_step8_0[] = {
	0xB9,
	0x80,
};

//reset

static char defect_step8_1[] = {
	0xFF,
	0xF0,
};

static char defect_step8_2[] = {
	0xB9,
	0xA0,
};

static char defect_step8_3[] = {
	0xFB,
	0x01,
};

static char defect_step8_4[] = {
	0xFF,
	0x24,
};

static char defect_step8_5[] = {
	0xE3,
	0x00,
};

static char defect_step8_6[] = {
	0x6E,
	0x10,
};

static char defect_step8_7[] = {
	0xC2,
	0x00,
};

static char defect_step8_8[] = {
	0xFB,
	0x01,
};

static char defect_step8_9[] = {
	0xFF,
	0x10,
};

static char defect_step8_10[] = {
	0x11,
	0x00,
};

//delay 50

static char defect_step8_11[] = {
	0xB8,
	0xAD,
};

static char defect_step8_12[] = {
	0xB5,
	0x86,
};

static char defect_step8_13[] = {
	0xB6,
	0x77,
};

static char defect_step8_14[] = {
	0xFF,
	0xF0,
};

static char defect_step8_15[] = {
	0xBD,
	0x00,
};

static char defect_step8_16[] = {
	0xB3,
	0xAA,
};

static char defect_step8_17[] = {
	0xB4,
	0x55,
};

static char defect_step8_18[] = {
	0xB5,
	0xAA,
};

static char defect_step8_19[] = {
	0xB6,
	0x55,
};

static char defect_step8_20[] = {
	0xB7,
	0xAA,
};

static char defect_step8_21[] = {
	0xB8,
	0x55,
};

static char defect_step8_22[] = {
	0xC7,
	0x80,
};

static char defect_step8_23[] = {
	0xB9,
	0xA0,
};

//delay 800
//REGR 0xBC,0x60

static char defect_end_0[] = {
	0xB9,
	0x80,
};

#if 0
static char defect_end_FF_10[] = {
	0xFF,
	0x10,
};

static char defect_end_sleep_out[] = {
	0x11,
	0x00,
};

static char defect_end_disp_on[] = {
	0x29,
	0x00,
};
#endif


static struct dsi_cmd_desc sharp_display_bist_check_cmds1[] = {
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_check_rst0), defect_check_rst0},
	{DTYPE_DCS_WRITE1, 0, 50, WAIT_TYPE_MS,
		sizeof(defect_check_rst1), defect_check_rst1},
	{DTYPE_DCS_WRITE1, 0, 100, WAIT_TYPE_MS,
		sizeof(defect_check_rst2), defect_check_rst2},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step1_0), defect_step1_0},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step1_1), defect_step1_1},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step1_2), defect_step1_2},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step1_3), defect_step1_3},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step1_4), defect_step1_4},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step1_5), defect_step1_5},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step1_6), defect_step1_6},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step1_7), defect_step1_7},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_MS,
		sizeof(defect_step1_8), defect_step1_8},
	{DTYPE_DCS_WRITE1, 0, 200, WAIT_TYPE_MS,
		sizeof(defect_step1_9), defect_step1_9},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step1_10), defect_step1_10},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step1_11), defect_step1_11},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step1_12), defect_step1_12},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step1_13), defect_step1_13},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step1_14), defect_step1_14},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step1_15), defect_step1_15},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step1_16), defect_step1_16},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step1_17), defect_step1_17},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step1_18), defect_step1_18},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step1_19), defect_step1_19},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step1_20), defect_step1_20},
	{DTYPE_DCS_WRITE1, 0, 3200, WAIT_TYPE_MS,
		sizeof(defect_step1_21), defect_step1_21},
};

static struct dsi_cmd_desc sharp_display_bist_check_cmds2[] = {
	{DTYPE_DCS_WRITE1, 0, 200, WAIT_TYPE_MS,
		sizeof(defect_step2_0), defect_step2_0},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step2_1), defect_step2_1},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step2_2), defect_step2_2},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step2_3), defect_step2_3},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step2_4), defect_step2_4},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step2_5), defect_step2_5},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step2_6), defect_step2_6},
	{DTYPE_DCS_WRITE1, 0, 3200, WAIT_TYPE_MS,
		sizeof(defect_step2_7), defect_step2_7},
};

static struct dsi_cmd_desc sharp_display_bist_check_cmds3[] = {
	{DTYPE_DCS_WRITE1, 0, 40, WAIT_TYPE_MS,
		sizeof(defect_step3_0), defect_step3_0},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_check_rst0), defect_check_rst0},
	{DTYPE_DCS_WRITE1, 0, 50, WAIT_TYPE_MS,
		sizeof(defect_check_rst1), defect_check_rst1},
	{DTYPE_DCS_WRITE1, 0, 100, WAIT_TYPE_MS,
		sizeof(defect_check_rst2), defect_check_rst2},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step3_1), defect_step3_1},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step3_2), defect_step3_2},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step3_3), defect_step3_3},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step3_4), defect_step3_4},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step3_5), defect_step3_5},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step3_6), defect_step3_6},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step3_7), defect_step3_7},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step3_8), defect_step3_8},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step3_9), defect_step3_9},
	{DTYPE_DCS_WRITE1, 0, 50, WAIT_TYPE_MS,
		sizeof(defect_step3_10), defect_step3_10},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step3_11), defect_step3_11},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step3_12), defect_step3_12},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step3_13), defect_step3_13},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step3_14), defect_step3_14},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step3_15), defect_step3_15},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step3_16), defect_step3_16},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step3_17), defect_step3_17},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step3_18), defect_step3_18},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step3_19), defect_step3_19},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step3_20), defect_step3_20},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step3_21), defect_step3_21},
	{DTYPE_DCS_WRITE1, 0, 3200, WAIT_TYPE_MS,
		sizeof(defect_step3_22), defect_step3_22},
};

static struct dsi_cmd_desc sharp_display_bist_check_cmds4[] = {
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step4_0), defect_step4_0},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step4_1), defect_step4_1},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step4_2), defect_step4_2},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step4_3), defect_step4_3},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step4_4), defect_step4_4},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step4_5), defect_step4_5},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step4_6), defect_step4_6},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step4_7), defect_step4_7},
	{DTYPE_DCS_WRITE1, 0, 3200, WAIT_TYPE_MS,
		sizeof(defect_step4_8), defect_step4_8},
};

static struct dsi_cmd_desc sharp_display_bist_check_cmds5[] = {
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step5_0), defect_step5_0},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_check_rst0), defect_check_rst0},
	{DTYPE_DCS_WRITE1, 0, 50, WAIT_TYPE_MS,
		sizeof(defect_check_rst1), defect_check_rst1},
	{DTYPE_DCS_WRITE1, 0, 100, WAIT_TYPE_MS,
		sizeof(defect_check_rst2), defect_check_rst2},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step5_1), defect_step5_1},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step5_2), defect_step5_2},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step5_no_reload), defect_step5_no_reload},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step5_3), defect_step5_3},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step5_4), defect_step5_4},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step5_5), defect_step5_5},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step5_6), defect_step5_6},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step5_7), defect_step5_7},
	{DTYPE_DCS_WRITE1, 0, 200, WAIT_TYPE_US,
		sizeof(defect_step5_8), defect_step5_8},
	{DTYPE_DCS_WRITE1, 0, 200, WAIT_TYPE_MS,
		sizeof(defect_step5_9), defect_step5_9},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step5_10), defect_step5_10},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step5_11), defect_step5_11},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step5_12), defect_step5_12},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step5_13), defect_step5_13},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step5_14), defect_step5_14},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step5_15), defect_step5_15},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step5_16), defect_step5_16},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step5_17), defect_step5_17},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step5_18), defect_step5_18},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step5_19), defect_step5_19},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step5_20), defect_step5_20},
	{DTYPE_DCS_WRITE1, 0, 100, WAIT_TYPE_MS,
		sizeof(defect_step5_21), defect_step5_21},
	{DTYPE_DCS_WRITE1, 0, 3200, WAIT_TYPE_MS,
		sizeof(defect_step5_22), defect_step5_22},
};

static struct dsi_cmd_desc sharp_display_bist_check_cmds6[] = {
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step6_0), defect_step6_0},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step6_1), defect_step6_1},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step6_2), defect_step6_2},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step6_3), defect_step6_3},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step6_4), defect_step6_4},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step6_5), defect_step6_5},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step6_6), defect_step6_6},
	{DTYPE_DCS_WRITE1, 0, 3200, WAIT_TYPE_MS,
		sizeof(defect_step6_7), defect_step6_7},
};

static struct dsi_cmd_desc sharp_display_bist_check_cmds7[] = {
	{DTYPE_DCS_WRITE1, 0, 150, WAIT_TYPE_US,
		sizeof(defect_step7_0), defect_step7_0},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step7_1), defect_step7_1},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step7_2), defect_step7_2},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step7_3), defect_step7_3},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step7_4), defect_step7_4},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step7_5), defect_step7_5},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step7_6), defect_step7_6},
	{DTYPE_DCS_WRITE1, 0, 3200, WAIT_TYPE_MS,
		sizeof(defect_step7_7), defect_step7_7},
};

static struct dsi_cmd_desc sharp_display_bist_check_cmds8[] = {
	{DTYPE_DCS_WRITE1, 0, 150, WAIT_TYPE_MS,
		sizeof(defect_step8_0), defect_step8_0},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_check_rst0), defect_check_rst0},
	{DTYPE_DCS_WRITE1, 0, 50, WAIT_TYPE_MS,
		sizeof(defect_check_rst1), defect_check_rst1},
	{DTYPE_DCS_WRITE1, 0, 100, WAIT_TYPE_MS,
		sizeof(defect_check_rst2), defect_check_rst2},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step8_1), defect_step8_1},
	{DTYPE_DCS_WRITE1, 0, 200, WAIT_TYPE_US,
		sizeof(defect_step8_2), defect_step8_2},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step8_3), defect_step8_3},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step8_4), defect_step8_4},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step8_5), defect_step8_5},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step8_6), defect_step8_6},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step8_7), defect_step8_7},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step8_8), defect_step8_8},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step8_9), defect_step8_9},
	{DTYPE_DCS_WRITE1, 0, 200, WAIT_TYPE_MS,
		sizeof(defect_step8_10), defect_step8_10},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step8_11), defect_step8_11},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step8_12), defect_step8_12},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step8_13), defect_step8_13},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step8_14), defect_step8_14},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step8_15), defect_step8_15},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step8_16), defect_step8_16},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step8_17), defect_step8_17},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step8_18), defect_step8_18},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step8_19), defect_step8_19},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step8_20), defect_step8_20},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step8_21), defect_step8_21},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_step8_22), defect_step8_22},
	{DTYPE_DCS_WRITE1, 0, 3200, WAIT_TYPE_MS,
		sizeof(defect_step8_23), defect_step8_23},

};

static struct dsi_cmd_desc sharp_display_bist_check_end[] = {
	{DTYPE_DCS_WRITE1, 0, 200, WAIT_TYPE_MS,
		sizeof(defect_end_0), defect_end_0},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_check_rst0), defect_check_rst0},
	{DTYPE_DCS_WRITE1, 0, 50, WAIT_TYPE_MS,
		sizeof(defect_check_rst1), defect_check_rst1},
	{DTYPE_DCS_WRITE1, 0, 100, WAIT_TYPE_MS,
		sizeof(defect_check_rst2), defect_check_rst2},
#if 0
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_end_FF_10), defect_end_FF_10},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_end_sleep_out), defect_end_sleep_out},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(defect_end_disp_on), defect_end_disp_on},
#endif
};

static u32 gamma_lut_table_R[] = {
	  0,	 16,	 32,	 48,	 64,	 80,	 96,	112,	128,	144,
	160,	176,	192,	208,	224,	240,	256,	272,	288,	304,
	320,	336,	352,	368,	384,	400,	416,	432,	448,	464,
	480,	496,	512,	528,	544,	560,	576,	592,	608,	624,
	640,	656,	672,	688,	704,	720,	736,	752,	768,	784,
	800,	816,	832,	848,	864,	880,	896,	912,	928,	944,
	960,	976,	992,	1008,	1024,	1040,	1056,	1072,	1088,	1104,
	1120,	1136,	1152,	1168,	1184,	1200,	1216,	1232,	1248,	1264,
	1280,	1296,	1312,	1328,	1344,	1360,	1376,	1392,	1408,	1424,
	1440,	1456,	1472,	1488,	1504,	1520,	1536,	1552,	1568,	1584,
	1600,	1616,	1632,	1648,	1664,	1680,	1696,	1712,	1728,	1744,
	1760,	1776,	1792,	1808,	1824,	1840,	1856,	1872,	1888,	1904,
	1920,	1936,	1952,	1968,	1984,	2000,	2016,	2032,	2048,	2064,
	2080,	2096,	2112,	2128,	2144,	2160,	2176,	2192,	2208,	2224,
	2240,	2256,	2272,	2288,	2304,	2320,	2336,	2352,	2368,	2384,
	2400,	2416,	2432,	2448,	2464,	2480,	2496,	2512,	2528,	2544,
	2560,	2576,	2592,	2608,	2624,	2640,	2656,	2672,	2688,	2704,
	2720,	2736,	2752,	2768,	2784,	2800,	2816,	2832,	2848,	2864,
	2880,	2896,	2912,	2928,	2944,	2960,	2976,	2992,	3008,	3024,
	3040,	3056,	3072,	3088,	3104,	3120,	3136,	3152,	3168,	3184,
	3200,	3216,	3232,	3248,	3264,	3280,	3296,	3312,	3328,	3344,
	3360,	3376,	3392,	3408,	3424,	3440,	3456,	3472,	3488,	3504,
	3520,	3536,	3552,	3568,	3584,	3600,	3616,	3632,	3648,	3664,
	3680,	3696,	3712,	3728,	3744,	3760,	3776,	3792,	3808,	3824,
	3840,	3856,	3872,	3888,	3904,	3920,	3936,	3952,	3968,	3984,
	4000,	4016,	4032,	4048,	4064,	4080,	4095
};

static u32 gamma_lut_table_G[] = {
	  0,	 16,	 32,	 48,	 64,	 80,	 96,	112,	128,	144,
	160,	176,	192,	208,	224,	240,	256,	272,	288,	304,
	320,	336,	352,	368,	384,	400,	416,	432,	448,	464,
	480,	496,	512,	528,	544,	560,	576,	592,	608,	624,
	640,	656,	672,	688,	704,	720,	736,	752,	768,	784,
	800,	816,	832,	848,	864,	880,	896,	912,	928,	944,
	960,	976,	992,	1008,	1024,	1040,	1056,	1072,	1088,	1104,
	1120,	1136,	1152,	1168,	1184,	1200,	1216,	1232,	1248,	1264,
	1280,	1296,	1312,	1328,	1344,	1360,	1376,	1392,	1408,	1424,
	1440,	1456,	1472,	1488,	1504,	1520,	1536,	1552,	1568,	1584,
	1600,	1616,	1632,	1648,	1664,	1680,	1696,	1712,	1728,	1744,
	1760,	1776,	1792,	1808,	1824,	1840,	1856,	1872,	1888,	1904,
	1920,	1936,	1952,	1968,	1984,	2000,	2016,	2032,	2048,	2064,
	2080,	2096,	2112,	2128,	2144,	2160,	2176,	2192,	2208,	2224,
	2240,	2256,	2272,	2288,	2304,	2320,	2336,	2352,	2368,	2384,
	2400,	2416,	2432,	2448,	2464,	2480,	2496,	2512,	2528,	2544,
	2560,	2576,	2592,	2608,	2624,	2640,	2656,	2672,	2688,	2704,
	2720,	2736,	2752,	2768,	2784,	2800,	2816,	2832,	2848,	2864,
	2880,	2896,	2912,	2928,	2944,	2960,	2976,	2992,	3008,	3024,
	3040,	3056,	3072,	3088,	3104,	3120,	3136,	3152,	3168,	3184,
	3200,	3216,	3232,	3248,	3264,	3280,	3296,	3312,	3328,	3344,
	3360,	3376,	3392,	3408,	3424,	3440,	3456,	3472,	3488,	3504,
	3520,	3536,	3552,	3568,	3584,	3600,	3616,	3632,	3648,	3664,
	3680,	3696,	3712,	3728,	3744,	3760,	3776,	3792,	3808,	3824,
	3840,	3856,	3872,	3888,	3904,	3920,	3936,	3952,	3968,	3984,
	4000,	4016,	4032,	4048,	4064,	4080,	4095
};

static u32 gamma_lut_table_B[] = {
	  0,	 16,	 32,	 48,	 64,	 80,	 96,	112,	128,	144,
	160,	176,	192,	208,	224,	240,	256,	272,	288,	304,
	320,	336,	352,	368,	384,	400,	416,	432,	448,	464,
	480,	496,	512,	528,	544,	560,	576,	592,	608,	624,
	640,	656,	672,	688,	704,	720,	736,	752,	768,	784,
	800,	816,	832,	848,	864,	880,	896,	912,	928,	944,
	960,	976,	992,	1008,	1024,	1040,	1056,	1072,	1088,	1104,
	1120,	1136,	1152,	1168,	1184,	1200,	1216,	1232,	1248,	1264,
	1280,	1296,	1312,	1328,	1344,	1360,	1376,	1392,	1408,	1424,
	1440,	1456,	1472,	1488,	1504,	1520,	1536,	1552,	1568,	1584,
	1600,	1616,	1632,	1648,	1664,	1680,	1696,	1712,	1728,	1744,
	1760,	1776,	1792,	1808,	1824,	1840,	1856,	1872,	1888,	1904,
	1920,	1936,	1952,	1968,	1984,	2000,	2016,	2032,	2048,	2064,
	2080,	2096,	2112,	2128,	2144,	2160,	2176,	2192,	2208,	2224,
	2240,	2256,	2272,	2288,	2304,	2320,	2336,	2352,	2368,	2384,
	2400,	2416,	2432,	2448,	2464,	2480,	2496,	2512,	2528,	2544,
	2560,	2576,	2592,	2608,	2624,	2640,	2656,	2672,	2688,	2704,
	2720,	2736,	2752,	2768,	2784,	2800,	2816,	2832,	2848,	2864,
	2880,	2896,	2912,	2928,	2944,	2960,	2976,	2992,	3008,	3024,
	3040,	3056,	3072,	3088,	3104,	3120,	3136,	3152,	3168,	3184,
	3200,	3216,	3232,	3248,	3264,	3280,	3296,	3312,	3328,	3344,
	3360,	3376,	3392,	3408,	3424,	3440,	3456,	3472,	3488,	3504,
	3520,	3536,	3552,	3568,	3584,	3600,	3616,	3632,	3648,	3664,
	3680,	3696,	3712,	3728,	3744,	3760,	3776,	3792,	3808,	3824,
	3840,	3856,	3872,	3888,	3904,	3920,	3936,	3952,	3968,	3984,
	4000,	4016,	4032,	4048,	4064,	4080,	4095
};

static u32 igm_lut_table_R[] = {
	  0,	 16,	 32,	 48,	 64,	 80,	 96,	112,	128,	144,
	160,	176,	192,	208,	224,	240,	256,	272,	288,	304,
	320,	336,	352,	368,	384,	400,	416,	432,	448,	464,
	480,	496,	512,	528,	544,	560,	576,	592,	608,	624,
	640,	656,	672,	688,	704,	720,	736,	752,	768,	784,
	800,	816,	832,	848,	864,	880,	896,	912,	928,	944,
	960,	976,	992,	1008,	1024,	1040,	1056,	1072,	1088,	1104,
	1120,	1136,	1152,	1168,	1184,	1200,	1216,	1232,	1248,	1264,
	1280,	1296,	1312,	1328,	1344,	1360,	1376,	1392,	1408,	1424,
	1440,	1456,	1472,	1488,	1504,	1520,	1536,	1552,	1568,	1584,
	1600,	1616,	1632,	1648,	1664,	1680,	1696,	1712,	1728,	1744,
	1760,	1776,	1792,	1808,	1824,	1840,	1856,	1872,	1888,	1904,
	1920,	1936,	1952,	1968,	1984,	2000,	2016,	2032,	2048,	2064,
	2080,	2096,	2112,	2128,	2144,	2160,	2176,	2192,	2208,	2224,
	2240,	2256,	2272,	2288,	2304,	2320,	2336,	2352,	2368,	2384,
	2400,	2416,	2432,	2448,	2464,	2480,	2496,	2512,	2528,	2544,
	2560,	2576,	2592,	2608,	2624,	2640,	2656,	2672,	2688,	2704,
	2720,	2736,	2752,	2768,	2784,	2800,	2816,	2832,	2848,	2864,
	2880,	2896,	2912,	2928,	2944,	2960,	2976,	2992,	3008,	3024,
	3040,	3056,	3072,	3088,	3104,	3120,	3136,	3152,	3168,	3184,
	3200,	3216,	3232,	3248,	3264,	3280,	3296,	3312,	3328,	3344,
	3360,	3376,	3392,	3408,	3424,	3440,	3456,	3472,	3488,	3504,
	3520,	3536,	3552,	3568,	3584,	3600,	3616,	3632,	3648,	3664,
	3680,	3696,	3712,	3728,	3744,	3760,	3776,	3792,	3808,	3824,
	3840,	3856,	3872,	3888,	3904,	3920,	3936,	3952,	3968,	3984,
	4000,	4016,	4032,	4048,	4064,	4080,	4095
};

static u32 igm_lut_table_G[] = {
	  0,	 16,	 32,	 48,	 64,	 80,	 96,	112,	128,	144,
	160,	176,	192,	208,	224,	240,	256,	272,	288,	304,
	320,	336,	352,	368,	384,	400,	416,	432,	448,	464,
	480,	496,	512,	528,	544,	560,	576,	592,	608,	624,
	640,	656,	672,	688,	704,	720,	736,	752,	768,	784,
	800,	816,	832,	848,	864,	880,	896,	912,	928,	944,
	960,	976,	992,	1008,	1024,	1040,	1056,	1072,	1088,	1104,
	1120,	1136,	1152,	1168,	1184,	1200,	1216,	1232,	1248,	1264,
	1280,	1296,	1312,	1328,	1344,	1360,	1376,	1392,	1408,	1424,
	1440,	1456,	1472,	1488,	1504,	1520,	1536,	1552,	1568,	1584,
	1600,	1616,	1632,	1648,	1664,	1680,	1696,	1712,	1728,	1744,
	1760,	1776,	1792,	1808,	1824,	1840,	1856,	1872,	1888,	1904,
	1920,	1936,	1952,	1968,	1984,	2000,	2016,	2032,	2048,	2064,
	2080,	2096,	2112,	2128,	2144,	2160,	2176,	2192,	2208,	2224,
	2240,	2256,	2272,	2288,	2304,	2320,	2336,	2352,	2368,	2384,
	2400,	2416,	2432,	2448,	2464,	2480,	2496,	2512,	2528,	2544,
	2560,	2576,	2592,	2608,	2624,	2640,	2656,	2672,	2688,	2704,
	2720,	2736,	2752,	2768,	2784,	2800,	2816,	2832,	2848,	2864,
	2880,	2896,	2912,	2928,	2944,	2960,	2976,	2992,	3008,	3024,
	3040,	3056,	3072,	3088,	3104,	3120,	3136,	3152,	3168,	3184,
	3200,	3216,	3232,	3248,	3264,	3280,	3296,	3312,	3328,	3344,
	3360,	3376,	3392,	3408,	3424,	3440,	3456,	3472,	3488,	3504,
	3520,	3536,	3552,	3568,	3584,	3600,	3616,	3632,	3648,	3664,
	3680,	3696,	3712,	3728,	3744,	3760,	3776,	3792,	3808,	3824,
	3840,	3856,	3872,	3888,	3904,	3920,	3936,	3952,	3968,	3984,
	4000,	4016,	4032,	4048,	4064,	4080,	4095
};

static u32 igm_lut_table_B[] = {
	  0,	 16,	 32,	 48,	 64,	 80,	 96,	112,	128,	144,
	160,	176,	192,	208,	224,	240,	256,	272,	288,	304,
	320,	336,	352,	368,	384,	400,	416,	432,	448,	464,
	480,	496,	512,	528,	544,	560,	576,	592,	608,	624,
	640,	656,	672,	688,	704,	720,	736,	752,	768,	784,
	800,	816,	832,	848,	864,	880,	896,	912,	928,	944,
	960,	976,	992,	1008,	1024,	1040,	1056,	1072,	1088,	1104,
	1120,	1136,	1152,	1168,	1184,	1200,	1216,	1232,	1248,	1264,
	1280,	1296,	1312,	1328,	1344,	1360,	1376,	1392,	1408,	1424,
	1440,	1456,	1472,	1488,	1504,	1520,	1536,	1552,	1568,	1584,
	1600,	1616,	1632,	1648,	1664,	1680,	1696,	1712,	1728,	1744,
	1760,	1776,	1792,	1808,	1824,	1840,	1856,	1872,	1888,	1904,
	1920,	1936,	1952,	1968,	1984,	2000,	2016,	2032,	2048,	2064,
	2080,	2096,	2112,	2128,	2144,	2160,	2176,	2192,	2208,	2224,
	2240,	2256,	2272,	2288,	2304,	2320,	2336,	2352,	2368,	2384,
	2400,	2416,	2432,	2448,	2464,	2480,	2496,	2512,	2528,	2544,
	2560,	2576,	2592,	2608,	2624,	2640,	2656,	2672,	2688,	2704,
	2720,	2736,	2752,	2768,	2784,	2800,	2816,	2832,	2848,	2864,
	2880,	2896,	2912,	2928,	2944,	2960,	2976,	2992,	3008,	3024,
	3040,	3056,	3072,	3088,	3104,	3120,	3136,	3152,	3168,	3184,
	3200,	3216,	3232,	3248,	3264,	3280,	3296,	3312,	3328,	3344,
	3360,	3376,	3392,	3408,	3424,	3440,	3456,	3472,	3488,	3504,
	3520,	3536,	3552,	3568,	3584,	3600,	3616,	3632,	3648,	3664,
	3680,	3696,	3712,	3728,	3744,	3760,	3776,	3792,	3808,	3824,
	3840,	3856,	3872,	3888,	3904,	3920,	3936,	3952,	3968,	3984,
	4000,	4016,	4032,	4048,	4064,	4080,	4095
};

static u32 gmp_lut_table_low32bit[9][9][9] = {
    {
        {0x00000000, 0x00000200, 0x00000400, 0x00000600, 0x00000800, 0x00000a00, 0x00000c00, 0x00000e00, 0x00000ff0, },
        {0x00200000, 0x00200200, 0x00200400, 0x00200600, 0x00200800, 0x00200a00, 0x00200c00, 0x00200e00, 0x00200ff0, },
        {0x00400000, 0x00400200, 0x00400400, 0x00400600, 0x00400800, 0x00400a00, 0x00400c00, 0x00400e00, 0x00400ff0, },
        {0x00600000, 0x00600200, 0x00600400, 0x00600600, 0x00600800, 0x00600a00, 0x00600c00, 0x00600e00, 0x00600ff0, },
        {0x00800000, 0x00800200, 0x00800400, 0x00800600, 0x00800800, 0x00800a00, 0x00800c00, 0x00800e00, 0x00800ff0, },
        {0x00a00000, 0x00a00200, 0x00a00400, 0x00a00600, 0x00a00800, 0x00a00a00, 0x00a00c00, 0x00a00e00, 0x00a00ff0, },
        {0x00c00000, 0x00c00200, 0x00c00400, 0x00c00600, 0x00c00800, 0x00c00a00, 0x00c00c00, 0x00c00e00, 0x00c00ff0, },
        {0x00e00000, 0x00e00200, 0x00e00400, 0x00e00600, 0x00e00800, 0x00e00a00, 0x00e00c00, 0x00e00e00, 0x00e00ff0, },
        {0x00ff0000, 0x00ff0200, 0x00ff0400, 0x00ff0600, 0x00ff0800, 0x00ff0a00, 0x00ff0c00, 0x00ff0e00, 0x00ff0ff0, },
    },
    {
        {0x00000000, 0x00000200, 0x00000400, 0x00000600, 0x00000800, 0x00000a00, 0x00000c00, 0x00000e00, 0x00000ff0, },
        {0x00200000, 0x00200200, 0x00200400, 0x00200600, 0x00200800, 0x00200a00, 0x00200c00, 0x00200e00, 0x00200ff0, },
        {0x00400000, 0x00400200, 0x00400400, 0x00400600, 0x00400800, 0x00400a00, 0x00400c00, 0x00400e00, 0x00400ff0, },
        {0x00600000, 0x00600200, 0x00600400, 0x00600600, 0x00600800, 0x00600a00, 0x00600c00, 0x00600e00, 0x00600ff0, },
        {0x00800000, 0x00800200, 0x00800400, 0x00800600, 0x00800800, 0x00800a00, 0x00800c00, 0x00800e00, 0x00800ff0, },
        {0x00a00000, 0x00a00200, 0x00a00400, 0x00a00600, 0x00a00800, 0x00a00a00, 0x00a00c00, 0x00a00e00, 0x00a00ff0, },
        {0x00c00000, 0x00c00200, 0x00c00400, 0x00c00600, 0x00c00800, 0x00c00a00, 0x00c00c00, 0x00c00e00, 0x00c00ff0, },
        {0x00e00000, 0x00e00200, 0x00e00400, 0x00e00600, 0x00e00800, 0x00e00a00, 0x00e00c00, 0x00e00e00, 0x00e00ff0, },
        {0x00ff0000, 0x00ff0200, 0x00ff0400, 0x00ff0600, 0x00ff0800, 0x00ff0a00, 0x00ff0c00, 0x00ff0e00, 0x00ff0ff0, },
    },
    {
        {0x00000000, 0x00000200, 0x00000400, 0x00000600, 0x00000800, 0x00000a00, 0x00000c00, 0x00000e00, 0x00000ff0, },
        {0x00200000, 0x00200200, 0x00200400, 0x00200600, 0x00200800, 0x00200a00, 0x00200c00, 0x00200e00, 0x00200ff0, },
        {0x00400000, 0x00400200, 0x00400400, 0x00400600, 0x00400800, 0x00400a00, 0x00400c00, 0x00400e00, 0x00400ff0, },
        {0x00600000, 0x00600200, 0x00600400, 0x00600600, 0x00600800, 0x00600a00, 0x00600c00, 0x00600e00, 0x00600ff0, },
        {0x00800000, 0x00800200, 0x00800400, 0x00800600, 0x00800800, 0x00800a00, 0x00800c00, 0x00800e00, 0x00800ff0, },
        {0x00a00000, 0x00a00200, 0x00a00400, 0x00a00600, 0x00a00800, 0x00a00a00, 0x00a00c00, 0x00a00e00, 0x00a00ff0, },
        {0x00c00000, 0x00c00200, 0x00c00400, 0x00c00600, 0x00c00800, 0x00c00a00, 0x00c00c00, 0x00c00e00, 0x00c00ff0, },
        {0x00e00000, 0x00e00200, 0x00e00400, 0x00e00600, 0x00e00800, 0x00e00a00, 0x00e00c00, 0x00e00e00, 0x00e00ff0, },
        {0x00ff0000, 0x00ff0200, 0x00ff0400, 0x00ff0600, 0x00ff0800, 0x00ff0a00, 0x00ff0c00, 0x00ff0e00, 0x00ff0ff0, },
    },
    {
        {0x00000000, 0x00000200, 0x00000400, 0x00000600, 0x00000800, 0x00000a00, 0x00000c00, 0x00000e00, 0x00000ff0, },
        {0x00200000, 0x00200200, 0x00200400, 0x00200600, 0x00200800, 0x00200a00, 0x00200c00, 0x00200e00, 0x00200ff0, },
        {0x00400000, 0x00400200, 0x00400400, 0x00400600, 0x00400800, 0x00400a00, 0x00400c00, 0x00400e00, 0x00400ff0, },
        {0x00600000, 0x00600200, 0x00600400, 0x00600600, 0x00600800, 0x00600a00, 0x00600c00, 0x00600e00, 0x00600ff0, },
        {0x00800000, 0x00800200, 0x00800400, 0x00800600, 0x00800800, 0x00800a00, 0x00800c00, 0x00800e00, 0x00800ff0, },
        {0x00a00000, 0x00a00200, 0x00a00400, 0x00a00600, 0x00a00800, 0x00a00a00, 0x00a00c00, 0x00a00e00, 0x00a00ff0, },
        {0x00c00000, 0x00c00200, 0x00c00400, 0x00c00600, 0x00c00800, 0x00c00a00, 0x00c00c00, 0x00c00e00, 0x00c00ff0, },
        {0x00e00000, 0x00e00200, 0x00e00400, 0x00e00600, 0x00e00800, 0x00e00a00, 0x00e00c00, 0x00e00e00, 0x00e00ff0, },
        {0x00ff0000, 0x00ff0200, 0x00ff0400, 0x00ff0600, 0x00ff0800, 0x00ff0a00, 0x00ff0c00, 0x00ff0e00, 0x00ff0ff0, },
    },
    {
        {0x00000000, 0x00000200, 0x00000400, 0x00000600, 0x00000800, 0x00000a00, 0x00000c00, 0x00000e00, 0x00000ff0, },
        {0x00200000, 0x00200200, 0x00200400, 0x00200600, 0x00200800, 0x00200a00, 0x00200c00, 0x00200e00, 0x00200ff0, },
        {0x00400000, 0x00400200, 0x00400400, 0x00400600, 0x00400800, 0x00400a00, 0x00400c00, 0x00400e00, 0x00400ff0, },
        {0x00600000, 0x00600200, 0x00600400, 0x00600600, 0x00600800, 0x00600a00, 0x00600c00, 0x00600e00, 0x00600ff0, },
        {0x00800000, 0x00800200, 0x00800400, 0x00800600, 0x00800800, 0x00800a00, 0x00800c00, 0x00800e00, 0x00800ff0, },
        {0x00a00000, 0x00a00200, 0x00a00400, 0x00a00600, 0x00a00800, 0x00a00a00, 0x00a00c00, 0x00a00e00, 0x00a00ff0, },
        {0x00c00000, 0x00c00200, 0x00c00400, 0x00c00600, 0x00c00800, 0x00c00a00, 0x00c00c00, 0x00c00e00, 0x00c00ff0, },
        {0x00e00000, 0x00e00200, 0x00e00400, 0x00e00600, 0x00e00800, 0x00e00a00, 0x00e00c00, 0x00e00e00, 0x00e00ff0, },
        {0x00ff0000, 0x00ff0200, 0x00ff0400, 0x00ff0600, 0x00ff0800, 0x00ff0a00, 0x00ff0c00, 0x00ff0e00, 0x00ff0ff0, },
    },
    {
        {0x00000000, 0x00000200, 0x00000400, 0x00000600, 0x00000800, 0x00000a00, 0x00000c00, 0x00000e00, 0x00000ff0, },
        {0x00200000, 0x00200200, 0x00200400, 0x00200600, 0x00200800, 0x00200a00, 0x00200c00, 0x00200e00, 0x00200ff0, },
        {0x00400000, 0x00400200, 0x00400400, 0x00400600, 0x00400800, 0x00400a00, 0x00400c00, 0x00400e00, 0x00400ff0, },
        {0x00600000, 0x00600200, 0x00600400, 0x00600600, 0x00600800, 0x00600a00, 0x00600c00, 0x00600e00, 0x00600ff0, },
        {0x00800000, 0x00800200, 0x00800400, 0x00800600, 0x00800800, 0x00800a00, 0x00800c00, 0x00800e00, 0x00800ff0, },
        {0x00a00000, 0x00a00200, 0x00a00400, 0x00a00600, 0x00a00800, 0x00a00a00, 0x00a00c00, 0x00a00e00, 0x00a00ff0, },
        {0x00c00000, 0x00c00200, 0x00c00400, 0x00c00600, 0x00c00800, 0x00c00a00, 0x00c00c00, 0x00c00e00, 0x00c00ff0, },
        {0x00e00000, 0x00e00200, 0x00e00400, 0x00e00600, 0x00e00800, 0x00e00a00, 0x00e00c00, 0x00e00e00, 0x00e00ff0, },
        {0x00ff0000, 0x00ff0200, 0x00ff0400, 0x00ff0600, 0x00ff0800, 0x00ff0a00, 0x00ff0c00, 0x00ff0e00, 0x00ff0ff0, },
    },
    {
        {0x00000000, 0x00000200, 0x00000400, 0x00000600, 0x00000800, 0x00000a00, 0x00000c00, 0x00000e00, 0x00000ff0, },
        {0x00200000, 0x00200200, 0x00200400, 0x00200600, 0x00200800, 0x00200a00, 0x00200c00, 0x00200e00, 0x00200ff0, },
        {0x00400000, 0x00400200, 0x00400400, 0x00400600, 0x00400800, 0x00400a00, 0x00400c00, 0x00400e00, 0x00400ff0, },
        {0x00600000, 0x00600200, 0x00600400, 0x00600600, 0x00600800, 0x00600a00, 0x00600c00, 0x00600e00, 0x00600ff0, },
        {0x00800000, 0x00800200, 0x00800400, 0x00800600, 0x00800800, 0x00800a00, 0x00800c00, 0x00800e00, 0x00800ff0, },
        {0x00a00000, 0x00a00200, 0x00a00400, 0x00a00600, 0x00a00800, 0x00a00a00, 0x00a00c00, 0x00a00e00, 0x00a00ff0, },
        {0x00c00000, 0x00c00200, 0x00c00400, 0x00c00600, 0x00c00800, 0x00c00a00, 0x00c00c00, 0x00c00e00, 0x00c00ff0, },
        {0x00e00000, 0x00e00200, 0x00e00400, 0x00e00600, 0x00e00800, 0x00e00a00, 0x00e00c00, 0x00e00e00, 0x00e00ff0, },
        {0x00ff0000, 0x00ff0200, 0x00ff0400, 0x00ff0600, 0x00ff0800, 0x00ff0a00, 0x00ff0c00, 0x00ff0e00, 0x00ff0ff0, },
    },
    {
        {0x00000000, 0x00000200, 0x00000400, 0x00000600, 0x00000800, 0x00000a00, 0x00000c00, 0x00000e00, 0x00000ff0, },
        {0x00200000, 0x00200200, 0x00200400, 0x00200600, 0x00200800, 0x00200a00, 0x00200c00, 0x00200e00, 0x00200ff0, },
        {0x00400000, 0x00400200, 0x00400400, 0x00400600, 0x00400800, 0x00400a00, 0x00400c00, 0x00400e00, 0x00400ff0, },
        {0x00600000, 0x00600200, 0x00600400, 0x00600600, 0x00600800, 0x00600a00, 0x00600c00, 0x00600e00, 0x00600ff0, },
        {0x00800000, 0x00800200, 0x00800400, 0x00800600, 0x00800800, 0x00800a00, 0x00800c00, 0x00800e00, 0x00800ff0, },
        {0x00a00000, 0x00a00200, 0x00a00400, 0x00a00600, 0x00a00800, 0x00a00a00, 0x00a00c00, 0x00a00e00, 0x00a00ff0, },
        {0x00c00000, 0x00c00200, 0x00c00400, 0x00c00600, 0x00c00800, 0x00c00a00, 0x00c00c00, 0x00c00e00, 0x00c00ff0, },
        {0x00e00000, 0x00e00200, 0x00e00400, 0x00e00600, 0x00e00800, 0x00e00a00, 0x00e00c00, 0x00e00e00, 0x00e00ff0, },
        {0x00ff0000, 0x00ff0200, 0x00ff0400, 0x00ff0600, 0x00ff0800, 0x00ff0a00, 0x00ff0c00, 0x00ff0e00, 0x00ff0ff0, },
    },
    {
        {0xf0000000, 0xf0000200, 0xf0000400, 0xf0000600, 0xf0000800, 0xf0000a00, 0xf0000c00, 0xf0000e00, 0xf0000ff0, },
        {0xf0200000, 0xf0200200, 0xf0200400, 0xf0200600, 0xf0200800, 0xf0200a00, 0xf0200c00, 0xf0200e00, 0xf0200ff0, },
        {0xf0400000, 0xf0400200, 0xf0400400, 0xf0400600, 0xf0400800, 0xf0400a00, 0xf0400c00, 0xf0400e00, 0xf0400ff0, },
        {0xf0600000, 0xf0600200, 0xf0600400, 0xf0600600, 0xf0600800, 0xf0600a00, 0xf0600c00, 0xf0600e00, 0xf0600ff0, },
        {0xf0800000, 0xf0800200, 0xf0800400, 0xf0800600, 0xf0800800, 0xf0800a00, 0xf0800c00, 0xf0800e00, 0xf0800ff0, },
        {0xf0a00000, 0xf0a00200, 0xf0a00400, 0xf0a00600, 0xf0a00800, 0xf0a00a00, 0xf0a00c00, 0xf0a00e00, 0xf0a00ff0, },
        {0xf0c00000, 0xf0c00200, 0xf0c00400, 0xf0c00600, 0xf0c00800, 0xf0c00a00, 0xf0c00c00, 0xf0c00e00, 0xf0c00ff0, },
        {0xf0e00000, 0xf0e00200, 0xf0e00400, 0xf0e00600, 0xf0e00800, 0xf0e00a00, 0xf0e00c00, 0xf0e00e00, 0xf0e00ff0, },
        {0xf0ff0000, 0xf0ff0200, 0xf0ff0400, 0xf0ff0600, 0xf0ff0800, 0xf0ff0a00, 0xf0ff0c00, 0xf0ff0e00, 0xf0ff0ff0, },
    },
};

static u32 gmp_lut_table_high4bit[9][9][9] = {
    {
        {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, },
        {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, },
        {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, },
        {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, },
        {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, },
        {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, },
        {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, },
        {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, },
        {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, },
    },
    {
        {0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, },
        {0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, },
        {0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, },
        {0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, },
        {0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, },
        {0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, },
        {0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, },
        {0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, },
        {0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, },
    },
    {
        {0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, },
        {0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, },
        {0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, },
        {0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, },
        {0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, },
        {0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, },
        {0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, },
        {0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, },
        {0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, },
    },
    {
        {0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, },
        {0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, },
        {0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, },
        {0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, },
        {0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, },
        {0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, },
        {0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, },
        {0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, },
        {0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, },
    },
    {
        {0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, },
        {0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, },
        {0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, },
        {0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, },
        {0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, },
        {0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, },
        {0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, },
        {0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, },
        {0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, },
    },
    {
        {0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, },
        {0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, },
        {0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, },
        {0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, },
        {0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, },
        {0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, },
        {0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, },
        {0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, },
        {0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, },
    },
    {
        {0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, },
        {0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, },
        {0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, },
        {0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, },
        {0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, },
        {0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, },
        {0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, },
        {0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, },
        {0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, },
    },
    {
        {0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, },
        {0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, },
        {0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, },
        {0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, },
        {0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, },
        {0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, },
        {0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, },
        {0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, },
        {0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, },
    },
    {
        {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, },
        {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, },
        {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, },
        {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, },
        {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, },
        {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, },
        {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, },
        {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, },
        {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, },
    },
};

static u32 xcc_table[12] = {0x0, 0x8000, 0x0,0x0,0x0,0x0,0x8000,0x0,0x0,0x0,0x0,0x8000,};

static struct dsi_cmd_desc* sharp_display_bist_check_cmds[] = {
	sharp_display_bist_check_cmds1,
	sharp_display_bist_check_cmds2,
	sharp_display_bist_check_cmds3,
	sharp_display_bist_check_cmds4,
	sharp_display_bist_check_cmds5,
	sharp_display_bist_check_cmds6,
	sharp_display_bist_check_cmds7,
	sharp_display_bist_check_cmds8,
};

static int bist_check_cmds_size[] = {
	ARRAY_SIZE(sharp_display_bist_check_cmds1),
	ARRAY_SIZE(sharp_display_bist_check_cmds2),
	ARRAY_SIZE(sharp_display_bist_check_cmds3),
	ARRAY_SIZE(sharp_display_bist_check_cmds4),
	ARRAY_SIZE(sharp_display_bist_check_cmds5),
	ARRAY_SIZE(sharp_display_bist_check_cmds6),
	ARRAY_SIZE(sharp_display_bist_check_cmds7),
	ARRAY_SIZE(sharp_display_bist_check_cmds8),
};
#endif
