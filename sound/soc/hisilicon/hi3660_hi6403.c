/*
 *  slimbus is a kernel driver which is used to manager SLIMbus devices
 *  Copyright (C) 2014  Hisilicon

 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.

 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/of.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>

#define  EXTERN_HIFI_CODEC_AK4376_NAME  "ak4376"

/*lint -e785*/

static struct snd_soc_dai_link hi3660_hi6403_dai_link[] = {
	{
		/* dai link name*/
		.name       = "hi3660_hi6403_pb_normal",
		/* stream name same as name */
		.stream_name    = "hi3660_hi6403_pb_normal",
		/* codec(hi6403) device name ,see in hi6403.c */
		.codec_name = "hi6403-codec",
		/* cpu(k3v3:asp) dai name(device name), see in hi3660-pcm.c */
		.cpu_dai_name   = "slimbus-dai",
		/* codec dai name, see in struct snd_soc_dai_driver in hi6403.c */
		.codec_dai_name = "hi6403-audio-dai",
		/* platform(k3v3:asp) device name, see in hi3660-pcm.c */
		.platform_name  = "hi6210-hifi",
	},
	{
		/* dai link name*/
		.name       = "hi3660_voice",
		/* stream name same as name */
		.stream_name    = "hi3660_voice",
		/* codec(hi6403) device name ,see in hi6403.c */
		.codec_name = "hi6403-codec",
		/* cpu(k3v3:asp) dai name(device name), see in hi3660-pcm.c */
		.cpu_dai_name   = "slimbus-dai",
		/* codec dai name, see in struct snd_soc_dai_driver in hi6403.c */
		.codec_dai_name = "hi6403-voice-dai",
		/* platform(k3v3:asp) device name, see in hi3660-pcm.c */
		.platform_name  = "snd-soc-dummy",
	},
	{
		/* dai link name*/
		.name       = "hi3660_fm1",
		/* stream name same as name */
		.stream_name    = "hi3660_fm1",
		/* codec(hi6403) device name ,see in hi6403.c */
		.codec_name = "hi6403-codec",
		/* cpu(k3v3:asp) dai name(device name), see in hi3660-pcm.c */
		.cpu_dai_name   = "slimbus-dai",
		/* codec dai name, see in struct snd_soc_dai_driver in hi6403.c */
		.codec_dai_name = "hi6403-fm-dai",
		/* platform(k3v3:asp) device name, see in hi3660-pcm.c */
		.platform_name  = "snd-soc-dummy",
	},
	{
		/* dai link name*/
		.name       = "hi3660_hi6403_pb_dsp",
		/* stream name same as name */
		.stream_name    = "hi3660_hi6403_pb_dsp",
		/* codec(hi6403) device name ,see in hi6403.c */
		.codec_name = "hi6403-codec",
		/* cpu(k3v3:asp) dai name(device name), see in hi3660-pcm.c */
		.cpu_dai_name   = "slimbus-dai",
		/* codec dai name, see in struct snd_soc_dai_driver in hi6403.c */
		.codec_dai_name = "hi6403-audio-dai",
		/* platform(k3v3:asp) device name, see in hi3660-pcm.c */
		.platform_name  = "hi6210-hifi",
	},
	{
		/* dai link name*/
		.name       = "hi3660_hi6403_pb_direct",
		/* stream name same as name */
		.stream_name    = "hi3660_hi6403_pb_direct",
		/* codec(hi6403) device name ,see in hi6403.c */
		.codec_name = "hi6403-codec",
		/* cpu(k3v3:asp) dai name(device name), see in hi3660-pcm.c */
		.cpu_dai_name   = "slimbus-dai",
		/* codec dai name, see in struct snd_soc_dai_driver in hi6403.c */
		.codec_dai_name = "hi6403-audio-dai",
		/* platform(k3v3:asp) device name, see in hi3660-pcm.c */
		.platform_name  = "hi6210-hifi",
	},
	{
		/* dai link name*/
		.name       = "hi3660_hi6403_lowlatency",
		/* stream name same as name */
		.stream_name    = "hi3660_hi6403_lowlatency",
		/* codec(hi6403) device name ,see in hi6403.c */
		.codec_name = "hi6403-codec",
		/* cpu(k3v3:asp) dai name(device name), see in slimbus_dai.c */
		.cpu_dai_name   = "slimbus-dai",
		/* codec dai name, see in struct snd_soc_dai_driver in hi6403.c */
		.codec_dai_name = "hi6403-audio-dai",
#ifdef AUDIO_LOW_LATENCY_LEGACY
		/* platform(k3v3:asp) device name, see in hi3xxx_asp_dma.c */
		.platform_name  = "hi3xxx-pcm-asp-dma",
#else
		/* platform device name, see in hi6210_pcm.c */
		.platform_name  = "hi6210-hifi",
#endif
	},
	{
		/* dai link name*/
		.name = "hi3660_hi6403_mmap",
		/* stream name same as name */
		.stream_name = "hi3660_hi6403_mmap",
		/* codec(hi6403) device name ,see in hi6403.c */
		.codec_name = "hi6403-codec",
		/* cpu(k3v3:asp) dai name(device name), see in slimbus_dai.c */
		.cpu_dai_name = "slimbus-dai",
		/* codec dai name, see in struct snd_soc_dai_driver in hi6403.c */
		.codec_dai_name = "hi6403-audio-dai",
		/* platform device name, see in hisi_pcm_hifi.c */
		.platform_name = "hi6210-hifi",
	},
};

static struct snd_soc_dai_link ak4376_dai_link[] = {
	/* RX for headset/headphone with audio mode */
	{
		 .name = "AK4376_PB_OUTPUT",
		 .stream_name = "Audio Playback",
		 .codec_name = "ak4376-codec",
		 .cpu_dai_name = "hifi-vir-dai",
		 .platform_name = "hi6210-hifi",
		 .codec_dai_name = "ak4376-AIF1",
	},
};

static struct snd_soc_dai_link hi3660_hi6403_ak4376_dai_links[
				ARRAY_SIZE(hi3660_hi6403_dai_link) +
				ARRAY_SIZE(ak4376_dai_link)];

/* Audio machine driver */
static struct snd_soc_card hi3660_hi6403_card = {
	/* sound card name, can see all sound cards in /proc/asound/cards */
	.name = "hi3660_HI6403_CARD",
	.owner = THIS_MODULE,
	.dai_link = hi3660_hi6403_dai_link,
	.num_links = ARRAY_SIZE(hi3660_hi6403_dai_link),
};

struct snd_soc_card hi3660_hi6403_ak4376_card = {
	.name = "hi3660_HI6403_AK4376_CARD",
	.owner = THIS_MODULE,
	.dai_link = hi3660_hi6403_ak4376_dai_links,
	.num_links = ARRAY_SIZE(hi3660_hi6403_ak4376_dai_links),
};

static int populate_extern_snd_card_dailinks(struct platform_device *pdev)
{
	pr_info("%s: Audio : hi3660_hi6403_ak4376_probe \n", __func__);

	memcpy(hi3660_hi6403_ak4376_dai_links, hi3660_hi6403_dai_link,
				sizeof(hi3660_hi6403_dai_link));
	memcpy(hi3660_hi6403_ak4376_dai_links + ARRAY_SIZE(hi3660_hi6403_dai_link),
			ak4376_dai_link, sizeof(ak4376_dai_link));

	return 0;
}

static int hi3660_hi6403_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct snd_soc_card *card = &hi3660_hi6403_card;
	const char *extern_codec_type = "huawei,extern_codec_type";
	const char *ptr = NULL;

	if (NULL == pdev) {
		pr_err("%s : enter with null pointer, fail!\n", __FUNCTION__);
		return -EINVAL;
	}

	pr_info("Audio : hi3660_hi6403_probe \n");
	ret = of_property_read_string(pdev->dev.of_node, extern_codec_type, &ptr);
	if (!ret) {
		pr_info("%s: extern_codec_type: %s in dt node\n", __func__, extern_codec_type);
		if (!strncmp(ptr, EXTERN_HIFI_CODEC_AK4376_NAME, strlen(EXTERN_HIFI_CODEC_AK4376_NAME))) {
			populate_extern_snd_card_dailinks(pdev);
			pr_info("Audio : set hi3660_hi6403_ak4376_card\n");
			card = &hi3660_hi6403_ak4376_card;
		} else {
			card = &hi3660_hi6403_card;
		}
	} else {
		card = &hi3660_hi6403_card;
	}
	card->dev = &pdev->dev;

	ret = snd_soc_register_card(card);
	if (ret) {
		pr_err("%s : register failed %d\n", __FUNCTION__, ret);
	}

	return ret;
}

static int hi3660_hi6403_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);

	if (NULL != card)
		snd_soc_unregister_card(card);

	return 0;
}

static const struct of_device_id hi3660_hi6403_of_match[] = {
	{.compatible = "hisilicon,hi3xxx-hi6403", },
	{ },
};
MODULE_DEVICE_TABLE(of, hi3660_hi6403_of_match);

static struct platform_driver hi3660_hi6403_driver = {
	.driver	= {
		.name = "hi3660_hi6403",
		.owner = THIS_MODULE,
		.of_match_table = hi3660_hi6403_of_match,
	},
	.probe	= hi3660_hi6403_probe,
	.remove	= hi3660_hi6403_remove,
};
//module_platform_driver(hi3660_hi6403_driver);

static int __init hi3660_init(void)
{
	pr_info("Audio : hi3xxx-hi6403 init \n");
    return platform_driver_register(&hi3660_hi6403_driver);
}

late_initcall(hi3660_init);

static void __exit hi3660_exit(void)
{
    //remove_proc_entry("status", audio_pcm_dir);

    platform_driver_unregister(&hi3660_hi6403_driver);
}
module_exit(hi3660_exit);

/* Module information */
MODULE_AUTHOR("liuyang <liuyang66@hisilicon.com>");
MODULE_DESCRIPTION("ALSA SoC for Hisilicon hi3660 with hi6403 codec");
MODULE_LICENSE("GPL");
MODULE_ALIAS("machine driver:hi3660-hi6403");
