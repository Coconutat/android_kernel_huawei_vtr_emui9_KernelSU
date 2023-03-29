/* Copyright (c) 2008-2019, Huawei Tech. Co., Ltd. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 and
* only version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
* GNU General Public License for more details.
*
*/
#ifndef DP_SOURCE_SWITCH_H
#define DP_SOURCE_SWITCH_H

#define DTS_DP_SOURCE_SWITCH "huawei,dp_source_switch"
#define DTS_DP_DEFAULT_SOURCE_MODE "dp_default_source_mode"
#define HWLOG_TAG "DP_SOURCE_SWITCH"
#define RETURN_ERR -1

/*resolution with*/
#define WITH_640   640;
#define WITH_800   600;
#define WITH_1024  1024;
#define WITH_1280  1280;
#define WITH_1360  1360;
#define WITH_1400  1400;
#define WITH_1440  1440;
#define WITH_1600  1600;
#define WITH_1680  1680;
#define WITH_1792  1792;
#define WITH_1920  1920;
#define WITH_2048  2048;
#define WITH_2560  2560;
#define WITH_3840  3840;

/*resolution high*/
#define HIGH_480    480;
#define HIGH_576    576;
#define HIGH_600    600;
#define HIGH_768    768;
#define HIGH_800    800;
#define HIGH_960    960;
#define HIGH_1024   1024;
#define HIGH_1050   1050;
#define HIGH_1080   1080;
#define HIGH_1200   1200;
#define HIGH_1344   1344;
#define HIGH_1536   1636;
#define HIGH_1600   1600;
#define HIGH_2160   2160;

/*fps*/
#define FPS_30   30;
#define FPS_60   60;
#define FPS_59   59;
#define FPS_50   50;
#define FPS_75   75;
/*vesa Id*/
#define 	VESA_ID_1 		1
#define 	VESA_ID_2		2
#define 	VESA_ID_3		3
#define 	VESA_ID_4		4
#define 	VESA_ID_5		5
#define 	VESA_ID_6		6
#define 	VESA_ID_7		7
#define 	VESA_ID_8		8
#define 	VESA_ID_9		9
#define 	VESA_ID_10		10
#define 	VESA_ID_11		11
#define 	VESA_ID_12		12
#define 	VESA_ID_13		13
#define 	VESA_ID_14		14
#define 	VESA_ID_15		15
#define 	VESA_ID_16		16
#define 	VESA_ID_17		17
#define 	VESA_ID_18		18
#define 	VESA_ID_19		19
#define 	VESA_ID_20		20
#define 	VESA_ID_21		21
#define 	VESA_ID_22		22
#define 	VESA_ID_23		23
#define 	VESA_ID_24		24
#define 	VESA_ID_25		25
#define 	VESA_ID_26		26
#define 	VESA_ID_27		27
#define 	VESA_ID_28		28
#define 	VESA_ID_29		29
#define 	VESA_ID_30		30
#define 	VESA_ID_31		31
#define 	VESA_ID_32		32
#define 	VESA_ID_33		33
#define 	VESA_ID_34		34
#define 	VESA_ID_35		35
#define 	VESA_ID_36		36
#define 	VESA_ID_37		37
#define 	VESA_ID_38		38
#define 	VESA_ID_39		39
#define 	VESA_ID_40		40
#define 	VESA_ID_41		41
#define 	VESA_ID_42		42
#define 	VESA_ID_43		43
#define 	VESA_ID_44		44
#define 	VESA_ID_45		45
#define 	VESA_ID_46		46
#define 	VESA_ID_47		47
#define 	VESA_ID_48		48
#define 	VESA_ID_49		49
#define 	VESA_ID_50		50
#define 	VESA_ID_51		51
#define 	VESA_ID_52		52
#define 	VESA_ID_53		53
#define 	VESA_ID_54		54
#define 	VESA_ID_55		55
#define 	VESA_ID_56		56
#define 	VESA_ID_57		57
#define 	VESA_ID_58		58
#define 	VESA_ID_59		59
#define 	VESA_ID_60		60
#define 	VESA_ID_61		61
#define 	VESA_ID_62		62
#define 	VESA_ID_63		63
#define 	VESA_ID_64		64
#define 	VESA_ID_65		65
#define 	VESA_ID_66		66
#define 	VESA_ID_67		67
#define 	VESA_ID_68		68
#define 	VESA_ID_69		69
#define 	VESA_ID_70		70
#define 	VESA_ID_71		71
#define 	VESA_ID_72		72
#define 	VESA_ID_73		73
#define 	VESA_ID_74		74
#define 	VESA_ID_75		75
#define 	VESA_ID_76		76
#define 	VESA_ID_77		77
#define 	VESA_ID_78		78
#define 	VESA_ID_79		79
#define 	VESA_ID_80		80
#define 	VESA_ID_81		81
#define 	VESA_ID_82		82
#define 	VESA_ID_83		83
#define 	VESA_ID_84		84
#define 	VESA_ID_85		85
#define 	VESA_ID_86		86
#define 	VESA_ID_87		87
#define 	VESA_ID_88		88
#define 	VESA_ID_89		89
#define 	VESA_ID_90		90
#define 	VESA_ID_91		91
#define 	VESA_ID_92		92
#define 	VESA_ID_93		93
#define 	VESA_ID_94		94
#define 	VESA_ID_95		95
#define 	VESA_ID_96		96
#define 	VESA_ID_97		97
#define 	VESA_ID_98		98
#define 	VESA_ID_99		99

enum dp_event_type
{
	DP_LINK_STATE_BAD = 0,
	DP_LINK_STATE_GOOD = 1
};

enum dp_resolution_type
{
	RESOLUTION_640_480_60FPS = 1,
	RESOLUTION_800_600_60FPS = 2,
	RESOLUTION_800_600_75FPS = 3,
	RESOLUTION_1024_768_60FPS = 4,
	RESOLUTION_1280_768_60FPS_CVT = 5,
	RESOLUTION_1280_768_60FPS =  6,
	RESOLUTION_1280_800_60FPS_CVT = 7,
	RESOLUTION_1280_800_60FPS = 8,
	RESOLUTION_1280_960_60FPS = 9,
	RESOLUTION_1280_1024_60FPS = 10,
	RESOLUTION_1360_768_60FPS = 11,
	RESOLUTION_1400_1050_60FPS = 12,
	RESOLUTION_1600_1200_60FPS_CVT = 13,
	RESOLUTION_1600_1200_60FPS = 14,
	RESOLUTION_1680_1050_60FPS_CVT = 15,
	RESOLUTION_1680_1050_60FPS = 16,
	RESOLUTION_1792_1344_60FPS = 17,
	RESOLUTION_1920_1080_60FPS = 18,
	RESOLUTION_1920_1200_60FPS = 19,
	RESOLUTION_2048_1536_60FPS = 20,
	RESOLUTION_2560_1600_60FPS_CVT = 21,
	RESOLUTION_2560_1600_60FPS = 22,
	RESOLUTION_1440_480_59FPS = 23,
	RESOLUTION_1440_576_50FPS = 24,
	RESOLUTION_3840_2160_60FPS = 25,
	RESOLUTION_1920_1080_60FPS_CEA = 26,
	RESOLUTION_3840_2160_30FPS = 27,
	RESOLUTION_UNDEF = 28
};

enum dp_source_mode
{
	DIFF_SOURCE = 0,
	SAME_SOURCE =1,
	VR_MODE =2,
	SOURCE_UNDEF = 255
};

enum dp_lcd_power_type
{
	LCD_POWER_OFF = 0,
	LCD_POWER_ON = 1
};

enum dp_video_format_type {
	VCEA = 0,
	CVT = 1,
	DMT = 2
};

/*choose VR or PC Mode*/
enum dp_external_display_type {
	EXT_PC_MODE = 0,
	EXT_VR_MODE = 1,
	EXT_UNDEF = 2
};

struct dp_source_resolution {
	uint16_t m_with;
	uint16_t m_high;
	uint16_t m_fps;
	uint8_t m_vesa_id;
};

struct dp_externel_info {
	uint16_t m_width;
	uint16_t m_high;
	uint16_t m_fps;
};

struct dp_source_data {
    struct class*    class;
	struct dp_source_resolution m_source_resolution;
	struct dp_externel_info m_externel_info;
	enum dp_resolution_type m_resolution_type;
	enum dp_source_mode m_source_mode;
	enum dp_video_format_type m_video_format_type;
	enum dp_external_display_type m_external_display_type;
	enum dp_lcd_power_type m_lcd_power_type;

	bool m_parse_dts_flag;
};

int dp_source_mode_parse_dts(void);
//static int dp_source_setup_sysfs(struct device *dev);
void dp_send_event(enum dp_event_type event);


#endif /*DP_SOURCE_SWITCH_H*/

