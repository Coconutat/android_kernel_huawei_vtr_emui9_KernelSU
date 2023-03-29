#ifndef __ACM_H__
#define __ACM_H__

#define ACM_LOG_TAG "ACM"
#define pr_fmt(fmt) ACM_LOG_TAG ": %s %d " fmt, __func__, __LINE__
#define PRINT_ERR(fmt, ...) printk(KERN_ERR pr_fmt(fmt), ##__VA_ARGS__)
#ifdef CONFIG_ACM_DEBUG
#define PRINT_DEBUG(fmt, ...) printk(KERN_ERR pr_fmt(fmt), ##__VA_ARGS__)
#else
#define PRINT_DEBUG(fmt, ...)
#endif

#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/ioctl.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/hash.h>
#include <asm-generic/unistd.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/path.h>
#include <linux/mount.h>
#include <linux/fs_struct.h>
#include <uapi/linux/limits.h>
#include <uapi/asm-generic/errno.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/timer.h>
#include <linux/acm_f2fs.h>

#define ACM_DEV_NAME "acm"
#define ACM_DEV_BASE_MINOR (0)
#define ACM_DEV_COUNT (1)

#define ACM_MAGIC       'a'
#define ACM_ADD         _IOW(ACM_MAGIC, 0, acm_fwk_pkg_t)
#define ACM_DEL         _IOW(ACM_MAGIC, 1, acm_fwk_pkg_t)
#define ACM_SEARCH      _IOR(ACM_MAGIC, 2, acm_mp_node_t)
#define ACM_ADD_DIR     _IOR(ACM_MAGIC, 3, acm_fwk_dir_t)
#define ACM_DEL_DIR     _IOR(ACM_MAGIC, 4, acm_fwk_dir_t)
#ifdef CONFIG_ACM_DSM
#define ACM_ADD_DSM     _IOR(ACM_MAGIC, 5, acm_mp_node_t)
#define ACM_CMD_MAXNR   (5)
#else
#define ACM_CMD_MAXNR   (4)
#endif

#define ACM_HASH_TABLE_SIZE (512)
#define HASH_TABLE_MAX_SIZE (4096)
#define ACM_PKGNAME_MAX_LEN (100)
#define ACM_PATH_MAX (1024)
#define DEPTH_INIT (-1)
#define ACM_DIR_MAX_LEN (64)
#define ACM_DIR_LIST_MAX_LEN (1024)
#define ACM_DNAME_MAX_LEN (256)
#define ACM_DMD_LIST_MAX_NODES (2048)
#define MAX_CACHED_DELETE_LOG 3
#define DELETE_LOG_UPLOAD_INTERVAL_MS 100

#define PATH_PREFIX_MEDIA "/media"
#define PATH_PREFIX_STORAGE_EMULATED "/storage/emulated"
#define PATH_UNKNOWN "unknown_path"

#define UEVENT_KEY_STR_MAX_LEN (16)
#define ENV_DSM_PKGNAME_MAX_LEN (UEVENT_KEY_STR_MAX_LEN + ACM_PKGNAME_MAX_LEN)
#define ENV_DSM_PATH_MAX_LEN 1024
#define ENV_DSM_NR_STR_MAX_LEN 32
#define ENV_DSM_DEPTH_STR_MAX_LEN 32
#define ENV_DSM_FILE_TYPE_STR_MAX_LEN 32

#define ERR_PATH_MAX_DENTRIES (6)
#define ERR_PATH_LAST_DENTRY (0)

#define UID_BOUNDARY 10000
#define DEL_ALLOWED 0
#define DEL_FORBIDDEN -1
#define ACM_SUCCESS 0

#ifdef CONFIG_ACM_DEBUG
#include <linux/debugfs.h>
#define acm_static
#define EXPORT_FOR_ACM_DEBUG(name) EXPORT_SYMBOL(name)
#else
#define acm_static static
#define EXPORT_FOR_ACM_DEBUG(name)
#endif

/* white list node */
typedef struct acm_hash_node {
	struct hlist_node hnode;
	char pkgname[ACM_PKGNAME_MAX_LEN];
} acm_hash_node_t;

typedef struct acm_hash_table {
	struct hlist_head *head;
	spinlock_t spinlock;
	int nr_nodes;
} acm_hash_table_t;

typedef struct acm_dir_node {
	struct list_head lnode;
	char dir[ACM_DIR_MAX_LEN];
} acm_dir_node_t;

/* data node for framework and DMD */
typedef struct acm_list_node {
	struct list_head lnode;
	char pkgname[ACM_PKGNAME_MAX_LEN];
	char path[ACM_PATH_MAX];
	int file_type;
	int depth;

	/*
	 * Number of deleted files in a period of time,
	 * only used in cache
	 */
	int nr;
} acm_list_node_t;

typedef struct acm_list {
	struct list_head head;
	unsigned long nr_nodes;
	spinlock_t spinlock;
} acm_list_t;

typedef struct acm_cache {
	acm_list_node_t cache[MAX_CACHED_DELETE_LOG];
	int count;
	//spinlock_t spinlock;
} acm_cache_t;

typedef struct acm_env {
	char pkgname[ENV_DSM_PKGNAME_MAX_LEN];
	char path[ENV_DSM_PATH_MAX_LEN];
	char depth[ENV_DSM_DEPTH_STR_MAX_LEN];
	char file_type[ENV_DSM_FILE_TYPE_STR_MAX_LEN];
	char nr[ENV_DSM_NR_STR_MAX_LEN];
	char *envp[UEVENT_NUM_ENVP];
} acm_env_t;

/* package name received from framework */
typedef struct acm_fwk_pkg {
	char pkgname[ACM_PKGNAME_MAX_LEN];
} acm_fwk_pkg_t;

/* directory received from framework */
typedef struct acm_fwk_dir {
	char dir[ACM_DIR_MAX_LEN];
} acm_fwk_dir_t;

/* data received from mediaprovider */
typedef struct acm_mp_node {
	char pkgname[ACM_PKGNAME_MAX_LEN];
	char path[ACM_PATH_MAX];
	int file_type;
	int flag;
} acm_mp_node_t;

acm_static acm_hash_node_t *acm_hash_search(struct hlist_head *hash, char *keystring);
acm_static unsigned int ELFHash(char *str);

acm_static void acm_fwk_add(struct list_head *head, acm_list_node_t *list_node);
acm_static void acm_dmd_add(struct list_head *head, acm_list_node_t *list_node);

acm_static void upload_data_to_fwk(void);

#endif /* __ACM_H__ */
