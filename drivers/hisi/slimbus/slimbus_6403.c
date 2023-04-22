/*
 *	slimbus is a kernel driver which is used to manager SLIMbus devices
 *	Copyright (C) 2014	Hisilicon

 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.

 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.

 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/delay.h>

#include "csmi.h"
#include "slimbus_drv.h"
#include "slimbus.h"
#include "slimbus_64xx.h"
#include <dsm/dsm_pub.h>

/*lint -e750 -e730 -e785 -e574*/
#define LOG_TAG "Slimbus_6403"

/* segment length */
static uint16_t sl_table[SLIMBUS_SCENE_CONFIG_MAX][SLIMBUS_TRACK_MAX][SLIMBUS_CHANNELS_MAX] = {
			/* 6.144M FPGA*/
			{
				{6, 6},                     /* audio playback */
				{4, 4},                     /* audio capture */
				{4, 4},                     /* voice down */
				{4, 4, 4, 4},               /* voice up */
				{5},                        /* image download */
				{0, 0},                     /* EC_REF */
				{0},                        /* sound trigger */
				{0},                        /* debug */
				{0, 0},                     /* direct play*/
				{0, 0},                     /* fast play*/
			},
			/* 6.144M play*/
			{
				{6, 6},                                         /* audio playback */
				{0, 0},                                         /* audio capture */
				{0, 0},                                         /* voice down */
				{0, 0, 0, 0},                                   /* voice up */
				{5},                                            /* image download */
				{6, 6},                                         /* EC_REF */
				{0},                                            /* sound trigger */
				{0},                                            /* debug */
				{0, 0},                                         /* direct play*/
				{0, 0},                                         /* fast play*/

			},
			/* 6.144M call*/
			{
				{0, 0},                                         /* audio playback */
				{0, 0},                                         /* audio capture */
				{6, 6},                                         /* voice down */
				{6, 6, 6, 6},                                   /* voice up */
				{5},                                            /* image download */
				{0, 0},                                         /* EC_REF */
				{0},                                            /* sound trigger */
				{0},                                            /* debug */
				{0, 0},                                         /* direct play*/
				{0, 0},                                         /* fast play*/
			},
			/* 24.576M normal*/
			{
				{6, 6},                                         /* audio playback */
				{6, 6, 6, 6},                                   /* audio capture */
				{6, 6},                                         /* voice down */
				{6, 6, 6, 6},                                   /* voice up */
				{5},                                            /* image download */
				{6, 6},                                         /* EC_REF */
				{8},                                            /* sound trigger */
				{8},                                            /* debug */
				{0, 0},                                         /* direct play*/
				{0, 0},                                         /* fast play*/
			},
			/* 24.576M anc call*/
			{
				{6, 6},                                         /* audio playback */
				{0, 0},                                         /* audio capture */
				{6, 6},                                         /* voice down */
				{6, 6, 6, 6},                                   /* voice up */
				{5},                                            /* image download */
				{0, 0},                                         /* EC_REF */
				{0},                                            /* sound trigger */
				{8, 8},                                         /* debug */
				{0, 0},                                         /* direct play*/
				{0, 0},                                         /* fast play*/
			},
			/* 24.576M hires play*/
			{
				{0, 0},                                         /* audio playback */
				{6, 6, 6, 6},                                   /* audio capture */
				{0, 0},                                         /* voice down */
				{0, 0, 0, 0},                                   /* voice up */
				{5},                                            /* image download */
				{6, 6},                                         /* EC_REF */
				{8},                                            /* sound trigger */
				{0},                                            /* debug */
				{6, 6},                                         /* direct play */
				{0, 0},                                         /* fast play*/
			},
			/* image load */
			{
				{0, 0},                                          /* audio playback */
				{0, 0, 0, 0},                                    /* audio capture */
				{0, 0},                                          /* voice down */
				{0, 0, 0, 0},                                    /* voice up */
				{5},                                             /* image download */
				{0, 0},                                          /* EC_REF */
				{0},                                             /* sound trigger */
				{0},                                             /* debug */
				{0, 0},                                          /* direct play */
				{0, 0},                                          /* fast play*/
			},
			/* 24.576M fast play*/
			{
				{0, 0},                                          /* audio playback */
				{0, 0, 0, 0},                                    /* audio capture */
				{0, 0},                                          /* voice down */
				{0, 0, 0, 0},                                    /* voice up */
				{5},                                             /* image download */
				{6, 6},                                          /* EC_REF */
				{0},                                             /* sound trigger */
				{0},                                             /* debug */
				{0, 0},                                          /* direct play */
				{6, 6},                                          /* fast play*/
			},
			/* 24.576M fast play & record*/
			{
				{6, 6},                                          /* audio playback */
				{6, 6, 6, 6},                                    /* audio capture */
				{0, 0},                                          /* voice down */
				{0, 0, 0, 0},                                    /* voice up */
				{5},                                             /* image download */
				{6, 6},                                          /* EC_REF */
				{0},                                             /* sound trigger */
				{0},                                             /* debug */
				{6, 6},                                          /* direct play */
				{6, 6},                                          /* fast play*/
			},
			/* 24.576M fast play & st*/
			{
				{6, 6},                                          /* audio playback */
				{0, 0, 0, 0},                                    /* audio capture */
				{0, 0},                                          /* voice down */
				{0, 0, 0, 0},                                    /* voice up */
				{5},                                             /* image download */
				{6, 6},                                          /* EC_REF */
				{8},                                             /* sound trigger */
				{0},                                             /* debug */
				{6, 6},                                          /* direct play */
				{6, 6},                                          /* fast play*/
			},
			/* 12.288M call*/
			{
				{6, 6},                                         /* audio playback */
				{0, 0},                                         /* audio capture */
				{6, 6},                                         /* voice down */
				{6, 6, 6, 6},                                   /* voice up */
				{5},                                            /* image download */
				{6, 6},                                         /* EC_REF */
				{0},                                            /* sound trigger */
				{0},                                            /* debug */
				{0, 0},                                         /* direct play*/
				{0, 0},                                         /* fast play*/
			},
			/* 6.144M soundtrigger enhance*/
			{
				{6, 6},                                         /* audio playback */
				{0, 0},                                         /* audio capture */
				{0, 0},                                         /* voice down */
				{0, 0, 0, 0},                                   /* voice up */
				{5},                                            /* image download */
				{6, 6},                                         /* EC_REF */
				{4, 4},                                         /* sound trigger */
				{0},                                            /* debug */
				{0, 0},                                         /* direct play*/
				{0, 0},                                         /* fast play*/
			},
};

static uint16_t sl_soundtrigger_16k[SLIMBUS_SCENE_CONFIG_MAX][SLIMBUS_CHANNELS_MAX] = {
			{0},                                                    /* 6.144M fpga */
			{0},                                                    /* 6.144M play */
			{0},                                                    /* 6.144M call */
			{4, 4},                                                 /* 24.576M normal */
			{0},                                                    /* 24.576M anc call */
			{4, 4},                                                 /* 24.576M direct play */
			{0},                                                    /* image */
			{0},                                                    /* 6.144M fast play */
			{0},                                                    /* 24.576M fast play & rec*/
			{4, 4},                                                 /* 24.576M fast play & st*/
			{0},                                                    /* 12.288M call */
			{4, 4},                                                 /* 6.144M soundtrigger enhance */
};

/* presence rate */
static slimbus_presence_rate_t pr_table[SLIMBUS_TRACK_MAX] = {
			SLIMBUS_PR_48K, SLIMBUS_PR_48K,
			SLIMBUS_PR_8K, SLIMBUS_PR_8K,
			SLIMBUS_PR_768K, SLIMBUS_PR_48K,
			SLIMBUS_PR_192K, SLIMBUS_PR_192K,
			SLIMBUS_PR_192K, SLIMBUS_PR_48K,
};

/* channel index */
static uint8_t cn_table[SLIMBUS_TRACK_MAX][SLIMBUS_CHANNELS_MAX] = {
			{AUDIO_PLAY_CHANNEL_LEFT, AUDIO_PLAY_CHANNEL_RIGHT},
			{AUDIO_CAPTURE_CHANNEL_LEFT, AUDIO_CAPTURE_CHANNEL_RIGHT, VOICE_UP_CHANNEL_MIC1, VOICE_UP_CHANNEL_MIC2},
			{VOICE_DOWN_CHANNEL_LEFT, VOICE_DOWN_CHANNEL_RIGHT},
			{VOICE_UP_CHANNEL_MIC3, VOICE_UP_CHANNEL_MIC4, VOICE_UP_CHANNEL_MIC1, VOICE_UP_CHANNEL_MIC2},
			{IMAGE_DOWNLOAD},
			{VOICE_ECREF, AUDIO_ECREF},
			{SOUND_TRIGGER_CHANNEL_LEFT, SOUND_TRIGGER_CHANNEL_RIGHT},
			{DEBUG_LEFT, DEBUG_RIGHT},
			{AUDIO_PLAY_CHANNEL_LEFT, AUDIO_PLAY_CHANNEL_RIGHT},
			{VOICE_DOWN_CHANNEL_LEFT, VOICE_DOWN_CHANNEL_RIGHT},
};


/* source port number */
static uint8_t source_pn_table[SLIMBUS_TRACK_MAX][SLIMBUS_CHANNELS_MAX] = {
			{AUDIO_PLAY_SOC_LEFT_PORT, AUDIO_PLAY_SOC_RIGHT_PORT},
			{AUDIO_CAPTURE_64XX_LEFT_PORT, AUDIO_CAPTURE_64XX_RIGHT_PORT, VOICE_UP_64XX_MIC3_PORT, VOICE_UP_64XX_MIC4_PORT},
			{VOICE_DOWN_SOC_LEFT_PORT, VOICE_DOWN_SOC_RIGHT_PORT},
			{VOICE_UP_64XX_MIC1_PORT, VOICE_UP_64XX_MIC2_PORT, VOICE_UP_64XX_MIC3_PORT, VOICE_UP_64XX_MIC4_PORT},
			{IMAGE_DOWNLOAD_SOC_PORT},
			{VOICE_64XX_ECREF_PORT, AUDIO_64XX_ECREF_PORT},
			{VOICE_UP_64XX_MIC1_PORT, VOICE_UP_64XX_MIC2_PORT},
			{BT_CAPTURE_64XX_LEFT_PORT, BT_CAPTURE_64XX_RIGHT_PORT},
			{AUDIO_PLAY_SOC_LEFT_PORT, AUDIO_PLAY_SOC_RIGHT_PORT},
			{VOICE_DOWN_SOC_LEFT_PORT, VOICE_DOWN_SOC_RIGHT_PORT},
};

/* sink port number */
static uint8_t sink_pn_table[SLIMBUS_TRACK_MAX][SLIMBUS_CHANNELS_MAX] = {
			{AUDIO_PLAY_64XX_LEFT_PORT, AUDIO_PLAY_64XX_RIGHT_PORT},
			{AUDIO_CAPTURE_SOC_LEFT_PORT, AUDIO_CAPTURE_SOC_RIGHT_PORT, VOICE_UP_SOC_MIC3_PORT, VOICE_UP_SOC_MIC4_PORT},
			{VOICE_DOWN_64XX_LEFT_PORT, VOICE_DOWN_64XX_RIGHT_PORT},
			{VOICE_UP_SOC_MIC1_PORT, VOICE_UP_SOC_MIC2_PORT, VOICE_UP_SOC_MIC3_PORT, VOICE_UP_SOC_MIC4_PORT},
			{IMAGE_DOWNLOAD_64XX_PORT},
			{VOICE_SOC_ECREF_PORT, AUDIO_SOC_ECREF_PORT},
			{VOICE_UP_SOC_MIC1_PORT, VOICE_UP_SOC_MIC2_PORT},
			{BT_CAPTURE_SOC_LEFT_PORT, BT_CAPTURE_SOC_RIGHT_PORT},
			{AUDIO_PLAY_64XX_LEFT_PORT, AUDIO_PLAY_64XX_RIGHT_PORT},
			{IMAGE_DOWNLOAD_64XX_PORT, SUPER_PLAY_64XX_PORT},
};

/* segment distribution */
static uint16_t sd_table[SLIMBUS_SCENE_CONFIG_MAX][SLIMBUS_TRACK_MAX][SLIMBUS_CHANNELS_MAX] ={
			/* 6.144M FPGA*/
			{
				{0xc24, 0xc2a},                                 /* audio playback */
				{0xc30, 0xc34},                                 /* audio capture */
				{0x58, 0x5c},                                   /* voice down 8k */
				{0x78, 0x7c, 0x458, 0x45C},                     /* voice up 8k */
				{0xC0B},                                        /* image download */
				{0, 0},                                         /* EC_REF */
				{0},                                            /* sound trigger 192K*/
				{0},                                            /* debug 192K*/
				{0},                                            /* direct play 192k*/
				{0},                                            /* fast play 48k*/
			},
			/* 6.144M play */
			{
				{0xC28, 0xC2E},                                 /* audio playback */
				{0, 0},                                         /* audio capture */
				{0, 0},                                         /* voice down 8k */
				{0, 0, 0, 0},                                   /* voice up 8k */
				{0xC0B},                                        /* image download */
				{0xC34, 0xC3A},                                 /* EC_REF */
				{0},                                            /* sound trigger 192K*/
				{0},                                            /* debug 192K*/
				{0},                                            /* direct play 192k*/
				{0},                                            /* fast play 48k*/
			},
			/* 6.144M call*/
			{
				{0, 0},                                         /* audio playback */
				{0, 0},                                         /* audio capture */
				{0x46, 0x5E},                                   /* voice down 8k */
				{0x4C, 0x52, 0x64, 0x6A},                       /* voice up 8k */
				{0xC0B},                                        /* image download */
				{0, 0},                                         /* EC_REF */
				{0},                                            /* sound trigger 192K*/
				{0},                                            /* debug 192K*/
				{0},                                            /* direct play 192k*/
				{0},                                            /* fast play 48k*/
			},
			/* 24.576M normal */
			{
				{0xCC1, 0xCE1},                                 /* audio playback */
				{0xC87, 0xCA7, 0xCC7, 0xCE7},                   /* audio capture */
				{0x10D, 0x12D},                                 /* voice down 8k */
				{0x14D, 0x16D, 0x18D, 0x1AD},                   /* voice up 8k */
				{0xC0B},                                        /* image download */
				{0xC81, 0xCA1},                                 /* EC_REF */
				{0xC33},                                        /* sound trigger 192K*/
				{0xC33},                                        /* debug 192K*/
				{0},                                            /* direct play 192k*/
				{0},                                            /* fast play 48k*/
			},
			/*24.576M anc call*/
			{
				{0xCC1, 0xCE1},                                 /* audio playback */
				{0, 0, 0, 0},                                   /* audio capture */
				{0x107, 0x127},                                 /* voice down 8k */
				{0x147, 0x167, 0x187, 0x1A7},                   /* voice up 8k */
				{0xC0B},                                        /* image download */
				{0, 0},                                         /* EC_REF */
				{0},                                            /* sound trigger 192K*/
				{0xC2D, 0xC35},                                 /* debug 192K*/
				{0},                                            /* direct play 192k*/
				{0},                                            /* fast play 48k*/
			},
			/*24.576M direct play*/
			{
				{0, 0},                                         /* audio playback */
				{0xC8A, 0xCAA, 0xCCA, 0xCEA},                   /* audio capture */
				{0, 0},                                         /* voice down 8k */
				{0, 0, 0, 0},                                   /* voice up 8k */
				{0xC0B},                                        /* image download */
				{0xC82, 0xCA2},                                 /* EC_REF */
				{0xC22},                                        /* sound trigger 192K */
				{0},                                            /* debug 192K */
				{0xC30,0xC36},                                  /* direct play 192k */
				{0},                                            /* fast play 48k*/
			},
			/*image load*/
			{
				{0, 0},                                         /* audio playback */
				{0, 0, 0, 0},                                   /* audio capture */
				{0, 0},                                         /* voice down 8k */
				{0, 0, 0, 0},                                   /* voice up 8k */
				{0xC0B},                                        /* image download */
				{0, 0},                                         /* EC_REF */
				{0},                                            /* sound trigger 192K*/
				{0, 0},                                         /* debug 192K*/
				{0},                                            /* direct play 192k*/
				{0},                                            /* fast play 48k*/
			},
			/*6.144M fast play*/
			{
				{0, 0},                                         /* audio playback */
				{0, 0, 0, 0},                                   /* audio capture */
				{0, 0},                                         /* voice down 8k */
				{0, 0, 0, 0},                                   /* voice up 8k */
				{0xC0B},                                        /* image download */
				{0xC34, 0xC3A},                                 /* EC_REF */
				{0},                                            /* sound trigger 192K */
				{0},                                            /* debug 192K */
				{0, 0},                                         /* direct play 192k */
				{0xC28, 0xC2E},                                 /* fast play 48k*/
			},
			/*24.576M fast play & rec*/
			{
				{0xCC2, 0xCE2},                                 /* audio playback */
				{0xC88, 0xCA8, 0xCC8, 0xCE8},                   /* audio capture */
				{0, 0},                                         /* voice down 8k */
				{0, 0, 0, 0},                                   /* voice up 8k */
				{0xC0B},                                        /* image download */
				{0xC82, 0xCA2},                                 /* EC_REF */
				{0},                                            /* sound trigger 192K */
				{0},                                            /* debug 192K */
				{0xC2E, 0xC34},                                 /* direct play 192k */
				{0xC9A, 0xCBA},                                 /* fast play 48k*/
			},
			/*24.576M fast play & st*/
			{
				{0xCC4, 0xCE4},                                 /* audio playback */
				{0, 0, 0, 0},                                   /* audio capture */
				{0, 0},                                         /* voice down 8k */
				{0, 0, 0, 0},                                   /* voice up 8k */
				{0xC0B},                                        /* image download */
				{0xC84, 0xCA4},                                 /* EC_REF */
				{0xC38},                                        /* sound trigger 192K */
				{0},                                            /* debug 192K */
				{0xC30,0xC36},                                  /* direct play 192k */
				{0xC8A, 0xCAA},                                 /* fast play 48k*/
			},
			/* 12.288M call*/
			{
				{0xC48, 0xC68},                                 /* audio playback */
				{0, 0},                                         /* audio capture */
				{0xCE, 0xD4},                                   /* voice down 8k */
				{0x8E, 0x94, 0xAE, 0xB4},                       /* voice up 8k */
				{0xC0B},                                        /* image download */
				{0xC5A, 0xC7A},                                 /* EC_REF */
				{0},                                            /* sound trigger 192K*/
				{0},                                            /* debug 192K*/
				{0},                                            /* direct play 192k*/
				{0},                                            /* fast play 48k*/
			},
			/* 6.144M soundtrigger enhance*/
			{
				{0xC28, 0xC2E},                                 /* audio playback */
				{0, 0},                                         /* audio capture */
				{0, 0},                                         /* voice down */
				{0, 0, 0, 0},                                   /* voice up */
				{0xC0B},                                        /* image download */
				{0xC34, 0xC3A},                                 /* EC_REF */
				{0x24, 0x424},                                  /* sound trigger */
				{0},                                            /* debug */
				{0, 0},                                         /* direct play*/
				{0, 0},                                         /* fast play*/
			},
};

static uint16_t sd_voice_down_16k[SLIMBUS_SCENE_CONFIG_MAX][SLIMBUS_CHANNELS_MAX] = {
			{0x38, 0x3C},                                       /* 6.144M FPGA voice down 16k */
			{0, 0},                                             /* 6.144M play */
			{0x26, 0x3E},                                       /* 6.144M call */
			{0x8D, 0xAD},                                       /* 24.576M normal */
			{0x87, 0xA7},                                       /* 24.576M anc call */
			{0, 0},                                             /* 24.576M direct play */
			{0, 0},                                             /* image */
			{0, 0},                                             /* 6.144M fast play */
			{0, 0},                                             /* 24.576M fast play & rec*/
			{0, 0},                                             /* 24.576M fast play & st*/
			{0x44E, 0x454},                                     /* 12.288M call */
			{0, 0},                                             /* 6.144M soundtrigger enhance */
};

static uint16_t sd_voice_up_16k[SLIMBUS_SCENE_CONFIG_MAX][SLIMBUS_CHANNELS_MAX] = {
			{0x438, 0x43c, 0x838, 0x83c},                       /* 6.144M FPGA voice up 16k */
			{0, 0, 0, 0},                                       /* 6.144M play */
			{0x2C, 0x32, 0x424, 0x42A},                         /* 6.144M call */
			{0xCD, 0xED, 0x48D, 0x4AD},                         /* 24.576M normal */
			{0xC7, 0xE7, 0x487, 0x4A7},                         /* 24.576M anc call */
			{0, 0, 0, 0},                                       /* 24.576M direct play */
			{0, 0, 0, 0},                                       /* image */
			{0, 0, 0, 0},                                       /* 6.144M fast play */
			{0, 0, 0, 0},                                       /* 24.576M fast play & rec*/
			{0, 0, 0, 0},                                       /* 24.576M fast play & st*/
			{0x4E, 0x34, 0x6E, 0x74},                           /* 12.288M call */
			{0, 0, 0, 0},                                       /* 6.144M soundtrigger enhance */
};

static uint16_t sd_voice_down_32k[SLIMBUS_SCENE_CONFIG_MAX][SLIMBUS_CHANNELS_MAX] = {
			{0, 0},                                             /* 6.144M FPGA voice down 32k */
			{0, 0},                                             /* 6.144M play */
			{0x16, 0x41E},                                      /* 6.144M call */
			{0x4D, 0x6D},                                       /* 24.576M normal */
			{0x47, 0x67},                                       /* 24.576M anc call */
			{0, 0},                                             /* 24.576M direct play */
			{0, 0},                                             /* image */
			{0, 0},                                             /* 6.144M fast play */
			{0, 0},                                             /* 24.576M fast play & rec*/
			{0, 0},                                             /* 24.576M fast play & st*/
			{0x82E, 0x834},                                     /* 12.288M call */
			{0, 0},                                             /* 6.144M soundtrigger enhance */
};

static uint16_t sd_voice_up_32k[SLIMBUS_SCENE_CONFIG_MAX][SLIMBUS_CHANNELS_MAX] = {
			{0, 0, 0, 0},                                       /* 6.144M FPGA voice up 32k*/
			{0, 0, 0, 0},                                       /* 6.144M PLAY */
			{0x1C, 0x412, 0x814, 0x81A},                        /* 6.144M call */
			{0x44D, 0x46D, 0x84D, 0x86D},                       /* 24.576M normal */
			{0x447, 0x467, 0x847, 0x867},                       /* 24.576M anc call */
			{0, 0, 0, 0},                                       /* 24.576M direct play */
			{0, 0, 0, 0},                                       /* image */
			{0, 0, 0, 0},                                       /* 6.144M fast play */
			{0, 0, 0, 0},                                       /* 24.576M fast play & rec*/
			{0, 0, 0, 0},                                       /* 24.576M fast play & st*/
			{0x2E, 0x34, 0x42E, 0x434},                         /* 12.288M call */
			{0, 0, 0, 0},                                       /* 6.144M soundtrigger enhance */
};

static uint16_t sd_soundtrigger_48k[SLIMBUS_SCENE_CONFIG_MAX][SLIMBUS_CHANNELS_MAX] = {
			{0},                                                /* 6.144M FPGA */
			{0},                                                /* 6.144M play */
			{0},                                                /* 6.144M call */
			{0xC93},                                            /* 24.576M normal */
			{0},                                                /* 24.576M anc call */
			{0xC82},                                            /* 24.576M direct play */
			{0},                                                /* image */
			{0},                                                /* 6.144M fast play */
			{0},                                                /* 24.576M fast play & rec*/
			{0xC98},                                            /* 24.576M fast play & st*/
			{0},                                                /* 12.288M call */
			{0},                                                /* 6.144M soundtrigger enhance */
};

static uint16_t sd_soundtrigger_16k[SLIMBUS_SCENE_CONFIG_MAX][SLIMBUS_CHANNELS_MAX] = {
			{0},                                                /* 6.144M fpga */
			{0},                                                /* 6.144M play */
			{0},                                                /* 6.144M call */
			{0x93, 0xB3},                                       /* 24.576M normal */
			{0},                                                /* 24.576M anc call */
			{0x82, 0xA2},                                       /* 24.576M direct play */
			{0},                                                /* image */
			{0},                                                /* 6.144M fast play */
			{0},                                                /* 24.576M fast play & rec*/
			{0x98, 0xB8},                                       /* 24.576M fast play & st*/
			{0},                                                /* 12.288M call */
			{0x24, 0x424},                                      /* 6.144M soundtrigger enhance */
};


static uint16_t sd_debug_192k[SLIMBUS_SCENE_CONFIG_MAX][SLIMBUS_CHANNELS_MAX] = {
			{0},                                                /* 6.144M fpga */
			{0},                                                /* 6.144M play */
			{0},                                                /* 6.144M call */
			{0xC33},                                            /* 24.576M normal */
			{0xC2D, 0xC35},                                     /* 24.576M anc call*/
			{0},                                                /* 24.576M direct play */
			{0},                                                /* image */
			{0},                                                /* 6.144M fast play */
			{0},                                                /* 24.576M fast play & rec*/
			{0},                                                /* 24.576M fast play & st*/
			{0},                                                /* 12.288M call */
			{0},                                                /* 6.144M soundtrigger enhance */
};
static uint16_t sd_direct_play_96k[SLIMBUS_SCENE_CONFIG_MAX][SLIMBUS_CHANNELS_MAX] = {
			{0, 0},                                             /* 6.144M FPGA voice down 16k */
			{0, 0},                                             /* 6.144M play */
			{0, 0},                                             /* 6.144M call */
			{0, 0},                                             /* 24.576M normal */
			{0, 0},                                             /* 24.576M anc call */
			{0xC50, 0xC56},                                     /* 24.576M direct play */
			{0, 0},                                             /* image */
			{0, 0},                                             /* 6.144M fast play */
			{0xC4E, 0xC54},                                     /* 24.576M fast play & rec*/
			{0xC50, 0xC56},                                     /* 24.576M fast play & st*/
			{0, 0},                                             /* 12.288M call */
			{0, 0},                                             /* 6.144M soundtrigger enhance */
};
static uint16_t sd_direct_play_48k[SLIMBUS_SCENE_CONFIG_MAX][SLIMBUS_CHANNELS_MAX] = {
			{0, 0},                                             /* 6.144M FPGA voice down 16k */
			{0, 0},                                             /* 6.144M play */
			{0, 0},                                             /* 6.144M call */
			{0, 0},                                             /* 24.576M normal */
			{0, 0},                                             /* 24.576M anc call */
			{0xC90, 0xC96},                                     /* 24.576M direct play */
			{0, 0},                                             /* image */
			{0, 0},                                             /* 6.144M fast play */
			{0xC8E, 0xC94},                                     /* 24.576M fast play & rec*/
			{0xC90, 0xC96},                                     /* 24.576M fast play & st*/
			{0, 0},                                             /* 12.288M call */
			{0, 0},                                             /* 6.144M soundtrigger enhance */
};

static void slimbus_hi6403_set_direct_play_sd(
					uint32_t scene_type,
					uint32_t ch,
					slimbus_presence_rate_t pr,
					uint16_t *sd)
{
	if (pr == SLIMBUS_PR_96K) {
		*sd = sd_direct_play_96k[scene_type][ch];
	} else if (pr == SLIMBUS_PR_48K) {
		*sd = sd_direct_play_48k[scene_type][ch];
	}
}

static void slimbus_hi6403_set_para_sd(
					uint32_t  track_type,
					uint32_t scene_type,
					uint32_t ch,
					slimbus_presence_rate_t pr,
					uint16_t *sd)
{
	*sd = sd_table[scene_type][track_type][ch];

	switch(track_type) {
	case SLIMBUS_TRACK_VOICE_DOWN:
		if (pr == SLIMBUS_PR_16K) {
			*sd = sd_voice_down_16k[scene_type][ch];
		} else if (pr == SLIMBUS_PR_32K) {
			*sd = sd_voice_down_32k[scene_type][ch];
		}
		break;
	case SLIMBUS_TRACK_VOICE_UP:
		if (pr == SLIMBUS_PR_16K) {
			*sd = sd_voice_up_16k[scene_type][ch];
		} else if (pr == SLIMBUS_PR_32K) {
			*sd = sd_voice_up_32k[scene_type][ch];
		}
		break;
	case SLIMBUS_TRACK_SOUND_TRIGGER:
		if (pr == SLIMBUS_PR_16K) {
			*sd = sd_soundtrigger_16k[scene_type][ch];
		} else if (pr == SLIMBUS_PR_48K) {
			*sd = sd_soundtrigger_48k[scene_type][ch];
		}
		break;
	case SLIMBUS_TRACK_DEBUG:
		*sd = sd_debug_192k[scene_type][ch];
		break;
	case SLIMBUS_TRACK_DIRECT_PLAY:
		slimbus_hi6403_set_direct_play_sd(scene_type, ch, pr, sd);
		break;
	default:
		break;
	}

	return;
}

static void slimbus_hi6403_param_update_sl(
				int   track_type,
				slimbus_scene_config_type_t scene_type,
				uint32_t ch,
				slimbus_presence_rate_t pr,
				uint16_t *sl)
{
	if (track_type >= SLIMBUS_TRACK_MAX) {
		pr_err("track type is invalid, track_type:%d\n", track_type);
		return;
	}

	*sl = sl_table[scene_type][track_type][ch];

	switch(track_type) {
	case SLIMBUS_TRACK_SOUND_TRIGGER:
		if (pr == SLIMBUS_PR_16K) {
			*sl = sl_soundtrigger_16k[scene_type][ch];
		}
		break;
	default:
		break;
	}

	return;
}

void slimbus_hi6403_param_init(slimbus_device_info_t *dev)
{
	uint32_t track = 0;
	uint32_t ch = 0;
	int i = 0;
	slimbus_transport_protocol_t  tp = SLIMBUS_TP_ISOCHRONOUS;
	uint8_t source_la = SOC_LA_GENERIC_DEVICE;
	uint8_t sink_la = HI64XX_LA_GENERIC_DEVICE;
	uint8_t (*chnum_table)[SLIMBUS_CHANNELS_MAX];
	uint8_t (*source_pn)[SLIMBUS_CHANNELS_MAX];
	uint8_t (*sink_pn)[SLIMBUS_CHANNELS_MAX];
	slimbus_channel_property_t *pchprop = NULL;
	uint32_t scene_type;

	if (NULL == dev) {
		pr_err("dev is null \n");
		return;
	}
	if (dev->scene_config_type >= SLIMBUS_SCENE_CONFIG_MAX) {
		pr_err("[%s:%d] track type is invalid, scene type:%d\n", __FUNCTION__, __LINE__,
			dev->scene_config_type);
		return;
	}

	scene_type = dev->scene_config_type;
	chnum_table = cn_table;
	source_pn = source_pn_table;
	sink_pn = sink_pn_table;

	for (track = 0; track < SLIMBUS_TRACK_MAX; track++) {
		tp = SLIMBUS_TP_ISOCHRONOUS;
		source_la = SOC_LA_GENERIC_DEVICE;
		sink_la = HI64XX_LA_GENERIC_DEVICE;

		slimbus_drv_get_params_la(track, &source_la, &sink_la, &tp);

		pchprop = track_config_table[track].channel_pro;

		for (ch = 0; ch <track_config_table[track].params.channels; ch++) {
			memset(pchprop, 0, sizeof(slimbus_channel_property_t));

			pchprop->cn = *(*(chnum_table + track) + ch);
			pchprop->source.la = source_la;
			pchprop->source.pn = *(*(source_pn + track) + ch);
			pchprop->sink_num = 1;
			for (i = 0; i < pchprop->sink_num; i++) {
				pchprop->sinks[i].la = sink_la;
				pchprop->sinks[i].pn = *(*(sink_pn + track) + ch);
			}
			if (SLIMBUS_TRACK_ECREF == track) {
				pchprop->sink_num = 2;
				pchprop->sinks[1].la = sink_la;
				pchprop->sinks[1].pn = BT_CAPTURE_SOC_RIGHT_PORT;

				if (!ch) {
					pchprop->sinks[1].pn  = IMAGE_DOWNLOAD_SOC_PORT;
				}
			}

			pchprop->tp = tp;
			pchprop->fl = 0;
			pchprop->af = SLIMBUS_AF_NOT_APPLICABLE;
			pchprop->dt = SLIMBUS_DF_NOT_INDICATED;
			pchprop->cl = 0;
			pchprop->pr = pr_table[track];

			pchprop->sl = sl_table[scene_type][track][ch];

			slimbus_hi6403_set_para_sd(track,
				scene_type, ch, pchprop->pr, &(pchprop->sd));

			slimbus_hi6403_param_update_sl(track, scene_type, ch, pchprop->pr, &(pchprop->sl));

			if (SLIMBUS_TP_PUSHED == tp) {
				pchprop->dl = pchprop->sl - 1;
			} else {
				pchprop->dl = pchprop->sl;
			}

			pchprop++;
		}
	}

	return;
}

int slimbus_hi6403_param_set(
			slimbus_device_info_t *dev,
			uint32_t track_type,
			slimbus_track_param_t *params)
{
	slimbus_channel_property_t *pchprop = NULL;
	uint32_t scene_type = SLIMBUS_SCENE_CONFIG_MAX;
	slimbus_track_param_t slimbus_param;
	unsigned int ch = 0;

	if (!dev) {
		pr_err("[%s:%d] dev is NULL\n", __FUNCTION__, __LINE__);
		return -EINVAL;
	}
	if (track_type >= SLIMBUS_TRACK_MAX) {
		pr_err("[%s:%d] track type is invalid, track type:%d\n", __FUNCTION__, __LINE__, track_type);
		return -EINVAL;
	}
	if (dev->scene_config_type >= SLIMBUS_SCENE_CONFIG_MAX) {
		pr_err("[%s:%d] scene type is invalid, scene type:%d\n", __FUNCTION__, __LINE__,
			dev->scene_config_type);
		return -EINVAL;
	}

	scene_type = dev->scene_config_type;

	if (!params) {
		memcpy(&slimbus_param, &(track_config_table[track_type].params), sizeof(slimbus_param));
		params = &slimbus_param;
	}
	pr_info("[%s:%d] track type:%d, rate:%d, chnum:%d\n", __FUNCTION__, __LINE__, track_type, params->rate, params->channels);

	track_config_table[track_type].params.channels = params->channels;
	track_config_table[track_type].params.rate = params->rate;
	track_config_table[track_type].params.callback = params->callback;

	pchprop = track_config_table[track_type].channel_pro;/*lint!e838*/

	if (SLIMBUS_TRACK_VOICE_UP == track_type) {
		dev->voice_up_chnum = params->channels;
	} else if (SLIMBUS_TRACK_AUDIO_CAPTURE == track_type) {
		dev->audio_up_chnum = params->channels;
	} else if (SLIMBUS_TRACK_VOICE_DOWN == track_type) {
		track_config_table[track_type].params.channels = 2;
	} else {
		//do nothing
	}

	if (pchprop) {
		for (ch = 0; ch < track_config_table[track_type].params.channels; ch++) {

			slimbus_hi64xx_set_para_pr(pr_table, track_type, params);

			pchprop->pr = pr_table[track_type];

			slimbus_hi6403_set_para_sd(track_type, scene_type,
				ch, pchprop->pr, &(pchprop->sd));
			slimbus_hi6403_param_update_sl(track_type, scene_type, ch, pchprop->pr, &(pchprop->sl));

			pchprop++;
		}
	}

	return 0;
}

void slimbus_hi6403_param_update(
			slimbus_device_info_t *dev,
			uint32_t track_type,
			slimbus_track_param_t *params)
{
	slimbus_hi64xx_set_para_pr(pr_table, track_type, params);

	slimbus_hi6403_param_init(dev);

	return;
}

/*
 * create hi6403 slimbus device
 * @device, pointer to created instance
 *
 * return 0, if success
 */
int create_hi6403_slimbus_device(slimbus_device_info_t **device)
{
	struct slimbus_device_info *dev_info = NULL;

	dev_info = kzalloc(sizeof(struct slimbus_device_info), GFP_KERNEL);
	if (!dev_info) {
		pr_err("[%s:%d] malloc slimbus failed!\n", __FUNCTION__, __LINE__);
		return -ENOMEM;
	}
	dev_info->slimbus_64xx_para = kzalloc(sizeof(struct slimbus_device_info), GFP_KERNEL);
	if (!dev_info->slimbus_64xx_para) {
		pr_err("[%s:%d] malloc slimbus failed!\n", __FUNCTION__, __LINE__);
		kfree(dev_info);
		dev_info = NULL;
		return -ENOMEM;
	}

	dev_info->generic_la = HI64XX_LA_GENERIC_DEVICE;
	dev_info->voice_up_chnum = SLIMBUS_VOICE_UP_2CH;;
	dev_info->audio_up_chnum = SLIMBUS_AUDIO_CAPTURE_MULTI_MIC_CHANNELS;//default is 4mic
	dev_info->page_sel_addr = 1;
	dev_info->sm = SLIMBUS_SM_1_CSW_32_SL;
	dev_info->cg = SLIMBUS_CG_10;
	dev_info->scene_config_type = SLIMBUS_SCENE_CONFIG_NORMAL;

	dev_info->slimbus_64xx_para->slimbus_track_max = SLIMBUS_TRACK_MAX;
	dev_info->slimbus_64xx_para->track_config_table = track_config_table;

	mutex_init(&dev_info->rw_mutex);
	mutex_init(&dev_info->track_mutex);

	*device = dev_info;

	return 0;
}

void slimbus_hi6403_get_st_params(slimbus_sound_trigger_params_t *params)
{
	if (params != NULL) {
		params->sample_rate = SLIMBUS_SAMPLE_RATE_48K;
		params->channels = 2;
		params->track_type = SLIMBUS_TRACK_AUDIO_CAPTURE;
	}

	return;
}

