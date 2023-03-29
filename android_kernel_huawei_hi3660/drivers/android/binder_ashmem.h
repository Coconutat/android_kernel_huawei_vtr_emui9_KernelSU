#ifndef _LINUX_BINDER_ASHMEM_H
#define _LINUX_BINDER_ASHMEM_H

#include <linux/file.h>

#define BINDER_CONFIG_HUAWEI_ASHMEM _IOR('b', 100, struct binder_ashmem_config)
#define BINDER_ASHMEM_MAGIC_CODE (0x48774173686d656d)
#define BINDER_ASHMEM_COMPLETE (0x434f4d504c455445)
#define BINDER_ASHMEM_ERROR (0x4173684572726F72)

#define BINDER_ASHMEM_SWITCH_ON (1)
#define BINDER_ASHMEM_SWITCH_OFF (0)
#define BINDER_ASHMEM_SWITCH_UNKNOWN (-1)

#define BINDER_ASHMEM_INVALID_FD (-1)

struct binder_ashmem_header {
	uint64_t magic;       /* Magic code for ashmem feature */
	uint16_t header_size; /* Size of this header struct    */
	int16_t  fd;          /* File descriptor of ashmem     */
	uint32_t size;        /* Size of the ashmem            */
	uint64_t start_addr;  /* Compatible with 32bit process */
	uint32_t data_size;   /* Size of data in ashmem        */
	uint32_t buffer_size; /* Size of buffer for hwbinder   */
};

struct binder_ashmem_config {
	int          switch_status;
	unsigned int max_ashmem_size;
};

struct binder_ashmem {
	struct file *file;
	struct file *vm_file;
};

struct binder_alloc;
struct binder_buffer;

void binder_ashmem_translate(struct binder_buffer *buffer);
int binder_ashmem_map(struct binder_alloc *alloc,
	struct binder_buffer *buffer);
void binder_ashmem_unmap(struct binder_alloc *alloc,
	struct binder_buffer *buffer);
void binder_ashmem_recycle(struct binder_buffer *buffer);
int binder_ashmem_config(struct binder_alloc *alloc, unsigned long arg);
#endif /* _LINUX_BINDER_ASHMEM_H */
