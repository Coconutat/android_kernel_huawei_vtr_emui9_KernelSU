#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#ifdef CONFIG_HISI_MMC_MANUAL_BKOPS
#include <linux/blkdev.h>
#endif


int hisi_mmc_add_card_debugfs(struct mmc_card *card, struct dentry *root)
{
#ifdef CONFIG_HISI_MMC_FLUSH_REDUCE
	if (!debugfs_create_u8("flush_reduce_en", S_IRUSR, root, &(card->host->mmc_flush_reduce_enable)))
		return -1;
#endif

	return 0;
}

