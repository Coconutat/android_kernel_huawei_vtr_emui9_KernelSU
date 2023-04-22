#ifndef __ERECOVERY_COMMON_H_
#define __ERECOVERY_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#define ERECOVERY_ERROR(format, ...)  do {printk(KERN_ERR "[*%s %d] " format, __func__, __LINE__, ##__VA_ARGS__);} while (0)
#define ERECOVERY_INFO(format, ...)  do {printk(KERN_DEBUG "[%s %d] " format, __func__, __LINE__, ##__VA_ARGS__);} while (0)

#define ERECOVERY_MAGIC_NUM  0x9527

#define __ERECIO  0xAD
#define ERECOVERY_WRITE_EVENT        _IO(__ERECIO, 1)
#define ERECOVERY_READ_EVENT        _IO(__ERECIO, 2)


#ifdef __cplusplus
}
#endif
#endif
