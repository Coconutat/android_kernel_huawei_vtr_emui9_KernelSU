#include "acm.h"

static struct dentry *acm_debugfs_root;

extern acm_hash_table_t acm_hash;
extern acm_list_t acm_fwk_list;
extern acm_list_t acm_dmd_list;

void acm_hash_cleanup(struct hlist_head *hash)
{
	int i;
	struct hlist_head *phead = NULL;
	acm_hash_node_t *pnode = NULL;

	spin_lock(&acm_hash.spinlock);
	for (i = 0; i < ACM_HASH_TABLE_SIZE; i++) {
		phead = &hash[i];
		while (!hlist_empty(phead)) {
			pnode = hlist_entry(phead->first, acm_hash_node_t, hnode);
			hlist_del(&pnode->hnode);
			kfree(pnode);
		}
	}
	acm_hash.nr_nodes = 0;
	spin_unlock(&acm_hash.spinlock);
}

bool acm_hash_empty(struct hlist_head *hash)
{
	struct hlist_head *phead = NULL;
	bool ret = true;
	int i;

	for (i = 0; i < ACM_HASH_TABLE_SIZE; i++) {
		phead = &hash[i];
		if (!hlist_empty(phead)) {
			ret = false;
			break;
		}
	}

	return ret;
}

void acm_list_cleanup(acm_list_t *list)
{
	acm_list_node_t *temp_list_node;
	struct list_head *head, *pos;

	spin_lock(&list->spinlock);
	head = &list->head;
	while (!list_empty(head)) {
		pos = head->next;
		temp_list_node = (acm_list_node_t *)list_entry(pos, acm_list_node_t, lnode);
		list_del(pos);
		kfree(temp_list_node);
	}
	list->nr_nodes = 0;
	spin_unlock(&list->spinlock);

	return;
}

static int acm_hash_table_open(struct inode *inode, struct file *filp)
{
	int i, err = 0;
	struct hlist_head *phead = NULL;
	struct hlist_head *hash = acm_hash.head;
	acm_hash_node_t *pnode = NULL;

	filp->private_data = inode->i_private;

	for (i = 0; i < ACM_HASH_TABLE_SIZE; i++) {
		PRINT_ERR("===acm_hash[%d]===\n", i);
		phead = &(hash[i]);
		hlist_for_each_entry(pnode, phead, hnode) {
			if (pnode && pnode->pkgname)
				PRINT_ERR("=======PKGNAME=%s\n", pnode->pkgname);
		}
	}
	return err;
}

static int acm_fwk_list_open(struct inode *inode, struct file *filp)
{
	int err = 0;
	acm_list_node_t *pnode = NULL;

	filp->private_data = inode->i_private;

	PRINT_ERR("===acm_fwk_list===\n");
	list_for_each_entry(pnode, &acm_fwk_list.head, lnode) {
		if (pnode)
			PRINT_ERR("=======pkgname=%s \t path=%s\n", pnode->pkgname, pnode->path);
	}

	return err;
}

static int acm_dmd_list_open(struct inode *inode, struct file *filp)
{
	int err = 0;
	acm_list_node_t *pnode = NULL;

	filp->private_data = inode->i_private;

	PRINT_ERR("===acm_dmd_list===\n");
	list_for_each_entry(pnode, &acm_dmd_list.head, lnode) {
		if (pnode)
			PRINT_ERR("=======pkgname=%s \t path=%s \t\n", pnode->pkgname, pnode->path);
	}
	PRINT_ERR("===acm_dmd_list.nrnodes:%lu===\n", acm_dmd_list.nr_nodes);

	return err;
}

static int acm_test_acm_fwk_upload(struct inode *inode, struct file *filp)
{
	acm_list_node_t *list_node;
	acm_list_node_t *tail_node;

	filp->private_data = inode->i_private;

	acm_list_cleanup(&acm_fwk_list);
	if (!list_empty(&acm_fwk_list.head) || acm_fwk_list.nr_nodes != 0) {
		PRINT_ERR("Faile to execute test!\n");
		return -EINVAL;
	}

	PRINT_ERR("===Testing acm_fwk_upload()===\n");
	//Add a node to fwk list, then do upload, List should be empty
	list_node = (acm_list_node_t *)kzalloc(sizeof(acm_list_node_t), GFP_KERNEL);
	if (!list_node) {
		PRINT_ERR("Faile to execute test!\n");
		return -ENOMEM;
	}
	strcpy(list_node->pkgname, "hello");
	strcpy(list_node->path, "h");
	acm_fwk_add(&acm_fwk_list.head, list_node);
	tail_node = (acm_list_node_t *)list_entry(acm_fwk_list.head.prev, acm_list_node_t, lnode);
	if (strcmp(tail_node->pkgname, list_node->pkgname) || strcmp(tail_node->path, list_node->path)) {
		PRINT_ERR("Error!");
		return -EINVAL;
	}
	upload_data_to_fwk();
	if (!list_empty(&acm_fwk_list.head)) {
		PRINT_ERR("Error!");
		return -EINVAL;
	}

	//Add 2 nodes to fwk list, then do upload.List should be empty
	list_node = (acm_list_node_t *)kzalloc(sizeof(acm_list_node_t), GFP_KERNEL);
	if (!list_node) {
		PRINT_ERR("Faile to execute test!\n");
		return -ENOMEM;
	}
	strcpy(list_node->pkgname, "hello");
	strcpy(list_node->path, "h");
	acm_fwk_add(&acm_fwk_list.head, list_node);
	tail_node = (acm_list_node_t *)list_entry(acm_fwk_list.head.prev, acm_list_node_t, lnode);
	if (strcmp(tail_node->pkgname, list_node->pkgname) || strcmp(tail_node->path, list_node->path)) {
		PRINT_ERR("Error!");
		return -EINVAL;
	}

	list_node = (acm_list_node_t *)kzalloc(sizeof(acm_list_node_t), GFP_KERNEL);
	if (!list_node) {
		PRINT_ERR("Faile to execute test!\n");
		return -ENOMEM;
	}
	strcpy(list_node->pkgname, "hello");
	strcpy(list_node->path, "h2");
	acm_fwk_add(&acm_fwk_list.head, list_node);
	tail_node = (acm_list_node_t *)list_entry(acm_fwk_list.head.prev, acm_list_node_t, lnode);
	if (strcmp(tail_node->pkgname, list_node->pkgname) || strcmp(tail_node->path, list_node->path)) {
		PRINT_ERR("Error!");
		return -EINVAL;
	}
	upload_data_to_fwk();
	if (!list_empty(&acm_fwk_list.head)) {
		PRINT_ERR("Error!");
		return -EINVAL;
	}

	PRINT_ERR("===Testing acm_fwk_upload() PASSED!===\n");
	return ACM_SUCCESS;
}

static int acm_test_acm_hash_search(struct inode *inode, struct file *filp)
{
	int idx;
	acm_hash_node_t *temp_hash_node;
	struct hlist_head *phead;
	struct hlist_head *hash = acm_hash.head;
	char teststr1[ACM_PKGNAME_MAX_LEN] = {"com.acm.testacm50"};
	char teststr2[ACM_PKGNAME_MAX_LEN] = {"com.acm.testacm51"};
	char teststr3[ACM_PKGNAME_MAX_LEN] = {"com.acm.testacm52"};

	filp->private_data = inode->i_private;

	//Clean hash table, then add 3nodes
	acm_hash_cleanup(acm_hash.head);
	if (!acm_hash_empty(acm_hash.head)) {
		PRINT_ERR("Error!");
		return -ENODATA;
	}
	temp_hash_node = (acm_hash_node_t *)kzalloc(sizeof(acm_hash_node_t), GFP_KERNEL);
	if (!temp_hash_node) {
		PRINT_ERR("Failed to execute test!\n");
		return -ENOMEM;
	}
	INIT_HLIST_NODE(&temp_hash_node->hnode);
	strcpy(temp_hash_node->pkgname, teststr1);
	temp_hash_node->pkgname[ACM_PKGNAME_MAX_LEN - 1] = '\0';
	idx = ELFHash(temp_hash_node->pkgname);
	spin_lock(&acm_hash.spinlock);
	phead = &hash[idx];
	hlist_add_head(&temp_hash_node->hnode, phead);
	acm_hash.nr_nodes++;
	spin_unlock(&acm_hash.spinlock);

	temp_hash_node = (acm_hash_node_t *)kzalloc(sizeof(acm_hash_node_t), GFP_KERNEL);
	if (!temp_hash_node) {
		PRINT_ERR("Failed to execute test!\n");
		return -ENOMEM;
	}
	INIT_HLIST_NODE(&temp_hash_node->hnode);
	strcpy(temp_hash_node->pkgname, teststr2);
	temp_hash_node->pkgname[ACM_PKGNAME_MAX_LEN - 1] = '\0';
	idx = ELFHash(temp_hash_node->pkgname);
	spin_lock(&acm_hash.spinlock);
	phead = &hash[idx];
	hlist_add_head(&temp_hash_node->hnode, phead);
	acm_hash.nr_nodes++;
	spin_unlock(&acm_hash.spinlock);

	temp_hash_node = (acm_hash_node_t *)kzalloc(sizeof(acm_hash_node_t), GFP_KERNEL);
	if (!temp_hash_node) {
		PRINT_ERR("Failed to execute test!\n");
		return -ENOMEM;
	}
	INIT_HLIST_NODE(&temp_hash_node->hnode);
	strcpy(temp_hash_node->pkgname, teststr3);
	temp_hash_node->pkgname[ACM_PKGNAME_MAX_LEN - 1] = '\0';
	idx = ELFHash(temp_hash_node->pkgname);
	spin_lock(&acm_hash.spinlock);
	phead = &hash[idx];
	hlist_add_head(&temp_hash_node->hnode, phead);
	acm_hash.nr_nodes++;
	spin_unlock(&acm_hash.spinlock);

	PRINT_ERR("===Testing acm_hash_search()===\n");

	//Search for teststr1
	temp_hash_node = acm_hash_search(acm_hash.head, teststr1);
	if (!temp_hash_node) {
		PRINT_ERR("Error!");
		return -EINVAL;
	}
	if (strcmp(temp_hash_node->pkgname, teststr1)) {
		PRINT_ERR("Error!");
		return -EINVAL;
	}
	//Search for teststr2
	temp_hash_node = acm_hash_search(acm_hash.head, teststr2);
	if (!temp_hash_node) {
		PRINT_ERR("Error!");
		return -EINVAL;
	}
	if (strcmp(temp_hash_node->pkgname, teststr2)) {
		PRINT_ERR("Error!");
		return -EINVAL;
	}
	//Search for teststr3
	temp_hash_node = acm_hash_search(acm_hash.head, teststr3);
	if (!temp_hash_node) {
		PRINT_ERR("Error!");
		return -EINVAL;
	}
	if (strcmp(temp_hash_node->pkgname, teststr3)) {
		PRINT_ERR("Error!");
		return -EINVAL;
	}

	PRINT_ERR("===Testing acm_hash_search() PASSED!===\n");
	return ACM_SUCCESS;
}

static int acm_test_ioctl(struct inode *inode, struct file *filp)
{
	int idx;
	acm_hash_node_t *temp_hash_node;
	struct hlist_head *phead;
	struct hlist_head *hash = acm_hash.head;
	char teststr[ACM_PKGNAME_MAX_LEN] = {"com.acm.testacm50"};

	filp->private_data = inode->i_private;

	PRINT_ERR("===Testing acm_ioctl()===\n");
	//Test ACM_ADD
	acm_hash_cleanup(acm_hash.head);
	PRINT_ERR("VAL acm_hash_empty=%d\n", acm_hash_empty(acm_hash.head));
	if (!acm_hash_empty(acm_hash.head)) {
		PRINT_ERR("Error!");
		return -ENODATA;
	}
	temp_hash_node = (acm_hash_node_t *)kzalloc(sizeof(acm_hash_node_t), GFP_KERNEL);
	if (!temp_hash_node) {
		PRINT_ERR("Failed to execute test!\n");
		return -ENOMEM;
	}
	INIT_HLIST_NODE(&temp_hash_node->hnode);
	strcpy(temp_hash_node->pkgname, teststr);
	temp_hash_node->pkgname[ACM_PKGNAME_MAX_LEN - 1] = '\0';
	idx = ELFHash(temp_hash_node->pkgname);
	spin_lock(&acm_hash.spinlock);
	phead = &hash[idx];
	hlist_add_head(&temp_hash_node->hnode, phead);
	acm_hash.nr_nodes++;
	phead = &hash[0];
	temp_hash_node = hlist_entry(phead->first, acm_hash_node_t, hnode);
	spin_unlock(&acm_hash.spinlock);
	if (strcmp(temp_hash_node->pkgname, teststr)) {
		PRINT_ERR("Error!");
		return -EINVAL;
	}

	//Test ACM_SEARCH
	temp_hash_node = acm_hash_search(acm_hash.head, teststr);
	if (!temp_hash_node) {
		PRINT_ERR("Error!");
		return -EINVAL;
	}
	if (strcmp(temp_hash_node->pkgname, teststr)) {
		PRINT_ERR("Error!");
		return -EINVAL;
	}

	//Test ACM_DEL
	temp_hash_node = acm_hash_search(acm_hash.head, teststr);
	if (!temp_hash_node) {
		PRINT_ERR("Error!");
		return -ENODATA;
	}
	hlist_del(&temp_hash_node->hnode);
	acm_hash.nr_nodes--;
	kfree(temp_hash_node);
	PRINT_ERR("VAL acm_hash_empty=%d\n", acm_hash_empty(acm_hash.head));
	if (!acm_hash_empty(acm_hash.head)) {
		PRINT_ERR("Error!");
		return -ENODATA;
	}

	PRINT_ERR("===Testing acm_ioctl() PASSED!===\n");
	return ACM_SUCCESS;
}

int acm_enable;
EXPORT_SYMBOL(acm_enable);
static int acm_enable_open(struct inode *inode, struct file *filp)
{

	filp->private_data = inode->i_private;

	return 0;
}

static ssize_t acm_enable_read(struct file *filp, char __user *buff,
	size_t count, loff_t *offp)
{
	char kbuf[4] = {'\0'};

	snprintf(kbuf, sizeof(kbuf), "%d", acm_enable);
	PRINT_ERR("kbuf = %s acm_enable = %d\n", kbuf, acm_enable);
	if (copy_to_user(buff, kbuf, 4))
		return -EFAULT;

	return 0;
}

static ssize_t acm_enable_write(struct file *filp, const char __user *buff,
	size_t count, loff_t *offp)
{
	char kbuf[4] = {'\0'};

	if (copy_from_user(kbuf, buff, 4))
		return -EFAULT;
	if (!strncmp(kbuf, "1", 1))
		acm_enable = 1;
	else
		acm_enable = 0;
	PRINT_ERR("acm_enable = %d\n", acm_enable);
	return count;
}

ssize_t acm_test_read(struct file *filp, char __user *buff, size_t count, loff_t *offp)
{
	return 0;
}

ssize_t acm_test_write(struct file *filp, const char __user *buff, size_t count, loff_t *offp)
{
	return 0;
}

static const struct file_operations acm_hash_table_file_ops = {
	.owner = THIS_MODULE,
	.open = acm_hash_table_open,
	.read = acm_test_read,
	.write = acm_test_write,
};

static const struct file_operations acm_fwk_list_file_ops = {
	.owner = THIS_MODULE,
	.open = acm_fwk_list_open,
	.read = acm_test_read,
	.write = acm_test_write,
};

static const struct file_operations acm_dmd_list_file_ops = {
	.owner = THIS_MODULE,
	.open = acm_dmd_list_open,
	.read = acm_test_read,
	.write = acm_test_write,
};

static const struct file_operations acm_test_acm_fwk_upload_file_ops = {
	.owner = THIS_MODULE,
	.open = acm_test_acm_fwk_upload,
	.read = acm_test_read,
	.write = acm_test_write,
};

static const struct file_operations acm_test_acm_hash_search_file_ops = {
	.owner = THIS_MODULE,
	.open = acm_test_acm_hash_search,
	.read = acm_test_read,
	.write = acm_test_write,
};

static const struct file_operations acm_test_ioctl_file_ops = {
	.owner = THIS_MODULE,
	.open = acm_test_ioctl,
	.read = acm_test_read,
	.write = acm_test_write,
};

static const struct file_operations acm_enable_file_ops = {
	.owner = THIS_MODULE,
	.open = acm_enable_open,
	.read = acm_enable_read,
	.write = acm_enable_write,
};

static int __init acm_debugfs_init(void)
{
	static struct dentry *acm_debugfs_file_hash_table;
	static struct dentry *acm_debugfs_file_list_for_fwk;
	static struct dentry *acm_debugfs_file_list_for_dmd;
	static struct dentry *acm_debugfs_file_test_acm_fwk_upload;
	static struct dentry *acm_debugfs_file_test_acm_hash_search;
	static struct dentry *acm_debugfs_file_test_ioctl;
	static struct dentry *acm_debugfs_file_enable;

	PRINT_ERR("Initializing debugfs.\n");
	acm_debugfs_root = debugfs_create_dir("acm_debug", NULL);
	if (!acm_debugfs_root) {
		PRINT_ERR("Failed to create acm_debug directory!\n");
		return -EINVAL;
	}

	acm_debugfs_file_hash_table = debugfs_create_file("acm_hash_table",
		0644, acm_debugfs_root, NULL, &acm_hash_table_file_ops);
	if (!acm_debugfs_file_hash_table) {
		PRINT_ERR("Failed to create file acm_hash_table!\n");
		goto free_debugfs;
	}

	acm_debugfs_file_list_for_fwk = debugfs_create_file("acm_fwk_list", 0644, acm_debugfs_root, NULL, &acm_fwk_list_file_ops);
	if (!acm_debugfs_file_list_for_fwk) {
		PRINT_ERR("Failed to create file acm_fwk_list!\n");
		goto free_debugfs;
	}

	acm_debugfs_file_list_for_dmd = debugfs_create_file("acm_dmd_list", 0644, acm_debugfs_root, NULL, &acm_dmd_list_file_ops);
	if (!acm_debugfs_file_list_for_dmd) {
		PRINT_ERR("Failed to create file acm_dmd_list!\n");
		goto free_debugfs;
	}

	acm_debugfs_file_test_acm_fwk_upload = debugfs_create_file("acm_test_acm_fwk_upload", 0644, acm_debugfs_root, NULL, &acm_test_acm_fwk_upload_file_ops);
	if (!acm_debugfs_file_test_acm_fwk_upload) {
		PRINT_ERR("Failed to create file acm_test_acm_fwk_upload!\n");
		goto free_debugfs;
	}

	acm_debugfs_file_test_acm_hash_search = debugfs_create_file("acm_test_acm_hash_search", 0644, acm_debugfs_root, NULL, &acm_test_acm_hash_search_file_ops);
	if (!acm_debugfs_file_test_acm_hash_search) {
		PRINT_ERR("Failed to create file acm_test_acm_hash_search!\n");
		goto free_debugfs;
	}

	acm_debugfs_file_test_ioctl = debugfs_create_file("acm_test_ioctl", 0644, acm_debugfs_root, NULL, &acm_test_ioctl_file_ops);
	if (!acm_debugfs_file_test_ioctl) {
		PRINT_ERR("Failed to create file acm_test_ioctl!\n");
		goto free_debugfs;
	}

	acm_debugfs_file_enable = debugfs_create_file("acm_enable",
		0644, acm_debugfs_root, NULL, &acm_enable_file_ops);
	if (!acm_debugfs_file_enable) {
		PRINT_ERR("Failed to create file acm_enable!\n");
		goto free_debugfs;
	}

	return ACM_SUCCESS;

free_debugfs:
	debugfs_remove_recursive(acm_debugfs_root);
	return -EINVAL;
}

static void __exit acm_debugfs_exit(void)
{
	debugfs_remove_recursive(acm_debugfs_root);
}

MODULE_LICENSE("GPL");
module_init(acm_debugfs_init);
module_exit(acm_debugfs_exit);
