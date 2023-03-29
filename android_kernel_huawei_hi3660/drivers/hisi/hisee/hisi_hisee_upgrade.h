#ifndef __HISI_HISEE_UPGRADE_H__
#define __HISI_HISEE_UPGRADE_H__

#define HISEE_OLD_COS_IMAGE_ERROR      (-8000)
#define HISEE_IS_OLD_COS_IMAGE         (-8001)

#define HISEE_MISC_VERSION0            (0x20)
/*version=0 indicat there is not enable cos image upgrade sw version*/
#define HISEE_DEFAULT_SW_UPGRADE_VERSION (0x0)

int cos_image_upgrade_func(void *buf, int para);
int handle_cos_image_upgrade(void *buf, int para);
int misc_image_upgrade_func(void *buf, unsigned int cos_id);
int cos_upgrade_image_read(unsigned int cos_id, hisee_img_file_type img_type);

#ifdef CONFIG_HICOS_MISCIMG_PATCH
int hisee_cos_patch_read(hisee_img_file_type img_type);
#endif

ssize_t hisee_has_new_cos_show(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t hisee_check_upgrade_show(struct device *dev, struct device_attribute *attr, char *buf);

#endif
