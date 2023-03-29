

#ifndef __OAL_SDIO_HOST_IF_H__
#define __OAL_SDIO_HOST_IF_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#include "oal_util.h"
#include "oal_net.h"
#include "oal_sdio_comm.h"
#include "oal_bus_if.h"
#include "oal_hcc_bus.h"


#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#ifdef CONFIG_MMC
#include <linux/mmc/host.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/sdio.h>
#endif
#endif

enum {
    SDIO_READ = 0,
    SDIO_WRITE,
    SDIO_OPT_BUTT
};

typedef oal_int32 (*hisdio_rx)(oal_void* data);

    /*0x30~0x38, 0x3c~7B*/
#define HISDIO_EXTEND_BASE_ADDR (0x30)
#define HISDIO_EXTEND_REG_COUNT (64)

#define HISDIO_ALIGN_4_OR_BLK(len)     ((len) < HISDIO_BLOCK_SIZE ? ALIGN((len), 4) : ALIGN((len), HISDIO_BLOCK_SIZE))

#define HISDIO_WAKEUP_DEV_REG  (0xf0)
#define ALLOW_TO_SLEEP_VALUE  (1)
#define DISALLOW_TO_SLEEP_VALUE  (0)

#if (_PRE_OS_VERSION_LINUX != _PRE_OS_VERSION)
struct sdio_func {
    unsigned		enable_timeout;	/* max enable timeout in msec */
};
#endif



struct hisdio_extend_func
{
    oal_uint32 int_stat;
    oal_uint32 msg_stat;
    oal_uint32 xfer_count;
    oal_uint32 credit_info;
    oal_uint8  comm_reg[HISDIO_EXTEND_REG_COUNT];
};

struct sdio_scatt
{
    oal_uint32 max_scatt_num;
    struct scatterlist*  sglist;
};

struct hsdio_func1_info
{
    oal_uint32 func1_err_reg_info;
    oal_uint32 func1_err_int_count;
    oal_uint32 func1_ack_int_acount;
    oal_uint32 func1_msg_int_count;
    oal_uint32 func1_data_int_count;
    oal_uint32 func1_no_int_count;
};

struct hsdio_error_info
{
    oal_uint32 rx_scatt_info_not_match;
};

typedef struct _hsdio_scatt_buff_
{
    /*record the tx scatt list assembled buffer*/
    oal_void* buff;
    oal_uint32 len;
}hsdio_scatt_buff;

struct hsdio_credit_info{
    oal_uint8   short_free_cnt;
    oal_uint8   large_free_cnt;
    oal_spin_lock_stru credit_lock;
};



struct oal_sdio
{
    /*sdio work state, sleep , work or shutdown?*/
    hcc_bus*                    pst_bus;

    oal_uint32                  state;	            /*总线当前状态*/
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    struct sdio_func           *func;
#endif

    hisdio_rx                   credit_update_cb;

    /*used to sg list sdio block align*/
    oal_uint8                  *sdio_align_buff;

    oal_uint64                  sdio_int_count;

    oal_uint32                  ul_sdio_suspend;
    oal_uint32                  ul_sdio_resume;


    struct sdio_scatt      scatt_info[SDIO_OPT_BUTT];

    /*This is get from sdio , must alloc for dma,
      the extend area only can access by CMD53*/
    struct hisdio_extend_func   *sdio_extend;
    struct hsdio_credit_info     sdio_credit_info;
    oal_uint32               func1_int_mask;
    struct hsdio_func1_info  func1_stat;
    struct hsdio_error_info  error_stat;

    hsdio_scatt_buff tx_scatt_buff;
    hsdio_scatt_buff rx_scatt_buff;
    oal_void*       rx_reserved_buff;/*use the mem when rx alloc mem failed!*/
    oal_uint32      rx_reserved_buff_len;

    oal_wakelock_stru           st_sdio_rx_wakelock;

    oal_completion              st_sdio_shutdown_response;
};

OAL_STATIC OAL_INLINE oal_void oal_sdio_claim_host(struct oal_sdio *hi_sdio)
{
#ifdef CONFIG_MMC
    if(OAL_WARN_ON(NULL == hi_sdio))
    {
        return;
    }

    if(OAL_WARN_ON(NULL == hi_sdio->func))
    {
        return;
    }
    sdio_claim_host(hi_sdio->func);
#endif
}

OAL_STATIC OAL_INLINE oal_void oal_sdio_release_host(struct oal_sdio *hi_sdio)
{
#ifdef CONFIG_MMC
    if(OAL_WARN_ON(NULL == hi_sdio))
    {
        return;
    }

    if(OAL_WARN_ON(NULL == hi_sdio->func))
    {
        return;
    }
    sdio_release_host(hi_sdio->func);
#endif
}



oal_uint32 oal_sdio_get_large_pkt_free_cnt_etc(struct oal_sdio *hi_sdio);

oal_void oal_netbuf_list_hex_dump_etc(oal_netbuf_head_stru* head);
oal_void oal_netbuf_hex_dump_etc(oal_netbuf_stru* netbuf);
extern oal_int32 oal_wifi_platform_load_sdio(oal_void);
extern oal_void  oal_wifi_platform_unload_sdio(oal_void);

#ifdef CONFIG_MMC
oal_void oal_sdio_info_show(struct oal_sdio *hi_sdio);
oal_void oal_netbuf_list_hex_dump_etc(oal_netbuf_head_stru* head);
oal_void oal_netbuf_hex_dump_etc(oal_netbuf_stru* netbuf);
oal_int32 oal_sdio_build_rx_netbuf_list_etc(struct oal_sdio *hi_sdio,
                                             oal_netbuf_head_stru   * head);
oal_int32 oal_sdio_func_probe(struct oal_sdio* hi_sdio);
oal_void oal_sdio_func_remove_etc(struct oal_sdio* hi_sdio);

oal_void  oal_sdio_credit_update_cb_register_etc(struct oal_sdio *hi_sdio,hisdio_rx cb);
oal_int32 oal_sdio_extend_buf_get(struct oal_sdio *hi_sdio);
extern oal_int32 oal_sdio_transfer_tx_etc(struct oal_sdio *hi_sdio, oal_netbuf_stru* netbuf);

oal_int32 oal_sdio_transfer_netbuf_list_etc(struct oal_sdio *hi_sdio,
                                      oal_netbuf_head_stru * head,
                                      oal_int32 rw);

extern struct oal_sdio*  oal_sdio_init_module_etc(oal_void);
extern oal_void  oal_sdio_exit_module_etc(struct oal_sdio* hi_sdio);

oal_int32 oal_sdio_send_msg_etc(hcc_bus* pst_bus, oal_uint32 val);

//extern struct oal_sdio* oal_get_sdio_default_handler(oal_void);
extern oal_void oal_sdio_get_dev_pm_state_etc(struct oal_sdio *hi_sdio,oal_uint* pst_ul_f1,oal_uint* pst_ul_f2,oal_uint* pst_ul_f3,oal_uint* pst_ul_f4);
extern oal_uint32 oal_sdio_func_max_req_size_etc(struct oal_sdio *pst_hi_sdio);
extern oal_int32 oal_sdio_110x_working_check(oal_void);

extern     struct oal_sdio  *_hi_sdio_;
OAL_STATIC OAL_INLINE struct oal_sdio* oal_get_sdio_default_handler(oal_void)
{
    return _hi_sdio_;
}

#else
extern struct oal_sdio hi_sdio_ut;
OAL_STATIC OAL_INLINE struct oal_sdio* oal_get_sdio_default_handler(oal_void)
{
    return &hi_sdio_ut;
}

OAL_STATIC OAL_INLINE oal_uint32 oal_sdio_func_max_req_size_etc(struct oal_sdio *pst_hi_sdio)
{
    return 0;
}

OAL_STATIC OAL_INLINE oal_void oal_sdio_info_show(struct oal_sdio *hi_sdio)
{
    return;
}

OAL_STATIC OAL_INLINE oal_int32 oal_sdio_send_msg_etc(oal_void* hi_sdio, oal_uint32 val)
{
    return OAL_SUCC;
}

OAL_STATIC OAL_INLINE oal_int32 oal_sdio_build_rx_netbuf_list_etc(struct oal_sdio *hi_sdio,
                                             oal_netbuf_head_stru   * head)
{
    return OAL_SUCC;
}

OAL_STATIC OAL_INLINE oal_void  oal_sdio_credit_update_cb_register_etc(struct oal_sdio *hi_sdio, hisdio_rx cb)
{
    return;
}


OAL_STATIC OAL_INLINE oal_int32 oal_sdio_transfer_tx_etc(struct oal_sdio *hi_sdio, oal_netbuf_stru* netbuf)
{
    return -OAL_FAIL;
}

OAL_STATIC OAL_INLINE oal_int32 oal_sdio_transfer_netbuf_list_etc(struct oal_sdio *hi_sdio,
                                      oal_netbuf_head_stru * head,
                                      oal_int32 rw)
{
    return -OAL_FAIL;
}

OAL_STATIC OAL_INLINE struct oal_sdio*  oal_sdio_init_module_etc(oal_void)
{
    return NULL;
}

OAL_STATIC OAL_INLINE oal_void oal_sdio_exit_module_etc(struct oal_sdio* hi_sdio)
{
}


OAL_STATIC OAL_INLINE oal_int32 oal_sdio_func_probe(struct oal_sdio* hi_sdio)
{
		return -1;
}

OAL_STATIC OAL_INLINE oal_void oal_sdio_func_remove_etc(struct oal_sdio* hi_sdio)
{
}

OAL_STATIC OAL_INLINE oal_void oal_sdio_get_dev_pm_state_etc(struct oal_sdio *hi_sdio,
                                                                oal_uint* pst_ul_f1,
                                                                oal_uint* pst_ul_f2,
                                                                oal_uint* pst_ul_f3,
                                                                oal_uint* pst_ul_f4)
{
}

#endif

/*Kernel Functions*/
#ifdef CONFIG_MMC
#define CONFIG_HISI_SDIO_TIME_DEBUG
/**
 *	sdio_memcpy_fromio - read a chunk of memory from a SDIO function
 *	@func: SDIO function to access
 *	@dst: buffer to store the data
 *	@addr: address to begin reading from
 *	@count: number of bytes to read
 *
 *	Reads from the address space of a given SDIO function. Return
 *	value indicates if the transfer succeeded or not.
 */
OAL_STATIC OAL_INLINE oal_int32 oal_sdio_memcpy_fromio(struct sdio_func *func, oal_void *dst,
	oal_uint32 addr, oal_int32 count)
{
    oal_int32 ret;
#ifdef CONFIG_HISI_SDIO_TIME_DEBUG
    ktime_t time_start;
    time_start = ktime_get();
#endif
    ret = sdio_memcpy_fromio(func, dst, addr, count);
#ifdef CONFIG_HISI_SDIO_TIME_DEBUG
    if(OAL_UNLIKELY(ret))
    {
        /*If sdio transfer failed, dump the sdio info*/
        oal_uint64  trans_us;
        ktime_t time_stop = ktime_get();
        trans_us = (oal_uint64)ktime_to_us(ktime_sub(time_stop, time_start));
        printk(KERN_WARNING"[E]sdio_memcpy_fromio fail=%d, time cost:%llu us,[dst:%p,addr:%u,count:%d]\n",
                            ret, trans_us, dst, addr, count);
    }
#endif
    return ret;
}

/**
 *	oal_sdio_readsb - read from a FIFO on a SDIO function
 *	@func: SDIO function to access
 *	@dst: buffer to store the data
 *	@addr: address of (single byte) FIFO
 *	@count: number of bytes to read
 *
 *	Reads from the specified FIFO of a given SDIO function. Return
 *	value indicates if the transfer succeeded or not.
 */
OAL_STATIC OAL_INLINE  oal_int32 oal_sdio_readsb(struct sdio_func *func, oal_void *dst, oal_uint32 addr,
	oal_int32 count)
{
    oal_int32 ret;
#ifdef CONFIG_HISI_SDIO_TIME_DEBUG
    ktime_t time_start;
    time_start = ktime_get();
#endif
    ret = sdio_readsb(func, dst, addr, count);
#ifdef CONFIG_HISI_SDIO_TIME_DEBUG
    if(OAL_UNLIKELY(ret))
    {
        /*If sdio transfer failed, dump the sdio info*/
        oal_uint64  trans_us;
        ktime_t time_stop = ktime_get();
        trans_us = (oal_uint64)ktime_to_us(ktime_sub(time_stop, time_start));
        printk(KERN_WARNING"[E]sdio_readsb fail=%d, time cost:%llu us,[dst:%p,addr:%u,count:%d]\n",
                            ret, trans_us, dst, addr, count);
    }
#endif
    return ret;
}

/**
 *	oal_sdio_writesb - write to a FIFO of a SDIO function
 *	@func: SDIO function to access
 *	@addr: address of (single byte) FIFO
 *	@src: buffer that contains the data to write
 *	@count: number of bytes to write
 *
 *	Writes to the specified FIFO of a given SDIO function. Return
 *	value indicates if the transfer succeeded or not.
 */
OAL_STATIC OAL_INLINE  oal_int32 oal_sdio_writesb(struct sdio_func *func, oal_uint32 addr, oal_void *src,
	int count)
{
    oal_int32 ret;
#ifdef CONFIG_HISI_SDIO_TIME_DEBUG
    ktime_t time_start;
    time_start = ktime_get();
#endif
	ret = sdio_writesb(func, addr , src, count);
#ifdef CONFIG_HISI_SDIO_TIME_DEBUG
    if(OAL_UNLIKELY(ret))
    {
        /*If sdio transfer failed, dump the sdio info*/
        oal_uint64  trans_us;
        ktime_t time_stop = ktime_get();
        trans_us = (oal_uint64)ktime_to_us(ktime_sub(time_stop, time_start));
        printk(KERN_WARNING"[E]oal_sdio_writesb fail=%d, time cost:%llu us,[src:%p,addr:%u,count:%d]\n",
                            ret, trans_us, src, addr, count);
    }
#endif
    return ret;
}

/**
 *	oal_sdio_readb - read a single byte from a SDIO function
 *	@func: SDIO function to access
 *	@addr: address to read
 *	@err_ret: optional status value from transfer
 *
 *	Reads a single byte from the address space of a given SDIO
 *	function. If there is a problem reading the address, 0xff
 *	is returned and @err_ret will contain the error code.
 */
OAL_STATIC OAL_INLINE oal_uint8 oal_sdio_readb(struct sdio_func *func, oal_uint32 addr, oal_int32 *err_ret)
{
    return sdio_readb(func, addr , err_ret);
}

/**
 *	oal_sdio_writeb - write a single byte to a SDIO function
 *	@func: SDIO function to access
 *	@b: byte to write
 *	@addr: address to write to
 *	@err_ret: optional status value from transfer
 *
 *	Writes a single byte to the address space of a given SDIO
 *	function. @err_ret will contain the status of the actual
 *	transfer.
 */
OAL_STATIC OAL_INLINE void oal_sdio_writeb(struct sdio_func *func, oal_uint8 b, oal_uint32 addr, oal_int32 *err_ret)
{
	sdio_writeb(func, b, addr, err_ret);
}

/**
 *	oal_sdio_readl - read a 32 bit integer from a SDIO function
 *	@func: SDIO function to access
 *	@addr: address to read
 *	@err_ret: optional status value from transfer
 *
 *	Reads a 32 bit integer from the address space of a given SDIO
 *	function. If there is a problem reading the address,
 *	0xffffffff is returned and @err_ret will contain the error
 *	code.
 */
OAL_STATIC OAL_INLINE oal_uint32 oal_sdio_readl(struct sdio_func *func, oal_uint32 addr, oal_int32 *err_ret)
{
	return sdio_readl(func , addr, err_ret);
}

/**
 *	oal_sdio_writel - write a 32 bit integer to a SDIO function
 *	@func: SDIO function to access
 *	@b: integer to write
 *	@addr: address to write to
 *	@err_ret: optional status value from transfer
 *
 *	Writes a 32 bit integer to the address space of a given SDIO
 *	function. @err_ret will contain the status of the actual
 *	transfer.
 */
OAL_STATIC OAL_INLINE oal_void oal_sdio_writel(struct sdio_func *func, oal_uint32 b, oal_uint32 addr, oal_int32 *err_ret)
{
	sdio_writel(func, b, addr, err_ret);
}
#else
OAL_STATIC OAL_INLINE oal_int32 oal_sdio_memcpy_fromio(struct sdio_func *func, oal_void *dst,
	oal_uint32 addr, oal_int32 count)
{
    OAL_REFERENCE(func);
    OAL_REFERENCE(dst);
    OAL_REFERENCE(addr);
    OAL_REFERENCE(count);
    return -OAL_EBUSY;
}

OAL_STATIC OAL_INLINE oal_int32 oal_sdio_readsb(struct sdio_func *func, oal_void *dst, oal_uint32 addr,
	oal_int32 count)
{
    OAL_REFERENCE(func);
    OAL_REFERENCE(dst);
    OAL_REFERENCE(addr);
    OAL_REFERENCE(count);
    return -OAL_EBUSY;
}

OAL_STATIC OAL_INLINE oal_int32 oal_sdio_writesb(struct sdio_func *func, oal_uint32 addr, oal_void *src,
	int count)
{
    OAL_REFERENCE(func);
    OAL_REFERENCE(src);
    OAL_REFERENCE(addr);
    OAL_REFERENCE(count);
    return -OAL_EBUSY;
}

OAL_STATIC OAL_INLINE oal_uint8 oal_sdio_readb(struct sdio_func *func, oal_uint32 addr, oal_int32 *err_ret)
{
    OAL_REFERENCE(func);
    OAL_REFERENCE(addr);
    *err_ret = -OAL_EBUSY;;
    return 0;
}

OAL_STATIC OAL_INLINE void oal_sdio_writeb(struct sdio_func *func, oal_uint8 b, oal_uint32 addr, oal_int32 *err_ret)
{
    OAL_REFERENCE(func);
    OAL_REFERENCE(addr);
    OAL_REFERENCE(b);
    *err_ret = -OAL_EBUSY;
}

OAL_STATIC OAL_INLINE oal_uint32 oal_sdio_readl(struct sdio_func *func, oal_uint32 addr, oal_int32 *err_ret)
{
    OAL_REFERENCE(func);
    OAL_REFERENCE(addr);
    *err_ret = -OAL_EBUSY;;
    return 0;
}

OAL_STATIC OAL_INLINE oal_void oal_sdio_writel(struct sdio_func *func, oal_uint32 b, oal_uint32 addr, oal_int32 *err_ret)
{
    OAL_REFERENCE(func);
    OAL_REFERENCE(addr);
    OAL_REFERENCE(b);
    *err_ret = -OAL_EBUSY;
}
#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif

