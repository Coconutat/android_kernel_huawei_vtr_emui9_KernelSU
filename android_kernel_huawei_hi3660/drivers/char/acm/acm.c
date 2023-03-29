#include "acm.h"

/* Hash table for white list */
acm_static acm_hash_table_t acm_hash;
/* List for dir */
acm_static acm_list_t acm_dir_list;
/* List for framework */
acm_static acm_list_t acm_fwk_list;
/* List for DMD */
acm_static acm_list_t acm_dmd_list;

static dev_t acm_devno;
static struct cdev *acm_cdev;
static struct class *acm_class;
static struct device *acm_device;
static struct kset *acm_kset;

static struct task_struct *acm_fwk_task;
static struct task_struct *acm_dmd_task;

acm_cache_t dmd_cache;
acm_env_t fwk_env, dsm_env;

/*
 * The flag of acm state after acm_init.
 * 0 is successful or none-zero errorno failed.
 * It should be initialized as none-zero.
 */
static long acm_init_state = -ENOTTY;

EXPORT_FOR_ACM_DEBUG(acm_hash);
EXPORT_FOR_ACM_DEBUG(acm_fwk_list);
EXPORT_FOR_ACM_DEBUG(acm_dmd_list);

acm_static int valid_len(char *str, int maxlen)
{
	int len = 0;

	len = strnlen(str, maxlen);
	if (len == 0 || len > maxlen - 1)
		return -EINVAL;

	return ACM_SUCCESS;
}

static int get_valid_strlen(char *p, int maxlen)
{
	int len = 0;

	len = strnlen(p, maxlen);
	if (len > maxlen - 1) {
		len = maxlen - 1;
		*(p + len) = '\0';
	}

	return len;
}

/* Hash functions */
int acm_hash_init(void)
{
	int i;
	struct hlist_head *head = NULL;

	head = (struct hlist_head *)kzalloc(sizeof(struct hlist_head) * ACM_HASH_TABLE_SIZE, GFP_KERNEL);
	if (!(head)) {
		PRINT_ERR("Failed to allocate memory for acm hash table.\n");
		return -ENOMEM;
	}
	for (i = 0; i < ACM_HASH_TABLE_SIZE; i++)
		INIT_HLIST_HEAD(&(head[i]));

	acm_hash.head = head;
	acm_hash.nr_nodes = 0;
	spin_lock_init(&acm_hash.spinlock);
	return ACM_SUCCESS;
}

acm_static acm_hash_node_t *acm_hash_search(struct hlist_head *hash, char *keystring)
{
	struct hlist_head *phead = NULL;
	acm_hash_node_t *pnode = NULL;
	unsigned int idx;

	idx = ELFHash(keystring);
	spin_lock(&acm_hash.spinlock);
	phead = &hash[idx];
	hlist_for_each_entry(pnode, phead, hnode) {
		if (pnode->pkgname) {
			if (!strcmp(pnode->pkgname, keystring))
				break;
		}
	}
	spin_unlock(&acm_hash.spinlock);
	return pnode;
}

static void acm_hash_add(acm_hash_table_t *hash_table, acm_hash_node_t *hash_node)
{
	struct hlist_head *phead = NULL;
	struct hlist_head *hash = hash_table->head;

	spin_lock(&hash_table->spinlock);
	WARN_ON(hash_table->nr_nodes > HASH_TABLE_MAX_SIZE - 1);
	phead = &hash[ELFHash(hash_node->pkgname)];
	hlist_add_head(&hash_node->hnode, phead);
	hash_table->nr_nodes++;
	spin_unlock(&hash_table->spinlock);
}

static void acm_hash_del(acm_hash_table_t *hash_table, acm_hash_node_t *hash_node)
{
	spin_lock(&hash_table->spinlock);
	WARN_ON(hash_table->nr_nodes < 1);
	hlist_del(&(hash_node->hnode));
	hash_table->nr_nodes--;
	spin_unlock(&hash_table->spinlock);
	kfree(hash_node);
}

/* ELFHash Function */
acm_static unsigned int ELFHash(char *str)
{
	unsigned int ret = 0, x = 0, hash = 0;

	while (*str) {
		hash = (hash << 4) + (*str++);
		x = hash & 0xF0000000L;
		if (x != 0) {
			hash ^= (x >> 24);
			hash &= ~x;
		}
	}
	ret = (hash & 0x7FFFFFFF) % ACM_HASH_TABLE_SIZE;
	return ret;
}

/* List functions */
static void acm_list_init(acm_list_t *list)
{
	INIT_LIST_HEAD(&list->head);
	list->nr_nodes = 0;
	spin_lock_init(&list->spinlock);
}

static void acm_dir_list_add(struct list_head *head, acm_dir_node_t *dir_node)
{
	spin_lock(&acm_dir_list.spinlock);
	WARN_ON(acm_dir_list.nr_nodes > ACM_DIR_LIST_MAX_LEN - 1);
	list_add_tail(&dir_node->lnode, head);
	acm_dir_list.nr_nodes++;
	spin_unlock(&acm_dir_list.spinlock);
}

acm_static void acm_fwk_add(struct list_head *head, acm_list_node_t *list_node)
{
	spin_lock(&acm_fwk_list.spinlock);
	list_add_tail(&list_node->lnode, head);
	spin_unlock(&acm_fwk_list.spinlock);
}

acm_static void acm_dmd_add(struct list_head *head, acm_list_node_t *list_node)
{

	spin_lock(&acm_dmd_list.spinlock);
	if (acm_dmd_list.nr_nodes > ACM_DMD_LIST_MAX_NODES - 1) {
		PRINT_ERR("List was too long! Data will be dropped! Pkgname = %s\n",
			list_node->pkgname);
		spin_unlock(&acm_dmd_list.spinlock);
		return;
	}
	list_add_tail(&list_node->lnode, head);
	acm_dmd_list.nr_nodes++;
	spin_unlock(&acm_dmd_list.spinlock);
}

/* ioctl related functions */
static int do_cmd_add(unsigned long args)
{
	int err = 0;
	acm_hash_node_t *temp_hash_node = NULL, *result = NULL;

	temp_hash_node = (acm_hash_node_t *)kzalloc(sizeof(acm_hash_node_t), GFP_KERNEL);
	if (!temp_hash_node) {
		PRINT_ERR("Failed to allocate memory for acm hash node!\n");
		return -ENOMEM;
	}
	INIT_HLIST_NODE(&temp_hash_node->hnode);

	if (copy_from_user(temp_hash_node->pkgname, (char *)args, ACM_PKGNAME_MAX_LEN)) {
		err = -EFAULT;
		goto do_cmd_add_ret;
	}
	if (valid_len(temp_hash_node->pkgname, ACM_PKGNAME_MAX_LEN)) {
		err = -EINVAL;
		goto do_cmd_add_ret;
	}
	temp_hash_node->pkgname[ACM_PKGNAME_MAX_LEN - 1] = '\0';

	result = acm_hash_search(acm_hash.head, temp_hash_node->pkgname);
	if (result) {
		PRINT_ERR("Pkgname %s is already in the white list!\n", temp_hash_node->pkgname);
		err = ACM_SUCCESS;
		goto do_cmd_add_ret;

	}

	acm_hash_add(&acm_hash, temp_hash_node);
	PRINT_DEBUG("Add pkgname:%s nr_nodes:%d\n", temp_hash_node->pkgname, acm_hash.nr_nodes);

	return err;

do_cmd_add_ret:
	kfree(temp_hash_node);
	return err;
}

static int do_cmd_delete(unsigned long args)
{
	int err = 0;
	acm_hash_node_t *temp_hash_node = NULL;
	acm_fwk_pkg_t *acm_fwk_pkg = NULL;

	acm_fwk_pkg = (acm_fwk_pkg_t *)kzalloc(sizeof(acm_fwk_pkg_t), GFP_KERNEL);
	if (!acm_fwk_pkg) {
		PRINT_ERR("Failed to allocate memory for acm_fwk_pkg!\n");
		return -ENOMEM;
	}

	if (copy_from_user(acm_fwk_pkg->pkgname, (char *)args, ACM_PKGNAME_MAX_LEN)) {
		err = -EFAULT;
		goto do_cmd_delete_ret;
	}
	if (valid_len(acm_fwk_pkg->pkgname, ACM_PKGNAME_MAX_LEN)) {
		err = -EINVAL;
		goto do_cmd_delete_ret;
	}
	acm_fwk_pkg->pkgname[ACM_PKGNAME_MAX_LEN - 1] = '\0';

	temp_hash_node = acm_hash_search(acm_hash.head, acm_fwk_pkg->pkgname);
	if (!temp_hash_node) {
		PRINT_ERR("Package name not found!Pkgname is %s\n", acm_fwk_pkg->pkgname);
		err = -ENODATA;
		goto do_cmd_delete_ret;
	}

	acm_hash_del(&acm_hash, temp_hash_node);
	PRINT_DEBUG("Delete pkgname:%s nr_nodes:%d\n", temp_hash_node->pkgname, acm_hash.nr_nodes);

do_cmd_delete_ret:
	kfree(acm_fwk_pkg);
	return err;
}

static int do_cmd_search(unsigned long args)
{
	int err = 0;
	acm_hash_node_t *temp_hash_node = NULL;
	acm_mp_node_t *acm_mp_node = NULL;
#ifdef CONFIG_ACM_TIME_COST
	struct timespec start, end;

	getrawmonotonic(&start);
#endif

	acm_mp_node = (acm_mp_node_t *)kzalloc(sizeof(acm_mp_node_t), GFP_KERNEL);
	if (!acm_mp_node) {
		PRINT_ERR("Failed to allocate memory for acm_mp_node!\n");
		return -ENOMEM;
	}
	if (copy_from_user(acm_mp_node, (acm_mp_node_t *)args,
		sizeof(acm_mp_node_t))) {
		err = -EFAULT;
		goto do_cmd_search_ret;
	}
	if (valid_len(acm_mp_node->pkgname, ACM_PKGNAME_MAX_LEN)) {
		err = -EINVAL;
		goto do_cmd_search_ret;
	}
	if (valid_len(acm_mp_node->path, ACM_PATH_MAX)) {
		err = -EINVAL;
		goto do_cmd_search_ret;
	}
	acm_mp_node->pkgname[ACM_PKGNAME_MAX_LEN - 1] = '\0';
	acm_mp_node->path[ACM_PATH_MAX - 1] = '\0';

	temp_hash_node = acm_hash_search(acm_hash.head, acm_mp_node->pkgname);
	acm_mp_node->flag = (temp_hash_node)?(0):(-1);

	if (copy_to_user((acm_mp_node_t *)args, acm_mp_node,
		sizeof(acm_mp_node_t))) {
		err = -EFAULT;
		goto do_cmd_search_ret;
	}
	PRINT_ERR("Search result:pkgname=%s flag=%d\n", acm_mp_node->pkgname,
		acm_mp_node->flag);

do_cmd_search_ret:
	kfree(acm_mp_node);
#ifdef CONFIG_ACM_TIME_COST
	getrawmonotonic(&end);
	PRINT_ERR("TIME_COST: start.tv_sec = %lu start.tv_nsec = %lu end.tv_sec = %lu end.tv_nsec = %lu duraion = %lu\n",
		start.tv_sec, start.tv_nsec, end.tv_sec, end.tv_nsec, end.tv_nsec - start.tv_nsec);
#endif
	return err;
}

static int do_cmd_add_dir(unsigned long args)
{
	int err = 0;
	acm_dir_node_t *dir_node = NULL;
	acm_fwk_dir_t *acm_fwk_dir = NULL;

	acm_fwk_dir = (acm_fwk_dir_t *)kzalloc(sizeof(acm_fwk_dir_t), GFP_KERNEL);
	if (!acm_fwk_dir) {
		PRINT_ERR("Failed to allocate memory for acm_fwk_dir!\n");
		return -ENOMEM;
	}

	if (copy_from_user(acm_fwk_dir->dir, (acm_fwk_dir_t *)args,
		sizeof(acm_fwk_dir_t))) {
		PRINT_ERR("Failed to copy dir from user space!\n");
		err = -EFAULT;
		goto add_dir_ret;
	}
	if (valid_len(acm_fwk_dir->dir, ACM_DIR_MAX_LEN)) {
		PRINT_ERR("Failed to check the length of dir name!\n");
		err = -EINVAL;
		goto add_dir_ret;
	}
	acm_fwk_dir->dir[ACM_DIR_MAX_LEN - 1] = '\0';

	/* Check whether dir is already in the acm_dir_list */
	list_for_each_entry(dir_node, &acm_dir_list.head, lnode) {
		if (strncasecmp(acm_fwk_dir->dir, dir_node->dir, ACM_DIR_MAX_LEN - 1) == 0) {
			PRINT_ERR("Dir %s is already in the dir list!\n", acm_fwk_dir->dir);
			goto add_dir_ret;
		}
	}

	dir_node = (acm_dir_node_t *)kzalloc(sizeof(acm_dir_node_t), GFP_KERNEL);
	if (!dir_node) {
		PRINT_ERR("Failed to allocate memory for acm dir node!\n");
		err = -ENOMEM;
		goto add_dir_ret;
	}

	memcpy(dir_node->dir, acm_fwk_dir->dir, ACM_DIR_MAX_LEN - 1);
	dir_node->dir[ACM_DIR_MAX_LEN - 1] = '\0';

	acm_dir_list_add(&acm_dir_list.head, dir_node);
	PRINT_DEBUG("Add a dir: %s\n", dir_node->dir);

add_dir_ret:
	kfree(acm_fwk_dir);
	return err;
}

static int do_cmd_del_dir(unsigned long args)
{
	int err = 0;
	acm_dir_node_t *n, *dir_node = NULL;
	acm_fwk_dir_t *acm_fwk_dir = NULL;

	acm_fwk_dir = (acm_fwk_dir_t *)kzalloc(sizeof(acm_fwk_dir_t), GFP_KERNEL);
	if (!acm_fwk_dir) {
		PRINT_ERR("Failed to allocate memory for acm_fwk_dir!\n");
		return -ENOMEM;
	}

	if (copy_from_user(acm_fwk_dir, (acm_fwk_dir_t *)args,
		sizeof(acm_fwk_dir_t))) {
		PRINT_ERR("Failed to copy dir from user space!\n");
		err = -EFAULT;
		goto del_dir_ret;
	}
	if (valid_len(acm_fwk_dir->dir, ACM_DIR_MAX_LEN)) {
		PRINT_ERR("Failed to check the length of dir name!\n");
		err = -EINVAL;
		goto del_dir_ret;
	}
	acm_fwk_dir->dir[ACM_DIR_MAX_LEN - 1] = '\0';

	list_for_each_entry_safe(dir_node, n, &acm_dir_list.head, lnode) {
		PRINT_DEBUG("dir = %s\n", dir_node->dir);
		if (strncasecmp(dir_node->dir, acm_fwk_dir->dir, ACM_DIR_MAX_LEN - 1) == 0) {
			PRINT_ERR(" Delete a dir: %s\n", dir_node->dir);
			spin_lock(&acm_dir_list.spinlock);
			WARN_ON(acm_dir_list.nr_nodes < 1);
			list_del(&dir_node->lnode);
			acm_dir_list.nr_nodes--;
			spin_unlock(&acm_dir_list.spinlock);
			kfree(dir_node);
			goto del_dir_ret;
		}
	}

	PRINT_ERR(" Dir %s not found!\n", acm_fwk_dir->dir);

del_dir_ret:
	kfree(acm_fwk_dir);
	return err;
}

static int do_cmd_add_dsm(unsigned long args)
{
#ifdef CONFIG_ACM_DSM
	int err = 0;
	acm_mp_node_t *acm_mp_node = NULL;
	acm_list_node_t *new_dmd_node = NULL;

	acm_mp_node = (acm_mp_node_t *)kzalloc(sizeof(acm_mp_node_t), GFP_KERNEL);
	if (!acm_mp_node) {
		PRINT_ERR("Failed to allocate memory for acm_mp_node!\n");
		return -ENOMEM;
	}
	if (copy_from_user(acm_mp_node, (acm_mp_node_t *)args,
		sizeof(acm_mp_node_t))) {
		err = -EFAULT;
		goto do_cmd_add_dsm_ret;
	}

	new_dmd_node = (acm_list_node_t *)kzalloc(sizeof(acm_list_node_t), GFP_KERNEL);
	if (!new_dmd_node) {
		PRINT_ERR("Failed to allocate memory for new_dmd_node!\n");
		err = -ENOMEM;
		goto do_cmd_add_dsm_ret;
	}

	memcpy(new_dmd_node->pkgname, acm_mp_node->pkgname, ACM_PKGNAME_MAX_LEN - 1);
	new_dmd_node->pkgname[ACM_PKGNAME_MAX_LEN - 1] = '\0';

	memcpy(new_dmd_node->path, acm_mp_node->path, ACM_PATH_MAX - 1);
	new_dmd_node->path[ACM_PATH_MAX - 1] = '\0';

	new_dmd_node->depth = DEPTH_INIT;
	new_dmd_node->file_type = acm_mp_node->file_type;

	acm_dmd_add(&acm_dmd_list.head, new_dmd_node);
	if (likely(acm_dmd_task))
		wake_up_process(acm_dmd_task);

do_cmd_add_dsm_ret:
	kfree(acm_mp_node);
	return err;
#endif
}

static long acm_ioctl(struct file *filp, unsigned int cmd, unsigned long args)
{
	int err = 0;

	if (acm_init_state) {
		PRINT_ERR("Failed to initialize Access Control Module! err = %ld\n", acm_init_state);
		return -ENOTTY;
	}

	if (_IOC_TYPE(cmd) != ACM_MAGIC) {
		PRINT_ERR("Failed to check ACM_MAGIC!\n");
		return -EINVAL;
	}

	if (_IOC_NR(cmd) > ACM_CMD_MAXNR) {
		PRINT_ERR("Failed to check ACM_CMD_MAXNR!\n");
		return -EINVAL;
	}

	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void *)args, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, (void *)args, _IOC_SIZE(cmd));

	if (err) {
		PRINT_ERR("Failed to check access permission!\n");
		return -EINVAL;
	}

	switch (cmd) {
	case ACM_ADD:
		err = do_cmd_add(args);
		break;
	case ACM_DEL:
		err = do_cmd_delete(args);
		break;
	case ACM_SEARCH:
		err = do_cmd_search(args);
		break;
	case ACM_ADD_DIR:
		err = do_cmd_add_dir(args);
		break;
	case ACM_DEL_DIR:
		err = do_cmd_del_dir(args);
		break;
#ifdef CONFIG_ACM_DSM
	case ACM_ADD_DSM:
		err = do_cmd_add_dsm(args);
		break;
#endif
	default:
		PRINT_ERR("Unknown command!\n");
		return -EINVAL;
	}

	return err;
}

static int set_path(acm_list_node_t *node, struct dentry *dentry, int buflen)
{
	char *path = node->path;

	path = dentry_path_raw(dentry, path, buflen);
	if (IS_ERR(path)) {
		PRINT_ERR("Failed to get path! err = %lu\n", PTR_ERR(path));
		return -EINVAL;
	}

	memcpy(node->path, path, node->path + buflen - path);
	node->path[ACM_PATH_MAX - 1] = '\0';

	return ACM_SUCCESS;
}

static int do_get_path_error(acm_list_node_t *node, struct dentry *dentry)
{
	struct dentry *d[ERR_PATH_MAX_DENTRIES];
	int i, err, depth;

	for (i = 0; i < ERR_PATH_MAX_DENTRIES; i++)
		d[i] = dget(dentry);

	/*
	 * Find the root dentry of the current file system.The d[i] saves the
	 * top ERR_PATH_MAX_DENTRIES-1 dentries to the root dentry.
	 */
	depth = 0;
	while (!IS_ROOT(dentry)) {
		dput(d[0]);
		for (i = 0; i < ERR_PATH_MAX_DENTRIES - 1; i++)
			d[i] = d[i+1];
		dentry = d[ERR_PATH_MAX_DENTRIES - 1] = dget_parent(dentry);
		depth++;
	}
	node->depth = depth;

	dentry = d[ERR_PATH_LAST_DENTRY];

	for (i = 0; i < ERR_PATH_MAX_DENTRIES; i++)
		dput(d[i]);

	dentry = dget(dentry);
	err = set_path(node, dentry, ACM_PATH_MAX);
	if (err) {
		dput(dentry);
		PRINT_ERR("Unknown error! err=%d\n", err);
		return -EINVAL;
	}
	dput(dentry);

	return ACM_SUCCESS;
}

static int delete_log_upload_fwk(char *pkgname, struct dentry *dentry)
{
	int err;
	acm_list_node_t *new_fwk_node = NULL;

	/* Data not found */
	new_fwk_node = (acm_list_node_t *)kzalloc(sizeof(acm_list_node_t), GFP_NOFS);
	if (!new_fwk_node) {
		PRINT_ERR("Failed to allocate memory for new_fwk_node!\n");
		return -ENOMEM;
	}

	memcpy(new_fwk_node->pkgname, pkgname, ACM_PKGNAME_MAX_LEN - 1);
	new_fwk_node->pkgname[ACM_PKGNAME_MAX_LEN - 1] = '\0';

	err = set_path(new_fwk_node, dentry, ACM_PATH_MAX);
	if (err) {
		kfree(new_fwk_node);
		PRINT_ERR("Failed to get path to upload to framework! err = %d\n", err);
		return err;
	}

	/* Data in new_fwk_list will be uploaded to framework by acm_fwk_task */
	acm_fwk_add(&acm_fwk_list.head, new_fwk_node);
	wake_up_process(acm_fwk_task);

	/* ENODATA means "package name is not in the white list" here. */
	return -ENODATA;
}

static int delete_log_upload_dmd(char *pkgname, struct dentry *dentry, int file_type)
{
#ifdef CONFIG_ACM_DSM
	int err = 0;
	acm_list_node_t *new_dmd_node = NULL;

	new_dmd_node = (acm_list_node_t *)kzalloc(sizeof(acm_list_node_t), GFP_NOFS);
	if (!new_dmd_node) {
		PRINT_ERR("Failed to allocate memory for new_dmd_node!\n");
		return -ENOMEM;
	}

	memcpy(new_dmd_node->pkgname, pkgname, ACM_PKGNAME_MAX_LEN - 1);
	new_dmd_node->pkgname[ACM_PKGNAME_MAX_LEN - 1] = '\0';

	new_dmd_node->depth = DEPTH_INIT;
	new_dmd_node->file_type = file_type;

	err = set_path(new_dmd_node, dentry, ACM_PATH_MAX);
	if (err) {
		PRINT_ERR("Failed to get full path! err = %d\n", err);
		err = do_get_path_error(new_dmd_node, dentry);
		if (err) {
			PRINT_ERR("Failed to get path to upload to dmd! err = %d\n", err);
		}

	}

	/*
	 * Data in new_dmd_list will be uploaded to unrmd by acm_dmd_task, and
	 * then uploaded to dmd server.
	 */
	acm_dmd_add(&acm_dmd_list.head, new_dmd_node);
	wake_up_process(acm_dmd_task);

	return err;
#endif
}

static int inquiry_delete_policy(char *pkgname, uid_t uid)
{
	acm_hash_node_t *temp_hash_node = NULL;

	if (uid < UID_BOUNDARY)
		return DEL_ALLOWED;

	temp_hash_node = acm_hash_search(acm_hash.head, pkgname);
	if (!temp_hash_node)
		return DEL_FORBIDDEN;

	return DEL_ALLOWED;
}

/**
 * acm_search() - search the white list for a package name and collect
 *                delete info to upload to DMD
 * @pkgname:   the package name to search
 * @dentry:    the file dentry to be deleted
 * @uid:       the uid of the task doing the delete operation
 * @file_type: an integer to represent different file types, defined
 *             in fs/f2fs/namei.c
 *
 * Returns 0 if the package name is in the white list, -ENODATA if
 * the package name is not in the white list, and other values on error.
 */
int acm_search(char *pkgname, struct dentry *dentry, uid_t uid, int file_type)
{
	int err = 0, ret = 0;
#ifdef CONFIG_ACM_TIME_COST
	struct timespec start, end;

	getrawmonotonic(&start);
#endif

	if (acm_init_state) {
		PRINT_ERR("Failed to initialize Access Control Module! err = %ld\n", acm_init_state);
		ret = -EINVAL;
		goto acm_search_ret;
	}

	/* Parameter validity check */
	if (!pkgname) {
		PRINT_ERR("Package name was NULL!\n");
		ret = -EINVAL;
		goto acm_search_ret;
	}
	if (valid_len(pkgname, ACM_PKGNAME_MAX_LEN)) {
		PRINT_ERR("Failed to check the length of package name!\n");
		ret = -EINVAL;
		goto acm_search_ret;
	}
	pkgname[ACM_PKGNAME_MAX_LEN - 1] = '\0';

	ret = inquiry_delete_policy(pkgname, uid);
	if (ret != DEL_ALLOWED) {
		ret = delete_log_upload_fwk(pkgname, dentry);
		if (ret != -ENODATA)
			PRINT_ERR("Failed to upload to framework! err = %d\n", ret);
	}

#ifdef CONFIG_ACM_DSM
	err = delete_log_upload_dmd(pkgname, dentry, file_type);
	if (err) {
		PRINT_ERR("Failed to upload to dmd! err = %d\n", err);
		ret = err;
	}
#endif

acm_search_ret:
#ifdef CONFIG_ACM_TIME_COST
	getrawmonotonic(&end);
	PRINT_ERR("TIME_COST: start.tv_sec = %lu start.tv_nsec = %lu end.tv_sec = %lu end.tv_nsec = %lu duraion = %lu\n",
		start.tv_sec, start.tv_nsec, end.tv_sec, end.tv_nsec, end.tv_nsec - start.tv_nsec);
#endif
	PRINT_DEBUG("err = %d, ret = %d\n", err, ret);
	return ret;
}

static char *remain_first_dname(char *path)
{
	int i, len = 0;
	char *pt;

	len = get_valid_strlen(path, ACM_PATH_MAX);

	pt = strstr(path, ".thumbnails");
	if (pt != NULL) {
		memset(pt + strlen(".thumbnails"), 0, len - (size_t)(pt - path) - strlen(".thumbnails"));
		return path;
	}

	for (i = 0; i < len; i++) {
		if (*(path + i) == '/') {
			memset(path + i, 0, len - i);
			break;
		}
	}

	return path;
}

void asterisk_path(char *path)
{
	int i, len = 0;

	len = get_valid_strlen(path, ACM_DNAME_MAX_LEN);

	for (i = 0; i < len; i++)
		*(path + i) = '*';
}

void remove_user_dir(acm_list_node_t *node)
{
	char *path = node->path;
	acm_dir_node_t *dir_node = NULL;

	list_for_each_entry(dir_node, &acm_dir_list.head, lnode) {
		if (strncasecmp(path, dir_node->dir, strlen(dir_node->dir)) == 0) {
			remain_first_dname(path);
			PRINT_DEBUG("path = %s\n", node->path);
			return;
		}

	}

	remain_first_dname(path);

	asterisk_path(path);
}

static void set_dmd_uevent_env(acm_list_node_t *node)
{
	int idx;

	memset(&dsm_env, 0, sizeof(acm_env_t));
	snprintf(dsm_env.pkgname, sizeof(dsm_env.pkgname), "DSM_PKGNAME=%s", node->pkgname);
	snprintf(dsm_env.path, sizeof(dsm_env.path), "DSM_PATH=%s", node->path);
	snprintf(dsm_env.depth, sizeof(dsm_env.depth), "DSM_DEPTH=%d", node->depth);
	snprintf(dsm_env.file_type, sizeof(dsm_env.file_type), "DSM_FTYPE=%d", node->file_type);
	snprintf(dsm_env.nr, sizeof(dsm_env.nr), "DSM_NR=%d", node->nr);

	idx = 0;
	dsm_env.envp[idx++] = dsm_env.pkgname;
	dsm_env.envp[idx++] = dsm_env.path;
	dsm_env.envp[idx++] = dsm_env.depth;
	dsm_env.envp[idx++] = dsm_env.file_type;
	dsm_env.envp[idx++] = dsm_env.nr;
	dsm_env.envp[idx] = NULL;
	PRINT_DEBUG("%s %s %s %s %s\n", dsm_env.pkgname,
		dsm_env.path, dsm_env.depth, dsm_env.file_type, dsm_env.nr);
}

static void set_fwk_uevent_env(acm_list_node_t *node)
{
	int idx;

	memset(&fwk_env, 0, sizeof(acm_env_t));
	snprintf(fwk_env.pkgname, sizeof(fwk_env.pkgname), "PKGNAME=%s", node->pkgname);
	snprintf(fwk_env.path, sizeof(fwk_env.path), "PIC_PATH=%s", node->path);

	idx = 0;
	fwk_env.envp[idx++] = fwk_env.pkgname;
	fwk_env.envp[idx++] = fwk_env.path;
	dsm_env.envp[idx] = NULL;
	PRINT_DEBUG("%s %s\n", fwk_env.pkgname, fwk_env.path);
}

static void upload_delete_log(void)
{
	int i, err = 0;

#ifdef CONFIG_ACM_DSM
	for (i = 0; i < dmd_cache.count; i++) {
		set_dmd_uevent_env(&dmd_cache.cache[i]);
		err = kobject_uevent_env(&(acm_cdev->kobj), KOBJ_CHANGE, dsm_env.envp);
		if (err) {
			PRINT_ERR("Failed to send uevent! err = %d %s %s %s",
				err, dsm_env.pkgname, dsm_env.path, dsm_env.depth);
		}
	}
#endif

	memset(&dmd_cache, 0, sizeof(acm_cache_t));

	/*
	 * Compiler optimization may remove the call to memset(),
	 * causing dmd_cache uncleaned, if dmd_cache is not accessed
	 * after memset(). So we access the count member after memset()
	 * to avoid this optimization.
	 */
	dmd_cache.count = 0;
}

/*
 * Return 1 if in the cache, 0 if NOT in the cache.
 */
static int is_a_cache(acm_list_node_t *node, acm_list_node_t *cache_node)
{
	return (node->depth == cache_node->depth) &&
		(node->file_type == cache_node->file_type) &&
		(strcmp(node->path, cache_node->path) == 0) &&
		(strcmp(node->pkgname, cache_node->pkgname) == 0);
}

static void add_cache(acm_list_node_t *node)
{

	WARN_ON(dmd_cache.count > MAX_CACHED_DELETE_LOG - 1);

	memcpy(&dmd_cache.cache[dmd_cache.count], node, sizeof(acm_list_node_t));
	dmd_cache.cache[dmd_cache.count].nr++;
	dmd_cache.count++;
	PRINT_DEBUG("count = %d, nr = %d\n", dmd_cache.count, dmd_cache.cache[dmd_cache.count - 1].nr);
}

/*
 * Return 1 if in the cache, 0 if NOT in the cache.
 */
static int is_cached(acm_list_node_t *node, int *idx)
{
	int i;

	//spin_lock(&dmd_cache.spinlock);
	for (i = 0; i < dmd_cache.count; i++) {
		if (is_a_cache(node, &dmd_cache.cache[i])) {
			*idx = i;
			return 1;
		}
	}
	//spin_unlock(&dmd_cache.spinlock);
	return 0;
}

static void cache_log(acm_list_node_t *node)
{
	int cached, which = -1;

	cached = is_cached(node, &which);

	if (cached) {
		WARN_ON(which < 0 || which > MAX_CACHED_DELETE_LOG - 1);

		dmd_cache.cache[which].nr++;
		PRINT_DEBUG("Hit cache! path = %s nr = %d\n", dmd_cache.cache[which].path, dmd_cache.cache[which].nr);
	} else {
		add_cache(node);
	}
}

static int cal_nr_slashes(char *str)
{
	int i, len = 0, nr_slashes = 0;

	len = get_valid_strlen(str, ACM_PATH_MAX);

	for (i = 0; i < len; i++) {
		if (*(str + i) == '/')
			nr_slashes++;
	}
	return nr_slashes;
}

static void do_remove_prefix(acm_list_node_t *node, int nr_slashes)
{
	int i, len = 0, slashes = 0;
	char *p = node->path;

	len = get_valid_strlen(node->path, ACM_PATH_MAX);

	for (i = 0; i < len; i++) {
		if (*(p + i) == '/') {
			if (++slashes > nr_slashes) {
				i++;
				break;
			}
		}
	}

	if (i > len - 1) {
		PRINT_ERR("Invalid path syntax! path = %s\n", node->path);
		memset(node->path, 0, sizeof(node->path));
		memcpy(node->path, PATH_UNKNOWN, sizeof(PATH_UNKNOWN));
	} else {
		memcpy(node->path, p + i, len - i);
		memset(p + len - i, 0, i);
	}
	PRINT_DEBUG("node->path = %s", node->path);
}

static void remove_path_prefix(acm_list_node_t *node)
{
	int nr_slashes_in_prefix = 0;

	PRINT_DEBUG("pkgname = %s path = %s\n", node->pkgname, node->path);
	if (!strncmp(node->path, PATH_PREFIX_MEDIA, strlen(PATH_PREFIX_MEDIA))) {
		nr_slashes_in_prefix = cal_nr_slashes(PATH_PREFIX_MEDIA);
	} else if (!strncmp(node->path, PATH_PREFIX_STORAGE_EMULATED,
		strlen(PATH_PREFIX_STORAGE_EMULATED))) {
		nr_slashes_in_prefix = cal_nr_slashes(PATH_PREFIX_STORAGE_EMULATED);
	} else {
		PRINT_ERR("Invalid path prefix! path = %s\n", node->path);
		nr_slashes_in_prefix = ACM_PATH_MAX - 1;
	}

	if (node->depth != DEPTH_INIT) {
		node->depth = node->depth - nr_slashes_in_prefix;
		node->depth--;    /* For user id */
		node->depth--;    /* For control dir */
		node->depth--;    /* For file name*/
	}

	do_remove_prefix(node, nr_slashes_in_prefix + 1);

}

static void set_depth(acm_list_node_t *node)
{
	if (node->depth == DEPTH_INIT)
		node->depth = cal_nr_slashes(node->path) - 1;
}

static void process_delete_log(acm_list_node_t *node)
{
	remove_path_prefix(node);
	set_depth(node);
	remove_user_dir(node);
	cache_log(node);
}

void sleep_if_list_empty(acm_list_t *list)
{
	set_current_state(TASK_INTERRUPTIBLE);
	spin_lock(&list->spinlock);
	if (list_empty(&list->head)) {
		spin_unlock(&list->spinlock);
		schedule();
	} else {
		spin_unlock(&list->spinlock);
	}

	set_current_state(TASK_RUNNING);
}

static acm_list_node_t *get_first_entry(struct list_head *head)
{
	struct list_head *pos;
	acm_list_node_t *node;

	pos = head->next;
	node = (acm_list_node_t *)list_entry(pos, acm_list_node_t, lnode);
	list_del(pos);

	return node;
}

void process_and_upload_delete_log(struct list_head *list)
{
	acm_list_node_t *node;

	while (1) {
		if (list_empty(list))
			break;
		node = get_first_entry(list);

		process_delete_log(node);

		kfree(node);

		if (dmd_cache.count > MAX_CACHED_DELETE_LOG - 1)
			upload_delete_log();
	}
	//msleep(DELETE_LOG_UPLOAD_INTERVAL_MS);
	if (dmd_cache.count > 0)
		upload_delete_log();

	return;
}

static int acm_dmd_loop(void *data)
{
	struct list_head list = LIST_HEAD_INIT(list);
#ifdef CONFIG_ACM_TIME_COST
	struct timespec start, end;
#endif

	while (!kthread_should_stop()) {

		msleep(DELETE_LOG_UPLOAD_INTERVAL_MS);
		spin_lock(&acm_dmd_list.spinlock);
		list_cut_position(&list, &acm_dmd_list.head, acm_dmd_list.head.prev);
		acm_dmd_list.nr_nodes = 0;
		spin_unlock(&acm_dmd_list.spinlock);
#ifdef CONFIG_ACM_TIME_COST
		getrawmonotonic(&start);
#endif

		process_and_upload_delete_log(&list);

#ifdef CONFIG_ACM_TIME_COST
		getrawmonotonic(&end);
		PRINT_ERR("TIME_COST: start.tv_sec = %lu start.tv_nsec = %lu end.tv_sec = %lu end.tv_nsec = %lu duraion = %lu\n",
			start.tv_sec, start.tv_nsec, end.tv_sec, end.tv_nsec, end.tv_nsec - start.tv_nsec);
#endif
		sleep_if_list_empty(&acm_dmd_list);
	}

	return ACM_SUCCESS;
}

acm_static void upload_data_to_fwk(void)
{
	int err = 0;
	acm_list_node_t *node;
#ifdef CONFIG_ACM_TIME_COST
	struct timespec start, end;
#endif

	while (1) {

		spin_lock(&acm_fwk_list.spinlock);
		if (list_empty(&acm_fwk_list.head)) {
			spin_unlock(&acm_fwk_list.spinlock);
			break;
		}
		node = get_first_entry(&acm_fwk_list.head);
		spin_unlock(&acm_fwk_list.spinlock);

#ifdef CONFIG_ACM_TIME_COST
		getrawmonotonic(&start);
#endif
		set_fwk_uevent_env(node);

		err = kobject_uevent_env(&(acm_cdev->kobj), KOBJ_CHANGE, fwk_env.envp);
		if (err)
			PRINT_ERR("Failed to upload data to framework! err = %d %s %s\n",
				err, fwk_env.pkgname, fwk_env.path);

		kfree(node);
#ifdef CONFIG_ACM_TIME_COST
		getrawmonotonic(&end);
		PRINT_ERR("TIME_COST: start.tv_sec = %lu start.tv_nsec = %lu end.tv_sec = %lu end.tv_nsec = %lu duraion = %lu\n",
			start.tv_sec, start.tv_nsec, end.tv_sec, end.tv_nsec, end.tv_nsec - start.tv_nsec);
#endif
	}
}

static int acm_fwk_loop(void *data)
{

	while (!kthread_should_stop()) {

		upload_data_to_fwk();

		sleep_if_list_empty(&acm_fwk_list);
	}
	return ACM_SUCCESS;
}

static const struct file_operations acm_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = acm_ioctl,
};

static int acm_task_init(void)
{
	long err = 0;

	//Create the main task to asynchronously upload data to framework or DMD
	acm_fwk_task = kthread_run(acm_fwk_loop, NULL, "acm_fwk_loop");
	if (IS_ERR(acm_fwk_task)) {
		err = PTR_ERR(acm_fwk_task);
		PRINT_ERR("Failed to create acm fwk task!err = %ld\n", err);
		return err;
	}

	acm_dmd_task = kthread_run(acm_dmd_loop, NULL, "acm_dmd_loop");
	if (IS_ERR(acm_dmd_task)) {
		err = PTR_ERR(acm_dmd_task);
		PRINT_ERR("Failed to create acm dmd task!err = %ld\n", err);
		return err;
	}

	return err;
}

static void acm_cache_init(void)
{

	memset(&dmd_cache.cache, 0, sizeof(dmd_cache.cache));

	dmd_cache.count = 0;
	//spin_lock_init(&dmd_cache.spinlock);
}

static int acm_chr_dev_init(void)
{

	long err = 0;

	//Dynamiclly allocate a device number
	err = alloc_chrdev_region(&acm_devno, ACM_DEV_BASE_MINOR, ACM_DEV_COUNT, ACM_DEV_NAME);
	if (err) {
		PRINT_ERR("Failed to alloc device number! err = %ld\n", err);
		return err;
	}

	//Initialize and add a cdev data structure to kernel
	acm_cdev = cdev_alloc();
	if (!acm_cdev) {
		err = -ENOMEM;
		PRINT_ERR("Failed to allocate memory for cdev data structure! err = %ld\n", err);
		goto free_devno;
	}
	acm_cdev->owner = THIS_MODULE;
	acm_cdev->ops = &acm_fops;
	err = cdev_add(acm_cdev, acm_devno, ACM_DEV_COUNT);
	if (err) {
		PRINT_ERR("Failed to register cdev to kernel! err = %ld\n", err);
		goto free_cdev;
	}

	//Dynamiclly create a device file
	acm_class = class_create(THIS_MODULE, ACM_DEV_NAME);
	if (IS_ERR(acm_class)) {
		err = PTR_ERR(acm_class);
		PRINT_ERR("Failed to create a class! err = %ld\n", err);
		goto free_cdev;
	}
	acm_device = device_create(acm_class, NULL, acm_devno, NULL, ACM_DEV_NAME);
	if (IS_ERR(acm_device)) {
		err = PTR_ERR(acm_device);
		PRINT_ERR("Failed to create a class! err = %ld\n", err);
		goto free_class;
	}

	//Initialize uevent stuff
	acm_kset = kset_create_and_add(ACM_DEV_NAME, NULL, kernel_kobj);
	if (!acm_kset) {
		err = -ENOMEM;
		PRINT_ERR("Failed to create kset! err = %ld\n", err);
		goto free_device;
	}
	acm_cdev->kobj.kset = acm_kset;
	acm_cdev->kobj.uevent_suppress = 0;
	err = kobject_add(&(acm_cdev->kobj), &(acm_kset->kobj), "acm_cdev_kobj");
	if (err) {
		kobject_put(&(acm_cdev->kobj));
		PRINT_ERR("Failed to add kobject to kernel! err = %ld\n", err);
		goto free_kset;
	}

	PRINT_ERR("Initialize acm character device succeed!\n");
	return err;

free_kset:
	kset_unregister(acm_kset);
free_device:
	device_destroy(acm_class, acm_devno);
free_class:
	class_destroy(acm_class);
free_cdev:
	cdev_del(acm_cdev);
free_devno:
	unregister_chrdev_region(acm_devno, ACM_DEV_COUNT);

	PRINT_ERR("Failed to initialize acm character device! err = %ld\n", err);
	return err;
}

static int __init acm_init(void)
{

	long err = 0;

	//Initialize hash table
	err = acm_hash_init();
	if (err) {
		PRINT_ERR("Failed to initialize hash table! err = %ld\n", err);
		goto fail_hash;
	}

	acm_list_init(&acm_dir_list);
	acm_list_init(&acm_fwk_list);
	acm_list_init(&acm_dmd_list);
	acm_cache_init();

	err = acm_task_init();
	if (err) {
		PRINT_ERR("Failed to initialize main task! err = %ld\n", err);
		goto fail_task;
	}

	//Initialize acm character device
	err = acm_chr_dev_init();
	if (err) {
		PRINT_ERR("Failed to initialize acm character device! err = %ld\n", err);
		goto fail_task;
	}

	PRINT_ERR("Initialize ACM moudule succeed!\n");

	acm_init_state = err;
	return err;

fail_task:
	kfree(acm_hash.head);
fail_hash:
	acm_init_state = err;
	return err;
}

static void __exit acm_exit(void)
{

	device_destroy(acm_class, acm_devno);
	class_destroy(acm_class);
	cdev_del(acm_cdev);
	unregister_chrdev_region(acm_devno, ACM_DEV_COUNT);
	kset_unregister(acm_kset);

	PRINT_ERR("Exited from ACM module.\n");
}

MODULE_LICENSE("GPL");
module_init(acm_init);
module_exit(acm_exit);
