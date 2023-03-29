/*
 * Copyright 2002 Hewlett-Packard Company
 *
 * Use consistent with the GNU GPL is permitted,
 * provided that this copyright notice is
 * preserved in its entirety in all copies and derived works.
 *
 * HEWLETT-PACKARD COMPANY MAKES NO WARRANTIES, EXPRESSED OR IMPLIED,
 * AS TO THE USEFULNESS OR CORRECTNESS OF THIS CODE OR ITS
 * FITNESS FOR ANY PARTICULAR PURPOSE.
 *
 */

#ifndef _MMC_KIRIN_LIB_H
#define _MMC_KIRIN_LIB_H

int mmc_set_boot_partition_type(int boot_partition_type);
int mmc_get_boot_partition_type(struct mmc_card *card, u8 *ext_csd);

#endif
