#ifndef FUNCIONT_HISI_RNDIS_HISI_H
#define FUNCIONT_HISI_RNDIS_HISI_H

#ifdef CONFIG_HISI_USB_CONFIGFS
void hisi_uether_enable_set(int n);
int hisi_uether_enable_get(void);
#else
static inline int hisi_uether_enable_set(int n)
{
	return 0;
}

static inline int hisi_uether_enable_get(void)
{
	return 1;
}
#endif /* CONFIG_HISI_USB_CONFIGFS */

#endif /* funciont-hisi/f_rndis_hisi.h */
