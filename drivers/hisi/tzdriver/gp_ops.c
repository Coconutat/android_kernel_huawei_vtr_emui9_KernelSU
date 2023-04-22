#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/version.h>
#include <asm/memory.h>
#include <linux/hisi/hisi_ion.h>
#include <linux/ion.h>
#include "securec.h"

#include "smc.h"
#include "teek_client_constants.h"
#include "tc_ns_client.h"
#include "teek_ns_client.h"
#include "agent.h"
#include "tc_ns_log.h"
#include "gp_ops.h"
#include "mem.h"
#include "mailbox_mempool.h"
#include "dynamic_mem.h"
#include "tlogger.h"
#include "cfc.h"

#include "security_auth_enhance.h"
struct session_secure_info g_cur_session_secure_info;
#define AES_LOGIN_MAXLEN   ((MAX_PUBKEY_LEN) > (MAX_PACKAGE_NAME_LEN) \
	? (MAX_PUBKEY_LEN) : (MAX_PACKAGE_NAME_LEN))
static int do_encryption(uint8_t *buffer, uint32_t buffer_size,
			 uint32_t payload_size);
static int encrypt_login_info(uint8_t flags, uint32_t cmd_id, int32_t index,
                              uint32_t login_info_size, uint8_t *buffer);

#define MAX_SHARED_SIZE 0x100000	/* 1 MiB */

#define TEEC_PARAM_TYPES(param0Type, param1Type, param2Type, param3Type) \
	((param3Type) << 12 | (param2Type) << 8 | \
	(param1Type) << 4 | (param0Type))

#define TEEC_PARAM_TYPE_GET(paramTypes, index) \
	(((paramTypes) >> (4*(index))) & 0x0F)
#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))

static int free_operation(TC_NS_ClientContext *client_context,
		TC_NS_Operation *operation,
		TC_NS_Temp_Buf local_temp_buffer[4]);

int tc_user_param_valid(TC_NS_ClientContext *client_context, int n)
{
	TC_NS_ClientParam *client_param;
	unsigned int param_type;

	if (!client_context) {
		tloge("client_context is null.\n");
		return -EINVAL;
	}

	if ((n < 0) || (n > 3)) {
		tloge("n is invalid.\n");
		return -EINVAL;
	}

	client_param = &(client_context->params[n]);
	param_type = TEEC_PARAM_TYPE_GET(client_context->paramTypes, n);
	tlogd("Param %d type is %x\n", n, param_type);

	if (TEEC_NONE == param_type) {
		tlogd("param_type is TEEC_NONE.\n");
		return 0;
	}

	/* temp buffers we need to allocate/deallocate for every
	 * operation */
	if ((TEEC_MEMREF_TEMP_INPUT == param_type) ||
	    (TEEC_MEMREF_TEMP_OUTPUT == param_type) ||
	    (TEEC_MEMREF_TEMP_INOUT  == param_type) ||
	    (TEEC_MEMREF_PARTIAL_INPUT == param_type) ||
	    (TEEC_MEMREF_PARTIAL_OUTPUT == param_type) ||
	    (TEEC_MEMREF_PARTIAL_INOUT == param_type)) {
		uint32_t size;
		/* Check the size and buffer addresses
		 * have valid userspace addresses */
		if (!access_ok(VERIFY_READ, client_param->memref.size_addr,
			       sizeof(uint32_t)))
			return -EFAULT;

		get_user(size, (uint32_t *) client_param->memref.size_addr);
		/* Check if the buffer address is valid user space address */
		if (!access_ok(VERIFY_READ, client_param->memref.buffer, size))
			return -EFAULT;
	}
	/* value */
	else if ((TEEC_VALUE_INPUT == param_type) ||
		 (TEEC_VALUE_OUTPUT == param_type) ||
		 (TEEC_VALUE_INOUT == param_type) ||
		 (TEEC_ION_INPUT == param_type)) {
		if (!access_ok
		    (VERIFY_READ, client_param->value.a_addr, sizeof(uint32_t)))
			return -EFAULT;
		if (!access_ok
		    (VERIFY_READ, client_param->value.b_addr, sizeof(uint32_t)))
			return -EFAULT;
	} else {
		tloge("param_types is not supported.\n");
		return -EFAULT;
	}
	return 0;
}

/* These 2 functions handle copying from client. Because client here can be
 * kernel client or user space client, we must use the proper copy function */
static inline int copy_from_client(void *dest, void __user *src, size_t size,
				   uint8_t kernel_api)
{
	int ret = 0;
	int s_ret = 0;

	if ((!dest) || (!src)) {
		tloge("src or dest is NULL input buffer\n");
		return -EINVAL;
	}
	/*
	 * size is checked before calling copy_from_client
	 * to be sure that size is <= dest's buffer size.
	 */
	if (0 == size)
		return 0;

	if (kernel_api) {
		/* Source is kernel valid address */
		if ((virt_addr_valid(src) || vmalloc_addr_valid(src) || modules_addr_valid(src))) { /*lint !e648 */
			s_ret = memcpy_s(dest, size, src, size);
			if (EOK != s_ret) {
				tloge("copy_from_client _s fail. line=%d, s_ret=%d.\n",
						__LINE__, s_ret);
				return s_ret;
			} else {
				return 0;
			}
		} else {
			tloge("copy_from_client check kernel addr %llx failed.\n", (uint64_t)src);
			return  -EFAULT;
		}
	} else {
		/* buffer is in user space(CA call TEE API) */
		if (copy_from_user(dest, src, size)) {
			tloge("copy_from_user failed.\n");
			return -EFAULT;
		} else {
			ret = 0;
		}
	}

	return ret;
}

static inline int copy_to_client(void __user *dest, void *src, size_t size,
				 uint8_t kernel_api)
{
	int ret = 0;
	int s_ret = 0;

	if ((!dest) || (!src)) {
		tloge("src or dest is NULL input buffer\n");
		return -EINVAL;
	}
	if (0 == size)
		return ret;

	/* Dest is kernel valid address */
	if (kernel_api) {
		if ((virt_addr_valid(dest) || vmalloc_addr_valid(dest) || modules_addr_valid(dest))) { /*lint !e648 */
			s_ret = memcpy_s(dest, size, src, size);
			if (EOK != s_ret) {
				tloge("copy_to_client _s fail. line=%d, s_ret=%d.\n",
					__LINE__, s_ret);
				return s_ret;
			}
		} else {
			tloge("copy_to_client check dst addr %llx failed.\n", (uint64_t)dest);
			return -EFAULT;
		}
	} else {
		/* buffer is in user space(CA call TEE API) */
		if (copy_to_user(dest, src, size))
			ret = -EFAULT;
	}
	return ret;
}

static int alloc_operation(TC_NS_DEV_File *dev_file,
					TC_NS_Operation *operation,
					TC_NS_ClientContext *client_context,
					TC_NS_Temp_Buf local_temp_buffer[4],
					uint8_t flags)
{
	TC_NS_ClientParam *client_param;
	TC_NS_Shared_MEM *shared_mem = NULL;
	ion_phys_addr_t drm_ion_phys = 0x0;
	size_t drm_ion_size = 0;
	int ret = 0;
	unsigned int param_type;
	int i;
	uint32_t buffer_size = 0;
	void *temp_buf;
	unsigned int trans_paramtype_to_tee[4];
	uint8_t kernel_params;

	if (!dev_file) {
		tloge("dev_file is null");
		return -EINVAL;
	}

	if (!operation) {
		tloge("operation is null\n");
		return -EINVAL;
	}

	if (!client_context) {
		tloge("client_context is null");
		return -EINVAL; /*lint !e747 */
	}

	if (!local_temp_buffer) {
		tloge("local_temp_buffer is null");
		return -EINVAL;
	}

	if (!client_context->paramTypes) {
		tloge("invalid param type\n");
		return -EINVAL;
	}

	kernel_params = dev_file->kernel_api;

	tlogd("Allocating param types %08X\n", client_context->paramTypes);
	/* Get the 4 params from the client context */
	for (i = 0; i < 4; i++) {
		/*
		 * Normally kernel_params = kernel_api
		 *
		 * But when TC_CALL_LOGIN, params 2/3 will
		 * be filled by kernel. so under this circumstance,
		 * params 2/3 has to be set to kernel mode; and
		 * param 0/1 will keep the same with kernel_api.
		 */
		if ((flags & TC_CALL_LOGIN) && (i >= 2)) {
			kernel_params = TEE_REQ_FROM_KERNEL_MODE;
		}

		client_param = &(client_context->params[i]);

		param_type = TEEC_PARAM_TYPE_GET(client_context->paramTypes, i);
		tlogd("Param %d type is %x\n", i, param_type);
		/* temp buffers we need to allocate/deallocate for every
		 * operation */
		if (teec_tmpmem_type(param_type, 2)) {
			/* For interface compatibility sake we assume buffer
			 * size to be 32bits */
			if (copy_from_client(&buffer_size,
					     (uint32_t __user *)
					     client_param->memref.size_addr,
					     sizeof(uint32_t), kernel_params)) {
				tloge("copy memref.size_addr failed\n");
				ret = -EFAULT;
				break;
			}
			/* Don't allow unbounded malloc requests */
			if (buffer_size > MAX_SHARED_SIZE) {
				tloge("buffer_size %u from user is too large\n",
				      buffer_size);
				ret = -EFAULT;
				break;
			}

			temp_buf = (void *)mailbox_alloc(buffer_size, MB_FLAG_ZERO); /*lint !e647 */
			/* If buffer size is zero or malloc failed */
			if (!temp_buf) {
				tloge("temp_buf malloc failed, i = %d.\n", i);
				ret = -ENOMEM;
				break;
			} else {
				tlogd("temp_buf malloc ok, i = %d.\n", i);
			}
			local_temp_buffer[i].temp_buffer = temp_buf;
			local_temp_buffer[i].size = buffer_size;
			if ((TEEC_MEMREF_TEMP_INPUT == param_type) ||
			    (TEEC_MEMREF_TEMP_INOUT == param_type)) {
				tlogv("client_param->memref.buffer=0x%llx\n",
					  client_param->memref.buffer);
				/* Kernel side buffer */
				if (copy_from_client(temp_buf,
						     (void *)
						     client_param->memref.
						     buffer, buffer_size,
						     kernel_params)) {
					tloge("copy memref.buffer failed\n");
					ret = -EFAULT;
					break;
				}
			}
			ret = encrypt_login_info(flags, client_context->cmd_id,
					i, buffer_size, temp_buf);
			if (0 != ret) {
				tloge("SECURITY_AUTH_ENHANCE:encry failed\n");
				break;
			}
			operation->params[i].memref.buffer = virt_to_phys((void *)temp_buf);
			operation->buffer_h_addr[i] = virt_to_phys((void *)temp_buf) >> 32;
			operation->params[i].memref.size = buffer_size;
			/*TEEC_MEMREF_TEMP_INPUT equal
			 * to TEE_PARAM_TYPE_MEMREF_INPUT*/
			trans_paramtype_to_tee[i] = param_type;
		}
		/* MEMREF_PARTIAL buffers are already allocated so we just need
		 * to search for the shared_mem ref */
		else if (teec_memref_type(param_type, 2)) {
			/* For interface compatibility sake we assume buffer
			 * size to be 32bits */
			if (copy_from_client(&buffer_size,
					     (uint32_t __user *)
					     client_param->memref.size_addr,
					     sizeof(buffer_size),
					     kernel_params)) {
				tloge("copy memref.size_addr failed\n");
				ret = -EFAULT;
				break;
			}
			if (!buffer_size) {
				tloge("buffer_size from user is 0\n");
				ret = -ENOMEM;
				break;
			}
			operation->params[i].memref.buffer = 0;
			/* find kernel addr refered to user addr */
			mutex_lock(&dev_file->shared_mem_lock);
			list_for_each_entry(shared_mem,
					    &dev_file->shared_mem_list, head) {
				if (shared_mem->user_addr ==
				    (void *)client_param->memref.buffer) {
					/* arbitrary CA can control offset by ioctl, so in here
					 * offset must be checked, and avoid integer overflow. */
					if (((shared_mem->len - client_param->memref.offset) >= buffer_size)
					    && (shared_mem->len > client_param->memref.offset)) {
						void *buffer_addr = (void *)((unsigned long)shared_mem->kernel_addr
							+ client_param->memref.offset);

						if (!shared_mem->from_mailbox) {
							buffer_addr = mailbox_copy_alloc(buffer_addr, buffer_size);
							if (!buffer_addr) {
								tloge("alloc mailbox copy failed\n");
								ret = -ENOMEM;
								break;
							}
							operation->mb_buffer[i] = buffer_addr;
						}

						operation->params[i].memref.buffer = virt_to_phys(buffer_addr);
						operation->buffer_h_addr[i] = virt_to_phys(buffer_addr) >> 32; /*lint !e572*/
						/* save shared_mem in operation so that we can use it
						 * while free_operation */
						operation->sharemem[i] = shared_mem;
						get_sharemem_struct(shared_mem);
					} else {
						tloge("Unexpected size %u vs %u",
						      shared_mem->len,
						      buffer_size);
					}
					break;
				}
			}
			mutex_unlock(&dev_file->shared_mem_lock);
			/* for 8G physical memory device, there is a chance that
			 * operation->params[i].memref.buffer could be all 0,
			 * buffer_h_addr cannot be 0 in the same time. */
			if ((!operation->params[i].memref.buffer)
			    && (!operation->buffer_h_addr[i])) {
				tloge("can not find shared buffer, exit\n");
				ret = -EINVAL;
				break;
			}
			operation->params[i].memref.size = buffer_size;
			/* TEEC_MEMREF_PARTIAL_INPUT -
			 * TEE_PARAM_TYPE_MEMREF_INPUT = 8 */
			trans_paramtype_to_tee[i] =
				param_type - (TEEC_MEMREF_PARTIAL_INPUT -
					      TEE_PARAM_TYPE_MEMREF_INPUT);
		}
		/* value */
		else if (teec_value_type(param_type, 2)) {
			if (copy_from_client(&operation->params[i].value.a,
					     client_param->value.a_addr,
					     sizeof(operation->params[i].
						    value.a), kernel_params)) {
				tloge("copy value.a_addr failed\n");
				ret = -EFAULT;
				break;
			}
			if (copy_from_client(&operation->params[i].value.b,
					     client_param->value.b_addr,
					     sizeof(operation->params[i].
						    value.b), kernel_params)) {
				tloge("copy value.b_addr failed\n");
				ret = -EFAULT;
				break;
			}
			/* TEEC_VALUE_INPUT equal
			 * to TEE_PARAM_TYPE_VALUE_INPUT */
			trans_paramtype_to_tee[i] = param_type;
		}
		/*ion*/
		else if (TEEC_ION_INPUT == param_type) {
			if (copy_from_client(&operation->params[i].value.a,
					     client_param->value.a_addr,
					     sizeof(operation->params[i].value.a),
					     kernel_params)) {
				tloge("value.a_addr copy failed\n");
				ret = -EFAULT;
				break;
			}

			if (copy_from_client(&operation->params[i].value.b,
					     client_param->value.b_addr,
					     sizeof(operation->params[i].value.b),
					     kernel_params)) {
				tloge("value.b_addr copy failed\n");
				ret = -EFAULT;
				break;
			}

			if ((int)operation->params[i].value.a >= 0) {
				unsigned int ion_shared_fd =
					operation->params[i].value.a;
				struct ion_handle *drm_ion_handle =
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0))
					ion_import_dma_buf_fd(drm_ion_client,
							   ion_shared_fd);
#else
					ion_import_dma_buf(drm_ion_client,
							   ion_shared_fd);
#endif
				if (IS_ERR(drm_ion_handle)) {
					tloge("in %s err: fd=%d\n",
					      __func__, ion_shared_fd);
					ret = -EFAULT;
					break;
				}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0))
				ret = ion_secmem_get_phys(drm_ion_client, drm_ion_handle,
					       &drm_ion_phys, &drm_ion_size);
#else
				ret = ion_phys(drm_ion_client, drm_ion_handle,
					       &drm_ion_phys, &drm_ion_size);
#endif

				if (ret) {
					ion_free(drm_ion_client, drm_ion_handle);
					tloge("in %s err:ret=%d fd=%d\n",
					      __func__, ret, ion_shared_fd);
					ret = -EFAULT;
					break;
				}

				if (drm_ion_size > operation->params[i].value.b)
					drm_ion_size = operation->params[i].value.b;

				operation->params[i].memref.buffer =
					(unsigned int)drm_ion_phys;
				operation->params[i].memref.size =
					(unsigned int)drm_ion_size;
				trans_paramtype_to_tee[i] = param_type;
				ion_free(drm_ion_client, drm_ion_handle);
			} else {
				tloge("in %s err: drm ion handle invaild!\n", __func__);
				ret = -EFAULT;
				break;
			}
		} else {
			tlogd("param_type = TEEC_NONE\n");
			trans_paramtype_to_tee[i] = TEE_PARAM_TYPE_NONE;
		}
	}
	if (!ret) {
		operation->paramTypes =
			TEEC_PARAM_TYPES(trans_paramtype_to_tee[0],
					 trans_paramtype_to_tee[1],
					 trans_paramtype_to_tee[2],
					 trans_paramtype_to_tee[3]);
		return 0;
	}

	(void)free_operation(client_context, operation, local_temp_buffer);
	return ret;
}

static int update_client_operation(TC_NS_DEV_File *dev_file,
				   TC_NS_ClientContext *client_context,
				   TC_NS_Operation *operation,
				   TC_NS_Temp_Buf local_temp_buffer[4],
				   bool incomplete)
{
	TC_NS_ClientParam *client_param;
	int ret = 0;
	unsigned int param_type;
	int i;
	uint32_t buffer_size = 0;

	if (!dev_file) {
		tloge("dev_file is null");
		return -EINVAL;
	}

	if (!client_context) {
		tloge("client_context is null");
		return -EINVAL;
	}

	if (!local_temp_buffer) {
		tloge("local_temp_buffer is null");
		return -EINVAL;
	}

	/*if paramTypes is NULL, no need to update*/
	if (!client_context->paramTypes)
		return 0;

	for (i = 0; i < 4; i++) {
		client_param = &(client_context->params[i]);
		param_type = TEEC_PARAM_TYPE_GET(client_context->paramTypes, i);
		if (teec_tmpmem_type(param_type, 1)) {
			/* temp buffer */
			buffer_size = operation->params[i].memref.size;
			/* Size is updated all the time */
			if (copy_to_client
			    ((void *)client_param->memref.size_addr,
			     &buffer_size, sizeof(buffer_size),
			     dev_file->kernel_api)) {
				tloge("copy tempbuf size failed\n");
				ret = -EFAULT;
				break;
			}

			/* Only update the buffer is the buffer size is valid in
			 * incomplete case, otherwise see next param */
			if (buffer_size > local_temp_buffer[i].size) {
				if (!incomplete)
					continue;
				else {
					tloge("client_param->memref.size has been changed larger than the initial\n");
					ret = -EFAULT;
				}
				break;
			}

			if (copy_to_client((void *)client_param->memref.buffer,
					   local_temp_buffer[i].temp_buffer,
					   operation->params[i].memref.size,
					   dev_file->kernel_api)) {
				tloge("copy tempbuf failed\n");
				ret = -ENOMEM;
				break;
			}

		} else if (teec_memref_type(param_type, 1)) {
			unsigned int orig_size = 0;
			/* update size */
			buffer_size = operation->params[i].memref.size;

			if (copy_from_client(&orig_size,
					(uint32_t __user *)client_param->memref.size_addr,
					sizeof(orig_size),
					dev_file->kernel_api)) {
				tloge("copy orig memref.size_addr failed\n");
				ret = -EFAULT;
				break;
			}

			if (copy_to_client
				((void *)client_param->memref.size_addr,
				 &buffer_size, sizeof(buffer_size),
				 dev_file->kernel_api)) {
				tloge("copy buf size failed\n");
				ret = -EFAULT;
				break;
			}

			/* copy from mb_buffer to sharemem */
			if (!operation->sharemem[i]->from_mailbox && operation->mb_buffer[i]
				&& orig_size >= buffer_size) {
				void *buffer_addr = (void *)((unsigned long)operation->sharemem[i]->kernel_addr
					+ client_param->memref.offset);
				if (memcpy_s(buffer_addr,
						operation->sharemem[i]->len - client_param->memref.offset,
						operation->mb_buffer[i], buffer_size)) {
					tloge("copy to sharemem failed\n");
					ret = -EFAULT;
					break;
				}
			}
		} else if (incomplete && teec_value_type(param_type, 1)) {
			/* value */
			if (copy_to_client(client_param->value.a_addr,
					   &operation->params[i].value.a,
					   sizeof(operation->params[i].value.a),
					   dev_file->kernel_api)) {
				tloge("inc copy value.a_addr failed\n");
				ret = -EFAULT;
				break;
			}
			if (copy_to_client(client_param->value.b_addr,
					   &operation->params[i].value.b,
					   sizeof(operation->params[i].value.b),
					   dev_file->kernel_api)) {
				tloge("inc copy value.b_addr failed\n");
				ret = -EFAULT;
				break;
			}
		} else {
			tlogd("param_type:%d don't need to update.\n", param_type);
		}
	}
	return ret;
}

static int free_operation(TC_NS_ClientContext *client_context,
			  TC_NS_Operation *operation,
			  TC_NS_Temp_Buf local_temp_buffer[4])
{
	int ret = 0;
	unsigned int param_type;
	int i;
	void *temp_buf;

	if (!operation)
		return -EFAULT;

	if (!client_context)
		return -EFAULT;

	if (!local_temp_buffer) {
		tloge("local_temp_buffer is null");
		return -EINVAL;
	}

	for (i = 0; i < 4; i++) {
		param_type = TEEC_PARAM_TYPE_GET(client_context->paramTypes, i);
		if ((TEEC_MEMREF_TEMP_INPUT == param_type) ||
		    (TEEC_MEMREF_TEMP_OUTPUT == param_type) ||
		    (TEEC_MEMREF_TEMP_INOUT == param_type)) {
			/* free temp buffer */
			/* TODO: this is all sorts of bad */
			temp_buf = local_temp_buffer[i].temp_buffer;
			tlogd("Free temp buf, i = %d\n", i);
			if (virt_addr_valid(temp_buf) && /*lint !e648 */
			    !ZERO_OR_NULL_PTR(temp_buf))
				mailbox_free(temp_buf);
		} else if ((TEEC_MEMREF_PARTIAL_INPUT == param_type) ||
			 (TEEC_MEMREF_PARTIAL_OUTPUT == param_type) ||
			 (TEEC_MEMREF_PARTIAL_INOUT == param_type)) {
			put_sharemem_struct(operation->sharemem[i]);
			if (operation->mb_buffer[i])
				mailbox_free(operation->mb_buffer[i]);
		}
	}
	return ret;
}

unsigned char g_ca_auth_hash_buf[HASH_PLAINTEXT_ALIGNED_SIZE + IV_BYTESIZE];

static int32_t save_token_info(void *dst_teec, uint8_t *src_buf,
					uint8_t kernel_api) {
	uint8_t temp_teec_token[TOKEN_SAVE_LEN];

	if ((!dst_teec) || (!src_buf)) {
		tloge("dst data or src data is invalid.\n");
		return -EINVAL;
	}
	/* copy libteec_token && timestamp to libteec */
	if (EOK != memmove_s(temp_teec_token, TIMESTAMP_SAVE_INDEX,
				src_buf, TIMESTAMP_SAVE_INDEX)) {
		tloge("copy teec token failed.\n");
		return -EFAULT;
	}

	if (EOK != memmove_s(&temp_teec_token[TIMESTAMP_SAVE_INDEX],
				TIMESTAMP_LEN_DEFAULT,
				&src_buf[TIMESTAMP_BUFFER_INDEX],
				TIMESTAMP_LEN_DEFAULT)) {
		tloge("copy teec timestamp failed.\n");
		return -EFAULT;
	}

	/* copy libteec_token to libteec */
	if (EOK != copy_to_client(dst_teec, temp_teec_token,
				TOKEN_SAVE_LEN, kernel_api)) {
		tloge("copy teec token & timestamp failed.\n");
		return -EFAULT;
	}

	/* clear libteec(16byte) */
	if (EOK != memset_s(src_buf, TIMESTAMP_SAVE_INDEX,
				0, TIMESTAMP_SAVE_INDEX)) {
		tloge("clear libteec failed.\n");
		return -EFAULT;
	}

	return EOK;
}
/*lint -e613*/
static int fill_token_info(TC_NS_SMC_CMD *smc_cmd,
			TC_NS_ClientContext *client_context,
			TC_NS_Token *tc_ns_token,
			TC_NS_DEV_File *dev_file, bool global,
			uint8_t *mb_pack_token, uint32_t mb_pack_token_size) {
	uint8_t temp_libteec_token[TOKEN_SAVE_LEN] = {0};
	errno_t ret_s;
	int paramcheck = (!smc_cmd) || (!client_context)
		|| (!dev_file) || (!tc_ns_token)
		|| (!mb_pack_token) || (mb_pack_token_size < TOKEN_BUFFER_LEN);
	if (paramcheck) {
		tloge("in parameter is ivalid.\n");
		return -EFAULT;
	}

	if ((!client_context->teec_token) || (!tc_ns_token->token_buffer)) {
		tloge("teec_token or token_buffer is NULL, error!\n");
		return -EFAULT;
	}

	if ((client_context->cmd_id == GLOBAL_CMD_ID_CLOSE_SESSION)
		|| (!global)) {
		if (copy_from_client(temp_libteec_token,
					client_context->teec_token,
					TOKEN_SAVE_LEN,
					dev_file->kernel_api)) {
			tloge("copy libteec token failed!\n");
			return -EFAULT;
		}

		if (memcmp(&temp_libteec_token[TIMESTAMP_SAVE_INDEX],
			&tc_ns_token->token_buffer[TIMESTAMP_BUFFER_INDEX],
			TIMESTAMP_LEN_DEFAULT)) {
			tloge("timestamp compare failed!\n");
			return -EFAULT;
		}

		/* combine token_buffer */
		/* teec_token, 0-15byte */
		if (EOK != memmove_s(tc_ns_token->token_buffer,
					TIMESTAMP_SAVE_INDEX,
					temp_libteec_token,
					TIMESTAMP_SAVE_INDEX)) {
			tloge("copy buffer failed!\n");
			ret_s = memset_s(tc_ns_token->token_buffer, TOKEN_BUFFER_LEN,
					0, TOKEN_BUFFER_LEN);
			if (ret_s != EOK) {
				tloge("memset_s buffer error=%d\n", ret_s);
			}
			return -EFAULT;
		}

		/* kernal_api, 40byte */
		if (EOK != memmove_s((tc_ns_token->token_buffer + KERNAL_API_INDEX),
				KERNAL_API_LEN, &dev_file->kernel_api,
				KERNAL_API_LEN)) {
			tloge("copy KERNAL_API_LEN failed!\n");
			ret_s = memset_s(tc_ns_token->token_buffer, TOKEN_BUFFER_LEN,
					0, TOKEN_BUFFER_LEN);
			if (ret_s != EOK) {
				tloge("fill buffer memset_s error=%d\n", ret_s);
			}
			return -EFAULT;
		}
	} else { /* open_session, set token_buffer 0 */
		if (EOK != memset_s(tc_ns_token->token_buffer, TOKEN_BUFFER_LEN,
				0, TOKEN_BUFFER_LEN)) {
			tloge("alloc tc_ns_token->token_buffer error.\n");
			return -EFAULT;
		}
	}

	/* TODO:add 1 memcpy */
	if (memcpy_s(mb_pack_token, mb_pack_token_size,
			tc_ns_token->token_buffer, TOKEN_BUFFER_LEN)) {
		tloge("copy token failed\n");
		return -EFAULT;
	}

	smc_cmd->pid = current->tgid;
	smc_cmd->token_phys = virt_to_phys(mb_pack_token);
	smc_cmd->token_h_phys = virt_to_phys(mb_pack_token) >> 32; /*lint !e572*/

	return EOK;
}
/*lint +e613*/

static int load_security_enhance_info(TC_NS_SMC_CMD *smc_cmd,
			TC_NS_ClientContext *client_context,
			TC_NS_Token *tc_ns_token, TC_NS_DEV_File *dev_file,
			struct mb_cmd_pack *mb_pack,
			bool global, bool is_token_work) {
	int ret = 0;
	bool is_open_session_cmd = false;

	if (!smc_cmd || !mb_pack) {
		tloge("in parameter is invalid.\n");
		return -EFAULT;
	}

	if (is_token_work) {
		ret = fill_token_info(smc_cmd, client_context, tc_ns_token,
			dev_file, global, mb_pack->token, sizeof(mb_pack->token));
		if (EOK != ret) {
			tloge("fill info failed. \
				global=%d, cmd_id=%d, session_id=%d\n",
				global, smc_cmd->cmd_id, smc_cmd->context_id);
			return -EFAULT;
		}
	}

	is_open_session_cmd = global
	                   && (smc_cmd->cmd_id == GLOBAL_CMD_ID_OPEN_SESSION);
	if (is_open_session_cmd) {
		if (generate_encrypted_session_secure_params(mb_pack->secure_params,
				sizeof(mb_pack->secure_params))) {
			tloge("Can't get encrypted session parameters buffer!");
			return -EFAULT;
		}

		smc_cmd->params_phys =
			virt_to_phys((void *)mb_pack->secure_params);
		smc_cmd->params_h_phys =
			virt_to_phys((void *)mb_pack->secure_params) >> 32; /*lint !e572*/
	}
	return EOK;
}

static int append_teec_token(TC_NS_ClientContext *client_context,
			TC_NS_Token *tc_ns_token,
			TC_NS_DEV_File *dev_file,
			bool global,
			uint8_t *mb_pack_token, uint32_t mb_pack_token_size) {
	uint8_t temp_libteec_token[TOKEN_SAVE_LEN] = {0};
	int sret;
	if ((!client_context) || (!dev_file) || (!tc_ns_token)) {
		tloge("in parameter is invalid.\n");
		return -EFAULT;
	}

	if ((!client_context->teec_token) || (!tc_ns_token->token_buffer)) {
		tloge("teec_token or token_buffer is NULL, error!\n");
		return -EFAULT;
	}

	if (!global) {
		if (copy_from_client(temp_libteec_token,
					client_context->teec_token,
					TOKEN_SAVE_LEN,
					dev_file->kernel_api)) {
			tloge("copy libteec token failed!\n");
			return -EFAULT;
		}

		/* combine token_buffer */
		/* teec_token, 0-15byte */
		if (EOK != memmove_s(tc_ns_token->token_buffer,
					TOKEN_BUFFER_LEN,
					temp_libteec_token,
					TIMESTAMP_SAVE_INDEX)) {
			tloge("copy temp_libteec_token failed!\n");
			sret = memset_s(tc_ns_token->token_buffer, TOKEN_BUFFER_LEN,
					0, TOKEN_BUFFER_LEN);
			if (sret != 0 ) {
				tloge("memset_s failed!\n");
				return -EFAULT;
			}
			return -EFAULT;
		}

		/* TODO:add 1 memcpy */
		if (memcpy_s(mb_pack_token, mb_pack_token_size,
				tc_ns_token->token_buffer, TOKEN_BUFFER_LEN)) {
			tloge("copy token failed\n");
			return -EFAULT;
		}
	}

	return EOK;
}

static int post_process_token(TC_NS_SMC_CMD *smc_cmd,
			TC_NS_ClientContext *client_context, TC_NS_Token *tc_ns_token,
			uint8_t *mb_pack_token, uint32_t mb_pack_token_size,
			uint8_t kernel_api, bool global)
{
	int ret = 0;

	if ((!smc_cmd) || (!client_context)
		|| (!tc_ns_token) || (!mb_pack_token) || !tc_ns_token->token_buffer
		|| (mb_pack_token_size < TOKEN_BUFFER_LEN)) {
		tloge("in parameter is invalid.\n");
		return -EINVAL;
	}

	/* TODO:add 1 memcpy */
	if (memcpy_s(tc_ns_token->token_buffer, TOKEN_BUFFER_LEN,
			mb_pack_token, mb_pack_token_size)) {
		tloge("copy token failed\n");
		return -EFAULT;
	}
	if (memset_s(mb_pack_token, mb_pack_token_size,
			0, mb_pack_token_size)) {
		tloge("memset mb_pack token error.\n");
		return -EFAULT;
	}

	sync_timestamp(smc_cmd, tc_ns_token->token_buffer,
			client_context->uuid, global);

	ret = save_token_info(client_context->teec_token,
			tc_ns_token->token_buffer,
			kernel_api);
	if (EOK != ret) {
		tloge("save_token_info  failed.\n");
		return -EFAULT;
	}

	return EOK;
}

#define TEE_TZMP \
{ \
    0xf8028dca,\
    0xaba0,\
    0x11e6,\
    { \
       0x80, 0xf5, 0x76, 0x30, 0x4d, 0xec, 0x7e, 0xb7 \
    } \
}
#define INVALID_TZMP_UID   0xffffffff
static DEFINE_MUTEX(tzmp_lock);
static unsigned int tzmp_uid = INVALID_TZMP_UID;

int TZMP2_uid(TC_NS_ClientContext *client_context, TC_NS_SMC_CMD *smc_cmd, bool global){
	TEEC_UUID uuid_tzmp = TEE_TZMP;

	if (!client_context || !smc_cmd) {
		tloge("client_context or smc_cmd is null! ");
		return -EINVAL;
	}

	if (0 == memcmp(client_context->uuid, (unsigned char *)&uuid_tzmp, sizeof(TEEC_UUID))) {
		if (smc_cmd->cmd_id == GLOBAL_CMD_ID_OPEN_SESSION && global) {
			/* save tzmp_uid */
			mutex_lock(&tzmp_lock);
			tzmp_uid = 0; /*for multisesion, we use same uid*/
			smc_cmd->uid = 0;
			tlogv("openSession , tzmp_uid.uid is %d", tzmp_uid);
			mutex_unlock(&tzmp_lock);

			return EOK;
		}

		mutex_lock(&tzmp_lock);
		if (tzmp_uid == INVALID_TZMP_UID) {
			tloge("tzmp_uid.uid error!");
			mutex_unlock(&tzmp_lock);
			return -EFAULT;
		}
		smc_cmd->uid = tzmp_uid;
		tlogv("invokeCommand or closeSession , tzmp_uid is %d, global is %d", tzmp_uid, global);
		mutex_unlock(&tzmp_lock);

		return EOK;
	}
	return -EFAULT;
}

int tc_client_call(TC_NS_ClientContext *client_context,
			TC_NS_DEV_File *dev_file,
			uint8_t flags,
			TC_NS_Token *tc_ns_token)
{
	int ret = 0;
	unsigned int tee_ret = 0;
	int s_ret = 0;
	TC_NS_SMC_CMD *smc_cmd = NULL;
	TC_NS_Session *session = NULL;
	TC_NS_Service *service = NULL;
	struct TC_wait_data *wq = NULL;
	TC_NS_Temp_Buf local_temp_buffer[4] = {{0, 0}, {0, 0}, {0, 0}, {0, 0} };
	bool global = flags & TC_CALL_GLOBAL;
	uint32_t uid;
	int needchecklogin = 0;
	int needreset = 0;
	bool is_token_work = false;
	struct mb_cmd_pack *mb_pack;
	bool operation_init = false;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
	kuid_t kuid;

	kuid = current_uid(); /*lint !e666 !e64 */
	uid = kuid.val;
#else
	uid = current_uid();
#endif
	if (!dev_file) {
		tloge("dev_file is null");
		return -EINVAL;
	}

	if (!client_context) {
		tloge("client_context is null");
		return -EINVAL;
	}

	if (client_context->cmd_id == GLOBAL_CMD_ID_OPEN_SESSION)
		CFC_FUNC_ENTRY(tc_client_call);

	smc_cmd = kzalloc(sizeof(TC_NS_SMC_CMD), GFP_KERNEL);
	if (!smc_cmd) {
		tloge("smc_cmd malloc failed.\n");
		return -ENOMEM;
	}

	mb_pack = mailbox_alloc_cmd_pack();
	if (!mb_pack) {
		kfree(smc_cmd);
		return -ENOMEM;
	}

	tlogd("Calling command %08x\n", client_context->cmd_id);

	if (client_context->paramTypes != 0) {
		ret = alloc_operation(dev_file, &mb_pack->operation, client_context,
					    local_temp_buffer, flags);
		if (ret) {
			tloge("alloc_operation malloc failed");
			goto error;
		}
		operation_init = true;
	}

	mb_pack->uuid[0] = ((true == global) ? 1 : 0);
	s_ret = memcpy_s(mb_pack->uuid + 1, 16, client_context->uuid, 16);
	if (EOK != s_ret) {
		ret = -EFAULT;
		tloge("alloc_operation _s error.\n");
		goto error;
	}
	smc_cmd->uuid_phys = virt_to_phys((void *)mb_pack->uuid);
	smc_cmd->uuid_h_phys = virt_to_phys((void *)mb_pack->uuid) >> 32; /*lint !e572*/
	smc_cmd->cmd_id = client_context->cmd_id;
	smc_cmd->dev_file_id = dev_file->dev_file_id;
	smc_cmd->context_id = client_context->session_id;
	smc_cmd->err_origin = client_context->returns.origin;
	smc_cmd->started = client_context->started;
	smc_cmd->uid = uid;
	TZMP2_uid(client_context, smc_cmd, global);
	tlogv("current uid is %d\n", smc_cmd->uid);
	if (client_context->paramTypes != 0) {
		smc_cmd->operation_phys = virt_to_phys(&mb_pack->operation);
		smc_cmd->operation_h_phys = virt_to_phys(&mb_pack->operation) >> 32; /*lint !e572*/
	} else {
		smc_cmd->operation_phys = 0;
		smc_cmd->operation_h_phys = 0;
	}

	smc_cmd->login_method = client_context->login.method;
	needchecklogin = sizeof(uint32_t) == dev_file->pub_key_len &&
		GLOBAL_CMD_ID_OPEN_SESSION == smc_cmd->cmd_id &&
		(current->mm != NULL) && global;
	if (needchecklogin) {
		ret = do_encryption(g_ca_auth_hash_buf,
		                    sizeof(g_ca_auth_hash_buf),
		                    MAX_SHA_256_SZ);
		if (ret) {
			tloge("hash encryption failed ret=%d\n", ret);
			goto error;
		}

		if (memcpy_s(mb_pack->login_data, sizeof(mb_pack->login_data),
				g_ca_auth_hash_buf, sizeof(g_ca_auth_hash_buf))) {
			tloge("copy login data failed\n");
			goto error;
		}
		smc_cmd->login_data_phy = virt_to_phys(mb_pack->login_data);
		smc_cmd->login_data_h_addr = virt_to_phys(mb_pack->login_data) >> 32; /*lint !e572*/
		smc_cmd->login_data_len = MAX_SHA_256_SZ;
	} else {
		smc_cmd->login_data_phy = 0;
		smc_cmd->login_data_h_addr = 0;
		smc_cmd->login_data_len = 0;
	}

	smc_cmd->ca_pid = current->pid;
	smc_cmd->real_pid = current->pid;
	is_token_work = (!global)
		|| (smc_cmd->cmd_id == GLOBAL_CMD_ID_OPEN_SESSION);

	ret = load_security_enhance_info(smc_cmd, client_context,
				tc_ns_token, dev_file, mb_pack,
				global, is_token_work);
	if (EOK != ret) {
		tloge("load_security_enhance_info  failed.\n");
		goto error;
	}
	/* send smc to secure world */
	tee_ret = TC_NS_SMC(smc_cmd, flags);

	client_context->session_id = smc_cmd->context_id;
       // if tee_ret error except TEEC_PENDING,but context_id is seted,need to reset to 0.
	needreset = global && client_context->cmd_id == GLOBAL_CMD_ID_OPEN_SESSION && 
		tee_ret !=0 && TEEC_PENDING != tee_ret;/*lint !e650 */
	if (needreset) {
		client_context->session_id = 0;/*lint !e63 */
	}

	if (is_token_work) {
		ret = post_process_token(smc_cmd, client_context, tc_ns_token,
				mb_pack->token, sizeof(mb_pack->token),
				dev_file->kernel_api, global);
		if (EOK != ret) {
			tloge("post_process_token  failed.\n");
			smc_cmd->err_origin = TEEC_ORIGIN_COMMS;
			goto error;
		}
	}
	if (tee_ret != 0) {
		while (TEEC_PENDING == tee_ret) { /*lint !e650 */
			mutex_lock(&dev_file->service_lock);
			service = tc_find_service(&dev_file->services_list,
					client_context->uuid); /*lint !e64 */
			get_service_struct(service);
			if (service) {
				mutex_lock(&service->session_lock);
				session = tc_find_session
					(&service->session_list,
					 client_context->session_id);
				get_session_struct(session);
				mutex_unlock(&service->session_lock);
				if (session)
					wq = &session->wait_data;
			}
			put_service_struct(service);
			mutex_unlock(&dev_file->service_lock);

			if (wq) {
				tlogv("before wait event\n");
				/* use wait_event instead of wait_event_interruptible so
				 * that ap suspend will not wake up the TEE wait call */
				wait_event(wq->send_cmd_wq, wq->send_wait_flag);
				wq->send_wait_flag = 0;
				put_session_struct(session);
			}

			tlogv("operation start is :%d\n",
					smc_cmd->started);
			if (is_token_work) {
				ret = append_teec_token(client_context,
						tc_ns_token, dev_file, global,
						mb_pack->token, sizeof(mb_pack->token));
				if (EOK != ret) {
					tloge("append teec's member failed. \
						global=%d, cmd_id=%d, session_id=%d\n",
						global, smc_cmd->cmd_id, smc_cmd->context_id);
					goto error;
				}
			}
			tee_ret = TC_NS_SMC_WITH_NO_NR(smc_cmd, flags);
			if (is_token_work) {
				ret = post_process_token(smc_cmd, client_context, tc_ns_token,
						mb_pack->token, sizeof(mb_pack->token),
						dev_file->kernel_api, global);
				if (EOK != ret) {
					tloge("NO NR, post_process_token  failed.\n");
					goto error;
				}
			}
		}
		/* Client was interrupted, return and let it handle it's own
		 * signals first then retry */
		if (TEEC_CLIENT_INTR == tee_ret) { /*lint !e650 */
			ret = -ERESTARTSYS;
			goto error;
		} else if (tee_ret) {
			tloge("smc_call returns error ret 0x%x\n", tee_ret);
			tloge("smc_call smc cmd ret 0x%x\n", smc_cmd->ret_val);
			goto error1;
		}

		client_context->session_id = smc_cmd->context_id;
	}

	/* wake_up tee log reader */
	tz_log_write();

	if (operation_init) {
		ret = update_client_operation(dev_file, client_context,
					      &mb_pack->operation, local_temp_buffer,
					      true);
		if (ret) {
			smc_cmd->err_origin = TEEC_ORIGIN_COMMS;
			goto error;
		}
	}

	ret = 0;
	goto error;

error1:
	if (TEEC_ERROR_SHORT_BUFFER == tee_ret) { /*lint !e650 */
		/* update size */
		if (operation_init) {
			ret = update_client_operation(dev_file, client_context,
						      &mb_pack->operation,
						      local_temp_buffer,
						      false);
			if (ret) {
				smc_cmd->err_origin = TEEC_ORIGIN_COMMS;
				goto error;
			}
		}
	}

	ret = EFAULT;
error:
	/* kfree(NULL) is safe and this check is probably not required*/
	client_context->returns.code = tee_ret;
	client_context->returns.origin = smc_cmd->err_origin;
	if (TEE_ERROR_TAGET_DEAD == tee_ret) {
		uint64_t phys_addr = smc_cmd->uuid_phys | (uint64_t)(((uint64_t)smc_cmd->uuid_h_phys)<<32);
		unsigned char *uuidchar = (void *)phys_to_virt(phys_addr);
		TEEC_UUID *uuid = (TEEC_UUID*)(uuidchar + 1);
		tloge("ta_crash uuid\n");
		kill_ion_by_uuid((TEEC_UUID*)uuid);
	}
	kfree(smc_cmd);
	if (operation_init)
		free_operation(client_context, &mb_pack->operation, local_temp_buffer);
	mailbox_free(mb_pack);

	return ret;
}

static bool is_opensession_by_index(uint8_t flags,  uint32_t cmd_id, int i)
{
	/* params[2] for apk cert or native ca uid;
	 * params[3] for pkg name; therefore we set i>= 2
	 */
	bool global = flags & TC_CALL_GLOBAL;
	bool login_en = (global
		&& (i >= 2)
		&& (GLOBAL_CMD_ID_OPEN_SESSION == cmd_id));

	return login_en;
}

static bool is_valid_size(uint32_t buffer_size, uint32_t temp_size)
{
	bool overflow = false;

	if (AES_LOGIN_MAXLEN  < buffer_size) {
		tloge("SECURITY_AUTH_ENHANCE: buffer_size is not right\n");
		return false;
	}

	overflow = (temp_size > ROUND_UP(buffer_size, SZ_4K)) ? true : false;
	if (overflow) {
		tloge("SECURITY_AUTH_ENHANCE: input data exceeds limit\n");
		return false;
	}
	return true;
}

static int do_encryption(uint8_t *buffer, uint32_t buffer_size,
                         uint32_t payload_size)
{
	int ret = 0;
	uint32_t plaintext_size;
	uint32_t plaintext_aligned_size;
	uint32_t total_size;
	uint8_t *cryptotext = NULL;
	uint8_t *plaintext = buffer;
	struct encryption_head head;

	if (!buffer) {
		tloge("Do encryption buffer is null!\n");
		return -EINVAL;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0))
#endif

	/* Payload + Head + Padding */
	plaintext_size = payload_size + sizeof(struct encryption_head);
	plaintext_aligned_size = ROUND_UP(plaintext_size,
	                                  CIPHER_BLOCK_BYTESIZE);

	/* Need 16 bytes to store AES-CBC iv */
	total_size = plaintext_aligned_size + IV_BYTESIZE;

	if (total_size > buffer_size) {
		tloge("Do encryption buffer is not enough!\n");
		ret = -ENOMEM;
		goto end;
	}

	cryptotext = kzalloc(total_size, GFP_KERNEL);
	if (!cryptotext) {
		tloge("Malloc failed when doing encryption!\n");
		ret = -ENOMEM;
		goto end;
	}

	/* Setting encryption head */
	ret = set_encryption_head(&head, plaintext, payload_size);
	if (ret) {
		tloge("Set encryption head failed, ret = %d.\n", ret);
		goto end;
	}

	ret = memcpy_s((void *)(plaintext + payload_size),
	               buffer_size - payload_size,
	               (void *)&head,
	               sizeof(struct encryption_head));
	if (ret) {
		tloge("Copy encryption head failed, ret = %d.\n", ret);
		goto end;
	}

	/* Setting padding data */
	ret = crypto_aescbc_cms_padding(plaintext, plaintext_aligned_size,
	                                plaintext_size);
	if (ret) {
		tloge("Set encryption padding data failed, ret = %d.\n", ret);
		goto end;
	}

	ret = crypto_session_aescbc_key256(plaintext,
	                        plaintext_aligned_size,
	                        cryptotext,
	                        total_size,
	                        g_cur_session_secure_info.crypto_info.key,
	                        NULL,
	                        ENCRYPT);
	if (ret) {
		tloge("Encrypt failed, ret=%d.\n", ret);
		goto end;
	}

	ret = memcpy_s((void *)buffer, buffer_size,
	               (void *)cryptotext, total_size);
	if (ret)
		tloge("Copy cryptotext failed, ret=%d.\n", ret);

end:
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0))
#endif
	if (cryptotext)
		kfree(cryptotext);
	return ret;
}

static int encrypt_login_info(uint8_t flags, uint32_t cmd_id, int32_t index,
                              uint32_t login_info_size, uint8_t *buffer)
{
	uint32_t payload_size;
	uint32_t plaintext_size;
	uint32_t plaintext_aligned_size;
	uint32_t total_size;
	bool login_en;

	if (!buffer) {
		tloge("Login information buffer is null!\n");
		return -EINVAL;
	}

	login_en = is_opensession_by_index(flags, cmd_id, index);
	if (!login_en)
		return 0;

	/* Need adding the termination null byte ('\0') to the end. */
	payload_size = login_info_size + sizeof(char);

	/* Payload + Head + Padding */
	plaintext_size = payload_size + sizeof(struct encryption_head);
	plaintext_aligned_size = ROUND_UP(plaintext_size,
	                                  CIPHER_BLOCK_BYTESIZE);

	/* Need 16 bytes to store AES-CBC iv */
	total_size = plaintext_aligned_size + IV_BYTESIZE;

	if (!is_valid_size(login_info_size, total_size)) {
		tloge("Login information encryption size is invalid!\n");
		return -EFAULT;
	}

	return do_encryption(buffer, total_size, payload_size);
}

