#ifndef __ZRHUNG_COMMON_H_
#define __ZRHUNG_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#define HTRANS_ERROR(format, ...)  do {printk(KERN_ERR "[*%s %d] " format, __func__, __LINE__, ##__VA_ARGS__);} while (0)
#define HTRANS_INFO(format, ...)  do {printk(KERN_DEBUG "[%s %d] " format, __func__, __LINE__, ##__VA_ARGS__);} while (0)

#define MAGIC_NUM  0x9517

#define __HTRANSIO  0xAB
#define LOGGER_WRITE_HEVENT		_IO(__HTRANSIO, 1)
#define LOGGER_READ_HEVENT		_IO(__HTRANSIO, 2)
#define LOGGER_SET_HCFG			_IO(__HTRANSIO, 3)
#define LOGGER_GET_HCFG			_IO(__HTRANSIO, 4)
#define LOGGER_GET_HLASTWORD		_IO(__HTRANSIO, 5)
#define LOGGER_SET_HCFG_FLAG		_IO(__HTRANSIO, 6)
#define LOGGER_GET_HCFG_FLAG		_IO(__HTRANSIO, 7)
#define LOGGER_SET_FEATURE              _IO(__HTRANSIO, 8)

#define LOGGER_CMD_MAX			_IO(__HTRANSIO, 9)

#ifdef __cplusplus
}
#endif
#endif /* __ZRHUNG_COMMON_H_ */
