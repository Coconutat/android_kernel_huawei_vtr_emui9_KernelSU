/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>

static int hifi_vir_dai_startup(struct snd_pcm_substream *substream,
						struct snd_soc_dai *dai)
{
	return 0;
}

static void hifi_vir_dai_shutdown(struct snd_pcm_substream *substream,
						  struct snd_soc_dai *dai)
{
	/* do nothing */
}

static int hifi_vir_dai_hw_params(struct snd_pcm_substream *substream,
						  struct snd_pcm_hw_params *params,
						  struct snd_soc_dai *dai)
{
	int ret = 0;

	return ret;
}

static  int hifi_vir_dai_trigger(struct snd_pcm_substream *substream, int cmd,
						 struct snd_soc_dai *dai)
{
	int ret = 0;

	return ret;
}

static int hifi_vir_dai_hw_free(struct snd_pcm_substream *substream,
						struct snd_soc_dai *dai)
{
	int ret = 0;

	return ret;
}

static const struct snd_soc_dai_ops hifi_vir_dai_ops = {
	.startup    = hifi_vir_dai_startup,
	.shutdown   = hifi_vir_dai_shutdown,
	.hw_params  = hifi_vir_dai_hw_params,
	.trigger    = hifi_vir_dai_trigger,
	.hw_free    = hifi_vir_dai_hw_free,
};

static int hifi_vir_dai_probe(struct snd_soc_dai *dai)
{
	return 0;
}


static const struct snd_soc_component_driver hifi_vir_dai_component = {
	.name		= "hifi-vir-dai",
};


static struct snd_soc_dai_driver hifi_vir_dais[] = {
	{
		.playback = {
			.rates = SNDRV_PCM_RATE_8000_192000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S16_BE |
			SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S24_BE,
			.channels_min = 1,
			.channels_max = 2,
			.rate_min =     8000,
			.rate_max =	    192000,
		},
		.ops = &hifi_vir_dai_ops,
		.probe = hifi_vir_dai_probe,
	},

};

static int hifi_vir_dai_dev_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int ret = 0;

	pr_info("[%s:%d] hifi_vir_dai probe!\n", __func__, __LINE__);

	dev_set_name(dev, "hifi-vir-dai");
	pr_info("%s: hifi_dai %s\n", __func__, dev_name(dev));

	ret = snd_soc_register_component(&pdev->dev, &hifi_vir_dai_component,
					 hifi_vir_dais, ARRAY_SIZE(hifi_vir_dais));

	return ret;
}

static int hifi_vir_dai_dev_remove(struct platform_device *pdev)
{
	snd_soc_unregister_component(&pdev->dev);
	return 0;
}

static const struct of_device_id hifi_vir_dai_dt_match[] = {
	{.compatible = "huawei,hifi-vir-dai"},
	{}
};

static struct platform_driver hifi_vir_dai_driver = {
	.probe  = hifi_vir_dai_dev_probe,
	.remove = hifi_vir_dai_dev_remove,
	.driver = {
		.name = "hifi-vir-dai",
		.owner = THIS_MODULE,
		.of_match_table = hifi_vir_dai_dt_match,
	},
};

static int __init hifi_vir_dai_init(void)
{
	return platform_driver_register(&hifi_vir_dai_driver);
}
module_init(hifi_vir_dai_init);

static void __exit hifi_vir_dai_exit(void)
{
	platform_driver_unregister(&hifi_vir_dai_driver);
}
module_exit(hifi_vir_dai_exit);


MODULE_LICENSE("GPL");

