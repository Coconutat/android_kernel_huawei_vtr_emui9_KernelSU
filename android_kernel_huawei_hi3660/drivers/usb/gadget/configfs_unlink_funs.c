

#include <linux/fs.h>
struct configfs_dirent {
	atomic_t		s_count;
	int			s_dependent_count;
	struct list_head	s_sibling;
	struct list_head	s_children;
	struct list_head	s_links;
	void			*s_element;
	int			s_type;
	umode_t			s_mode;
	struct dentry		*s_dentry;
	struct iattr		*s_iattr;
#ifdef CONFIG_LOCKDEP
	int			s_depth;
#endif
};

struct configfs_symlink {
	struct list_head sl_list;
	struct config_item *sl_target;
};

#define CONFIGFS_ITEM_LINK      0x0020
extern int configfs_unlink(struct inode *dir, struct dentry *dentry);

static void gadget_config_unlink_functions(struct dentry *p_dentry)
{
	struct configfs_dirent *p_sd = p_dentry->d_fsdata;

	if (!list_empty(&p_sd->s_children)) {
		struct configfs_dirent *sd, *tmp;

		list_for_each_entry_safe(sd, tmp,
				&p_sd->s_children, s_sibling) {
			if (sd->s_type & CONFIGFS_ITEM_LINK) {
				struct configfs_symlink *sl = sd->s_element;
				struct config_item *item = sl->sl_target;

				pr_info("config unlink %s\n", item->ci_name);
				configfs_unlink(0, sd->s_dentry);
			}
		}
	}
}

static inline void gadget_unlink_functions(struct gadget_info *gi)
{
	if (!list_empty(&gi->configs_group.cg_children)) {
		struct config_item *item;

		list_for_each_entry(item,
				&gi->configs_group.cg_children, ci_entry) {
			pr_info("unlink %s\n", item->ci_name);
			mutex_unlock(&gi->lock);
			gadget_config_unlink_functions(item->ci_dentry);
			mutex_lock(&gi->lock);
		}
	}
}

