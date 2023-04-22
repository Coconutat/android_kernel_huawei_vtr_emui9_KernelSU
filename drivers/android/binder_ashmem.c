#include <linux/fs.h>
#include <linux/mman.h>
#include <linux/uaccess.h>

#include <uapi/linux/android/binder.h>

#include "../staging/android/uapi/ashmem.h"
#include "binder_alloc.h"

#define MAX_VM_FILE_COUNT (3)

static int binder_ashmem_switch_status = BINDER_ASHMEM_SWITCH_UNKNOWN;

static inline void binder_ashmem_recycle_impl(struct binder_ashmem *ashmem,
	size_t size, int err_happend)
{
	long fcnt = file_count(ashmem->vm_file);
	uint64_t flags = BINDER_ASHMEM_COMPLETE;
	loff_t pos;
	ssize_t ret;

	pos = (loff_t)(size - sizeof(uint64_t));
	if (unlikely(pos < 0 || pos > INT_MAX)) {
		pr_err("pos %lld is out of range", (long long)pos);
		goto out;
	}

	if (unlikely(err_happend != 0)) {
		flags = BINDER_ASHMEM_ERROR;
		goto write_flags;
	}

	if (unlikely(fcnt > MAX_VM_FILE_COUNT)) {
		pr_err("Invalid file count of vm_file, %ld, %p\n", fcnt, ashmem->vm_file);
		flags = BINDER_ASHMEM_ERROR;
	}

write_flags:
	ret = __kernel_write(ashmem->vm_file, (char *)&flags,
		sizeof(uint64_t), &pos);
	if (unlikely(ret < 0 || ret != sizeof(uint64_t)))
		pr_err("Failed to write to vm_file, ret = %zd\n", ret);

out:
	fput(ashmem->file);
	ashmem->file = NULL;

	fput(ashmem->vm_file);
	ashmem->vm_file = NULL;
}

static inline bool binder_ashmem_acquire(atomic_t *v, size_t size,
	size_t max_size)
{
	size_t old = atomic_read(v);
	size_t new = old + size;

	while (likely(new <= max_size)) {
		size_t value = atomic_cmpxchg(v, old, new);

		if (likely(value == old))
			return true;

		old = value;
		new = old + size;
	}

	return false;
}

static inline void binder_ashmem_release(atomic_t *v, size_t size)
{
	atomic_sub(size, v);
}

static struct file *binder_get_vm_file(unsigned long addr, unsigned long size)
{
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	struct file *vm_file = NULL;

	mm = get_task_mm(current);
	if (unlikely(!mm)) {
		pr_err("Failed to get mm_struct\n");
		goto out;
	}

	down_read(&mm->mmap_sem); /* [false alarm]:unlikely(!mm) */
	vma = find_exact_vma(mm, addr, addr + size);
	if (unlikely(!vma)) {
		pr_err("Failed to find vma\n");
		goto mmap_sem_up_read;
	}

	if (unlikely((!vma->vm_file))) { /* [false alarm]:unlikely(!vma) */
		pr_err("vm_file is NULL\n");
		goto mmap_sem_up_read;
	}

	vm_file = get_file(vma->vm_file);

mmap_sem_up_read:
	up_read(&mm->mmap_sem);
	mmput(mm);

out:
	return vm_file;
}

void binder_ashmem_translate(struct binder_buffer *buffer)
{
	struct binder_ashmem_header *header;
	struct file *file;
	struct file *vm_file;
	long ret;
	const struct file_operations *fops;
	const size_t h_size = sizeof(struct binder_ashmem_header);

	if (unlikely(binder_ashmem_switch_status <= 0))
		return;

	if (unlikely(!buffer))
		return;

	if (buffer->data_size < h_size) /* [false alarm]:unlikely(!buffer) */
		return;

	/* Check head of data to see if using ashmem */
	header = (struct binder_ashmem_header *)buffer->data;
	if (header->magic != BINDER_ASHMEM_MAGIC_CODE ||
		header->header_size != h_size)
		return;

	/* Check if it is a valid fd */
	file = fget(header->fd);
	if (!file)
		return;

	/* Check if ashmem size is matched */
	fops = file->f_op;
	if (!fops || !fops->unlocked_ioctl) {
		fput(file);
		return;
	}

	ret = fops->unlocked_ioctl(file, ASHMEM_GET_SIZE, 0);
	if (ret < 0 || ret != header->size) {
		fput(file);
		return;
	}

	vm_file = binder_get_vm_file(header->start_addr, header->size);
	if (unlikely(!vm_file)) {
		fput(file);
		return;
	}

	buffer->ashmem.file = file;
	buffer->ashmem.vm_file = vm_file;
}

int binder_ashmem_map(struct binder_alloc *alloc, struct binder_buffer *buffer)
{
	struct binder_ashmem *ashmem;
	struct binder_ashmem_header *header;
	unsigned long addr;

	if (unlikely(!alloc || !buffer))
		return -EINVAL;

	ashmem = &buffer->ashmem;
	if (likely(!ashmem->file))
		return 0;

	header = (struct binder_ashmem_header *)buffer->data;

	if (unlikely(!binder_ashmem_acquire(&alloc->ashmem_size,
		header->size, alloc->max_ashmem_size))) {
		pr_err("Acquire ashmem failed, too many ashmem used");
		goto err_acquire;
	}

	addr = vm_mmap(ashmem->file, 0, header->size, PROT_READ, MAP_SHARED, 0);
	if (unlikely(IS_ERR_VALUE(addr))) {
		pr_err("Failed to map ashmem in kernel space");
		goto err_mmap;
	}

	/* Update head of data */
	header->start_addr = addr;
	header->fd = BINDER_ASHMEM_INVALID_FD;
	return 0;

err_mmap:
	binder_ashmem_release(&alloc->ashmem_size, header->size);

err_acquire:
	return -ENOSPC;
}

void binder_ashmem_unmap(struct binder_alloc *alloc,
	struct binder_buffer *buffer)
{
	struct binder_ashmem *ashmem;
	struct binder_ashmem_header *header;
	int err_happend = 0;

	if (unlikely(!alloc || !buffer))
		return;

	ashmem = &buffer->ashmem;
	if (likely(!ashmem->file))
		return;

	header = (struct binder_ashmem_header *)buffer->data;

	/* Make sure ashmem is already mapped in process of receiver */
	if (likely(BINDER_ASHMEM_INVALID_FD == header->fd)) {
		if (unlikely(vm_munmap(header->start_addr, header->size) < 0)) {
			pr_err("Failed to unmap ashmem\n");
			err_happend = 1;
			goto recycle_ashmem;
		}

		binder_ashmem_release(&alloc->ashmem_size, header->size);
	}

recycle_ashmem:
	binder_ashmem_recycle_impl(ashmem, header->size, err_happend);
}

void binder_ashmem_recycle(struct binder_buffer *buffer)
{
	struct binder_ashmem *ashmem;

	if (unlikely(!buffer))
		return;

	ashmem = &buffer->ashmem;
	if (unlikely(ashmem->file)) {
		struct binder_ashmem_header *header =
			(struct binder_ashmem_header *)buffer->data;

		binder_ashmem_recycle_impl(ashmem, header->size, 0);
	}
}

int binder_ashmem_config(struct binder_alloc *alloc, unsigned long arg)
{
	void __user *ubuf = (void __user *)arg;
	struct binder_ashmem_config cfg;

	if (unlikely(!alloc))
		return -EINVAL;

	if (copy_from_user(&cfg, ubuf, sizeof(cfg)))
		return -EFAULT;

	if (likely(binder_ashmem_switch_status >= 0)) {
		cfg.switch_status = binder_ashmem_switch_status;
		alloc->max_ashmem_size = cfg.max_ashmem_size;
	} else if (cfg.switch_status >= 0) {
		binder_ashmem_switch_status = cfg.switch_status;
		alloc->max_ashmem_size = cfg.max_ashmem_size;
	}

	if (copy_to_user(ubuf, &cfg, sizeof(cfg)))
		return -EFAULT;

	return 0;
}
