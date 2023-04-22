

#ifndef __OAL_UTIL_H__
#define __OAL_UTIL_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "oal_types.h"
#include "oal_mm.h"
#include "arch/oal_util.h"

/*****************************************************************************
  2 宏定义
*****************************************************************************/

#define OAL_VA_START            va_start
#define OAL_VA_END              va_end

#define OAL_VA_LIST             va_list

/* 拼接为16 bit或者 32bit */
#define OAL_MAKE_WORD16(lsb, msb) ((((oal_uint16)(msb) << 8) & 0xFF00) | (lsb))
#define OAL_MAKE_WORD32(lsw, msw) ((((oal_uint32)(msw) << 16) & 0xFFFF0000) | (lsw))
#define OAL_JOIN_WORD32(lsb, ssb, asb, msb) (((oal_uint32)(msb) << 24) | ((oal_uint32)(asb) << 16) | ((oal_uint32)(ssb) << 8) | (lsb))
#define OAL_JOIN_WORD20(lsw, msw) ((((oal_uint32)(msw) << 10) & 0xFFC00) | (lsw & 0x3FF))

/* 计算为字节对齐后填充后的长度 */
#define PADDING(x, size)           (((x) + (size) - 1) & (~ ((size) - 1)))

/* increment with wrap-around */
#define OAL_INCR(_l, _sz)   (_l)++; (_l) &= ((_sz) - 1)
#define OAL_DECR(_l, _sz)   (_l)--; (_l) &= ((_sz) - 1)

/* 获取大小 */
#define OAL_SIZEOF                                  sizeof

/* 获取数组大小 */
#define OAL_ARRAY_SIZE(_ast_array)                  (sizeof(_ast_array) / sizeof((_ast_array)[0]))

/* 四字节对齐 */
#define OAL_GET_4BYTE_ALIGN_VALUE(_ul_size)         (((_ul_size) + 0x03) & (~0x03))

/* 获取当前线程信息 */
#define OAL_CURRENT_TASK     (current_thread_info()->task)

#define OAL_SWAP_BYTEORDER_16(_val) ((((_val) & 0x00FF) << 8) + (((_val) & 0xFF00) >> 8))

#if (_PRE_BIG_CPU_ENDIAN == _PRE_CPU_ENDIAN)          /* BIG_ENDIAN */
#define OAL_BYTEORDER_TO_LE32(_val)        OAL_SWAP_BYTEORDER_32(_val)
#define OAL_BYTEORDER_TO_LE16(_val)        OAL_SWAP_BYTEORDER_16(_val)
#define OAL_MASK_INVERSE(_len, _offset)    ((oal_uint32)(OAL_SWAP_BYTEORDER_32(~(((1 << (_len)) - 1) << (_offset)))))
#define OAL_MASK(_len, _offset)            ((oal_uint32)(OAL_SWAP_BYTEORDER_32(((1 << (_len)) - 1) << (_offset))))
#define OAL_NTOH_16(_val)                  (_val)
#define OAL_NTOH_32(_val)                  (_val)
#define OAL_HTON_16(_val)                  (_val)
#define OAL_HTON_32(_val)                  (_val)

#elif (_PRE_LITTLE_CPU_ENDIAN == _PRE_CPU_ENDIAN)     /* LITTLE_ENDIAN */
#define OAL_BYTEORDER_TO_LE32(_val)        (_val)
#define OAL_BYTEORDER_TO_LE16(_val)        (_val)
#define OAL_MASK_INVERSE(_len, _offset)    ((oal_uint32)(~(((1UL << (_len)) - 1) << (_offset))))
#define OAL_MASK(_len, _offset)            ((oal_uint32)(((1UL << (_len)) - 1) << (_offset)))
#define OAL_NTOH_16(_val)                  OAL_SWAP_BYTEORDER_16(_val)
#define OAL_NTOH_32(_val)                  OAL_SWAP_BYTEORDER_32(_val)
#define OAL_HTON_16(_val)                  OAL_SWAP_BYTEORDER_16(_val)
#define OAL_HTON_32(_val)                  OAL_SWAP_BYTEORDER_32(_val)
#endif

#if (!defined(_PRE_PC_LINT) && !defined(WIN32))
#ifdef __GNUC__
#define OAL_BUILD_BUG_ON(_con) ((oal_void)sizeof(char[1 - 2*!!(_con)]))
#else
#define OAL_BUILD_BUG_ON(_con)
#endif
#else
#define OAL_BUG_ON(_con)
#define OAL_BUILD_BUG_ON(_con)
#endif

#ifndef atomic_inc_return
#define oal_atomic_inc_return(a)    (0)
#else
#define oal_atomic_inc_return   atomic_inc_return
#endif

#if 0  /* 编不过 */
#ifndef current
#define current (0)
#endif
#endif

/* 比较宏 */
#define OAL_MIN(_A, _B) (((_A) < (_B))? (_A) : (_B))

/* 比较宏 */
#define OAL_MAX(_A, _B) (((_A) > (_B))? (_A) : (_B))

#define OAL_SUB(_A, _B) (((_A) > (_B))? ((_A) - (_B)) : (0))

#define OAL_ABSOLUTE_SUB(_A, _B) (((_A) > (_B))? ((_A) - (_B)) : ((_B) - (_A)))

/* 从某个设备读取某个寄存器地址的32-bit寄存器的值。*/
#define OAL_REG_READ32(_addr)    \
        *((OAL_VOLATILE oal_uint32 *)(_addr))

#define OAL_REG_READ16(_addr)    \
    *((OAL_VOLATILE oal_uint16 *)(_addr))

/* 往某个设备某个32-bit寄存器地址写入某个值 */
#define OAL_REG_WRITE32(_addr, _val)    \
    (*((OAL_VOLATILE oal_uint32 *)(_addr)) = (_val))
#define OAL_REG_WRITE16(_addr, _val)    \
    (*((OAL_VOLATILE oal_uint16 *)(_addr)) = (_val))

/* Is val aligned to "align" ("align" must be power of 2) */
#ifndef IS_ALIGNED
#define OAL_IS_ALIGNED(val, align)		\
	(((oal_uint32)(val) & ((align) - 1)) == 0)
#else
#define OAL_IS_ALIGNED  IS_ALIGNED
#endif


/* Bit Values */
#define BIT31                   ((oal_uint32)(1UL << 31))
#define BIT30                   ((oal_uint32)(1 << 30))
#define BIT29                   ((oal_uint32)(1 << 29))
#define BIT28                   ((oal_uint32)(1 << 28))
#define BIT27                   ((oal_uint32)(1 << 27))
#define BIT26                   ((oal_uint32)(1 << 26))
#define BIT25                   ((oal_uint32)(1 << 25))
#define BIT24                   ((oal_uint32)(1 << 24))
#define BIT23                   ((oal_uint32)(1 << 23))
#define BIT22                   ((oal_uint32)(1 << 22))
#define BIT21                   ((oal_uint32)(1 << 21))
#define BIT20                   ((oal_uint32)(1 << 20))
#define BIT19                   ((oal_uint32)(1 << 19))
#define BIT18                   ((oal_uint32)(1 << 18))
#define BIT17                   ((oal_uint32)(1 << 17))
#define BIT16                   ((oal_uint32)(1 << 16))
#define BIT15                   ((oal_uint32)(1 << 15))
#define BIT14                   ((oal_uint32)(1 << 14))
#define BIT13                   ((oal_uint32)(1 << 13))
#define BIT12                   ((oal_uint32)(1 << 12))
#define BIT11                   ((oal_uint32)(1 << 11))
#define BIT10                   ((oal_uint32)(1 << 10))
#define BIT9                    ((oal_uint32)(1 << 9))
#define BIT8                    ((oal_uint32)(1 << 8))
#define BIT7                    ((oal_uint32)(1 << 7))
#define BIT6                    ((oal_uint32)(1 << 6))
#define BIT5                    ((oal_uint32)(1 << 5))
#define BIT4                    ((oal_uint32)(1 << 4))
#define BIT3                    ((oal_uint32)(1 << 3))
#define BIT2                    ((oal_uint32)(1 << 2))
#define BIT1                    ((oal_uint32)(1 << 1))
#define BIT0                    ((oal_uint32)(1 << 0))
#define ALL                     0xFFFF

#define BIT(nr)                 (1UL << (nr))

#define OAL_BITS_PER_BYTE       8   /* 一个字节中包含的bit数目 */


/* 位操作 */
#define OAL_SET_BIT(_val)                           (1 << (_val))
#define OAL_LEFT_SHIFT(_data, _num)                 ((_data) << (_num))
#define OAL_RGHT_SHIFT(_data, _num)                 ((_data) >> (_num))
#define OAL_WRITE_BITS(_data, _val, _bits, _pos)    do{\
        (_data) &= ~((((oal_uint32)1 << (_bits)) - 1) << (_pos));\
        (_data) |= (((_val) & (((oal_uint32)1 << (_bits)) - 1)) << (_pos));\
    }while(0)
#define OAL_GET_BITS(_data, _bits, _pos)      (((_data) >> (_pos)) & (((oal_uint32)1 << (_bits)) - 1))

/* 位数定义 */
#define NUM_1_BITS                        1
#define NUM_2_BITS                        2
#define NUM_3_BITS                        3
#define NUM_4_BITS                        4
#define NUM_5_BITS                        5
#define NUM_6_BITS                        6
#define NUM_7_BITS                        7
#define NUM_8_BITS                        8
#define NUM_9_BITS                        9
#define NUM_10_BITS                       10
#define NUM_11_BITS                       11
#define NUM_12_BITS                       12
#define NUM_13_BITS                       13
#define NUM_14_BITS                       14
#define NUM_15_BITS                       15
#define NUM_16_BITS                       16
#define NUM_17_BITS                       17
#define NUM_18_BITS                       18
#define NUM_19_BITS                       19
#define NUM_20_BITS                       20
#define NUM_21_BITS                       21
#define NUM_22_BITS                       22
#define NUM_23_BITS                       23
#define NUM_24_BITS                       24
#define NUM_25_BITS                       25
#define NUM_26_BITS                       26
#define NUM_27_BITS                       27
#define NUM_28_BITS                       28
#define NUM_29_BITS                       29
#define NUM_30_BITS                       30
#define NUM_31_BITS                       31
#define NUM_32_BITS                       32

/* 位偏移定义 */
#define BIT_OFFSET_0                        0
#define BIT_OFFSET_1                        1
#define BIT_OFFSET_2                        2
#define BIT_OFFSET_3                        3
#define BIT_OFFSET_4                        4
#define BIT_OFFSET_5                        5
#define BIT_OFFSET_6                        6
#define BIT_OFFSET_7                        7
#define BIT_OFFSET_8                        8
#define BIT_OFFSET_9                        9
#define BIT_OFFSET_10                       10
#define BIT_OFFSET_11                       11
#define BIT_OFFSET_12                       12
#define BIT_OFFSET_13                       13
#define BIT_OFFSET_14                       14
#define BIT_OFFSET_15                       15
#define BIT_OFFSET_16                       16
#define BIT_OFFSET_17                       17
#define BIT_OFFSET_18                       18
#define BIT_OFFSET_19                       19
#define BIT_OFFSET_20                       20
#define BIT_OFFSET_21                       21
#define BIT_OFFSET_22                       22
#define BIT_OFFSET_23                       23
#define BIT_OFFSET_24                       24
#define BIT_OFFSET_25                       25
#define BIT_OFFSET_26                       26
#define BIT_OFFSET_27                       27
#define BIT_OFFSET_28                       28
#define BIT_OFFSET_29                       29
#define BIT_OFFSET_30                       30
#define BIT_OFFSET_31                       31

/*定点数四舍五入，并取整数部分, fract_bits为小数位数*/
#define _ROUND_POS(fix_num, fract_bits) (((fix_num) + (1 << ((fract_bits) - 1))) >> (fract_bits))
#define _ROUND_NEG(fix_num, fract_bits) -_ROUND_POS(-(fix_num), fract_bits)
#define OAL_ROUND(fix_num, fract_bits) ((fix_num) > 0 ? _ROUND_POS(fix_num, fract_bits) : _ROUND_NEG(fix_num, fract_bits))


#define OAL_RSSI_INIT_MARKER     0x320    /* RSSI平滑值初始值*/
#define OAL_RSSI_MAX_DELTA       24       /*最大步长 24/8 = 3*/
#define OAL_RSSI_FRACTION_BITS   3
#define OAL_RSSI_SIGNAL_MIN      -103     /*上报RSSI下边界*/
#define OAL_RSSI_SIGNAL_MAX      5        /*上报RSSI上边界*/
#define OAL_SNR_INIT_VALUE       0x7F     /* SNR上报的初始值 */
#define OAL_RSSI_INIT_VALUE      (-128)   /* RSSI的初始值 */

#define OAL_IPV6_ADDR_LEN 16

/*****************************************************************************
  3 枚举定义
*****************************************************************************/

/*****************************************************************************
  4 全局变量声明
*****************************************************************************/


/*****************************************************************************
  5 消息头定义
*****************************************************************************/


/*****************************************************************************
  6 消息定义
*****************************************************************************/


/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/
#define HI11XX_LOG_ERR  0
#define HI11XX_LOG_WARN 1
#define HI11XX_LOG_INFO 2
#define HI11XX_LOG_DBG  3
#define HI11XX_LOG_VERBOSE 4

#ifdef CONFIG_PRINTK
#include <linux/module.h>

#ifndef HI11XX_LOG_MODULE_NAME
#define HI11XX_LOG_MODULE_NAME "[HI11XX]"
extern oal_int32 hi11xx_loglevel;
#else
oal_int32 HI11XX_LOG_MODULE_NAME_VAR = HI11XX_LOG_INFO;
module_param(HI11XX_LOG_MODULE_NAME_VAR, int, S_IRUGO | S_IWUSR);
#endif

#ifndef HI11XX_LOG_MODULE_NAME_VAR
#define HI11XX_LOG_MODULE_NAME_VAR hi11xx_loglevel
extern oal_int32 hi11xx_loglevel;
#endif

#define HI11XX_LOG_MODULE_NAME_NUMS_STR(num) #num

extern char* g_hi11xx_loglevel_format[] ;

#define oal_print_hi11xx_log(loglevel, fmt, arg...)\
        do{\
            if(OAL_UNLIKELY(HI11XX_LOG_MODULE_NAME_VAR >= loglevel))\
            {\
                printk("%s%s" fmt"[%s:%d]\n", g_hi11xx_loglevel_format[loglevel],  HI11XX_LOG_MODULE_NAME, ##arg, __FUNCTION__, __LINE__);\
            }\
		}while(0)
#else
#define oal_print_hi11xx_log
#endif


#ifdef _PRE_CONFIG_HISI_PANIC_DUMP_SUPPORT
typedef  struct _hwifi_panic_log_   hwifi_panic_log;
typedef oal_int32 (* hwifi_panic_log_cb)(oal_void* data,oal_uint8*pst_buf,oal_int32 buf_len);
struct _hwifi_panic_log_
{
    struct list_head list;
    /*the log module name*/
    char* name;
    hwifi_panic_log_cb    cb;
    oal_void* data;
};
#define DECLARE_WIFI_PANIC_STRU(module_name,func) \
        hwifi_panic_log module_name = \
        {\
            .name = #module_name,\
            .cb = (hwifi_panic_log_cb)func,\
        }
#endif

/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/
#ifdef _PRE_CONFIG_HISI_PANIC_DUMP_SUPPORT

extern oal_void hwifi_panic_log_register_etc(hwifi_panic_log* log, void* data);
extern oal_void hwifi_panic_log_dump_etc(char* print_level);
#else
OAL_STATIC OAL_INLINE oal_void hwifi_panic_log_dump_etc(char* print_level)
{
}
#endif


OAL_STATIC OAL_INLINE oal_void oal_print_inject_check_stack(oal_void)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    char trinity_name[50];
    memset(trinity_name, 0, sizeof(trinity_name));
    memcpy(trinity_name, current->comm, OAL_STRLEN("trinity"));
    if(unlikely(!memcmp((void*)"trinity", (void*)trinity_name, OAL_STRLEN("trinity"))))
    {
        /*Debug*/
        WARN_ON(1);
    }
#endif
}


OAL_STATIC OAL_INLINE oal_uint8  oal_strtohex(const oal_int8 *c_string)
{
    oal_uint8 uc_ret = 0;

    if (*c_string >= '0' && *c_string <= '9')
    {
        uc_ret = (oal_uint8)(*c_string - '0');
    }
    else if (*c_string >= 'A' && *c_string <= 'F')
    {
        uc_ret = (oal_uint8)(*c_string - 'A' + 10);
    }
    else if (*c_string >= 'a' && *c_string <= 'f')
    {
        uc_ret = (oal_uint8)(*c_string - 'a' + 10);
    }

    return uc_ret;
}


OAL_STATIC OAL_INLINE oal_void  oal_strtoaddr(const oal_int8 *pc_param, oal_uint8 *puc_mac_addr)
{
    oal_uint8   uc_char_index;

    /* 获取mac地址,16进制转换 */
    for (uc_char_index = 0; uc_char_index < 12; uc_char_index++)
    {
        if ((':' == *pc_param) || ('-' == *pc_param))
        {
            pc_param++;
            if (0 != uc_char_index)
            {
                uc_char_index--;
            }

            continue;
        }

        puc_mac_addr[uc_char_index/2] =
        (oal_uint8)(puc_mac_addr[uc_char_index/2] * 16 * (uc_char_index % 2) +
                                        oal_strtohex(pc_param));
        pc_param++;
    }

}

/*****************************************************************************
 函 数 名  : oal_strtoipv6
 功能描述  : 字符串转ipv6地址
 输入参数  : pc_param: ipv6地址字符串, 格式 xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx
 输出参数  : puc_mac_addr: 转换成16进制后的ipv6地址
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :


*****************************************************************************/
OAL_STATIC OAL_INLINE oal_void  oal_strtoipv6(const oal_int8 *pc_param, oal_uint8 *puc_ipv6_addr)
{
    oal_uint8   uc_char_index;

    /* 获取ipv6地址,16进制转换 */
    for (uc_char_index = 0; uc_char_index < OAL_IPV6_ADDR_LEN*2; uc_char_index++)
    {
        if ((':' == *pc_param))
        {
            pc_param++;
            if (0 != uc_char_index)
            {
                uc_char_index--;
            }

            continue;
        }
        /* 将ipv6字符串转换为16进制数组 */
        puc_ipv6_addr[uc_char_index>>1] =
        (oal_uint8)(((puc_ipv6_addr[uc_char_index>>1])<<4) * (uc_char_index % 2) +
                                        oal_strtohex(pc_param));
        pc_param++;
    }

}



OAL_STATIC OAL_INLINE oal_int  oal_memcmp(OAL_CONST oal_void *p_buf1, OAL_CONST oal_void *p_buf2, oal_uint32 ul_count)
{
    return OAL_MEMCMP(p_buf1, p_buf2, ul_count);
}

OAL_STATIC OAL_INLINE oal_int  oal_strncmp(OAL_CONST oal_int8 *p_buf1, OAL_CONST oal_int8 *p_buf2, oal_uint32 ul_count)
{
    return OAL_STRNCMP(p_buf1, p_buf2, ul_count);
}

OAL_STATIC OAL_INLINE oal_int  oal_strncasecmp(OAL_CONST oal_int8 *p_buf1, OAL_CONST oal_int8 *p_buf2, oal_uint32 ul_count)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    return OAL_STRNCASECMP(p_buf1, p_buf2, ul_count);
#else
    return OAL_STRNCMP(p_buf1, p_buf2, ul_count); /* windows still use strncmp */
#endif
}


OAL_STATIC OAL_INLINE oal_uint8  oal_get_random(oal_void)
{
    /* TBD */
    return 1;
}


OAL_STATIC OAL_INLINE oal_uint8  oal_gen_random(oal_uint32 ul_val, oal_uint8 us_rst_flag)
{
    OAL_STATIC oal_uint32 ul_rand = 0;
    if (0 != us_rst_flag)
    {
        ul_rand = ul_val;
    }
    ul_rand = ul_rand * 1664525L + 1013904223L;
	return (oal_uint8) (ul_rand >> 24);
}


OAL_STATIC OAL_INLINE oal_uint8  oal_bit_get_num_one_byte(oal_uint8 uc_byte)
{

    uc_byte = (uc_byte & 0x55) + ((uc_byte >> 1) & 0x55);
    uc_byte = (uc_byte & 0x33) + ((uc_byte >> 2) & 0x33);
    uc_byte = (uc_byte & 0x0F) + ((uc_byte >> 4) & 0x0F);

    return uc_byte;
}


OAL_STATIC OAL_INLINE oal_uint32  oal_bit_get_num_four_byte(oal_uint32 ul_byte)
{
    ul_byte = (ul_byte & 0x55555555) + ((ul_byte >>  1) & 0x55555555);
    ul_byte = (ul_byte & 0x33333333) + ((ul_byte >>  2) & 0x33333333);
    ul_byte = (ul_byte & 0x0F0F0F0F) + ((ul_byte >>  4) & 0x0F0F0F0F);
    ul_byte = (ul_byte & 0x00FF00FF) + ((ul_byte >>  8) & 0x00FF00FF);
    ul_byte = (ul_byte & 0x0000FFFF) + ((ul_byte >> 16) & 0x0000FFFF);

    return ul_byte;
}


OAL_STATIC OAL_INLINE oal_void  oal_bit_set_bit_one_byte(oal_uint8 *puc_byte, oal_bitops nr)
{
    *puc_byte |= ((oal_uint8)(1 << nr));
}


OAL_STATIC OAL_INLINE oal_void  oal_bit_clear_bit_one_byte(oal_uint8 *puc_byte, oal_bitops nr)
{
    *puc_byte &=(~((oal_uint8)(1 << nr)));
}

OAL_STATIC OAL_INLINE oal_uint8 oal_bit_get_bit_one_byte(oal_uint8 uc_byte, oal_bitops nr)
{
    return ((uc_byte >> nr) & 0x1);
}


OAL_STATIC OAL_INLINE oal_void  oal_bit_set_bit_four_byte(oal_uint32 *pul_byte, oal_bitops nr)
{
    *pul_byte |= ((oal_uint32)(1 << nr));
}


OAL_STATIC OAL_INLINE oal_void  oal_bit_clear_bit_four_byte(oal_uint32 *pul_byte, oal_bitops nr)
{
    *pul_byte &= ~((oal_uint32)(1 << nr));
}


OAL_STATIC OAL_INLINE oal_void  oal_bit_set_bit_eight_byte(oal_uint64 *pull_byte, oal_bitops nr)
{
    *pull_byte |= ((oal_uint64)1 << nr);
}


OAL_STATIC OAL_INLINE oal_void  oal_bit_clear_bit_eight_byte(oal_uint64 *pull_byte, oal_bitops nr)
{
    *pull_byte &= ~((oal_uint64)1 << nr);
}


OAL_STATIC OAL_INLINE oal_uint8  oal_bit_find_first_bit_one_byte(oal_uint8 uc_byte)
{
    oal_uint8 uc_ret = 0;

    uc_byte = uc_byte & (-uc_byte);

    while (uc_byte != 1)
    {
        uc_ret++;
        uc_byte = (uc_byte >> 1);

        if (uc_ret > 7)
        {
            return uc_ret;
        }
    }

    return uc_ret;
}


OAL_STATIC OAL_INLINE oal_uint8  oal_bit_find_first_zero_one_byte(oal_uint8 uc_byte)
{
    oal_uint8 uc_ret = 0;

    uc_byte = ~uc_byte;
    uc_byte = uc_byte & (-uc_byte);

    while (uc_byte != 1)
    {
        uc_ret++;
        uc_byte = (uc_byte >> 1);

        if (uc_ret > 7)
        {
            return uc_ret;
        }
    }

    return uc_ret;
}


OAL_STATIC OAL_INLINE oal_uint8  oal_bit_find_first_bit_four_byte(oal_uint32 ul_byte)
{
    oal_uint8 uc_ret = 0;

    if (0 == ul_byte)
    {
        return uc_ret;
    }

    if (!(ul_byte & 0xffff))
    {
        ul_byte >>= 16;
        uc_ret += 16;
    }

    if (!(ul_byte & 0xff))
    {
        ul_byte >>= 8;
        uc_ret += 8;
    }

    if (!(ul_byte & 0xf))
    {
        ul_byte >>= 4;
        uc_ret += 4;
    }

    if (!(ul_byte & 3))
    {
        ul_byte >>= 2;
        uc_ret += 2;
    }

    if (!(ul_byte & 1))
    {
        uc_ret += 1;
    }

    return uc_ret;
}


OAL_STATIC OAL_INLINE oal_uint8  oal_bit_find_first_zero_four_byte(oal_uint32 ul_byte)
{
    oal_uint8 uc_ret = 0;

    ul_byte = ~ul_byte;

    if (!(ul_byte & 0xffff))
    {
        ul_byte >>= 16;
        uc_ret += 16;
    }

    if (!(ul_byte & 0xff))
    {
        ul_byte >>= 8;
        uc_ret += 8;
    }

    if (!(ul_byte & 0xf))
    {
        ul_byte >>= 4;
        uc_ret += 4;
    }

    if (!(ul_byte & 3))
    {
        ul_byte >>= 2;
        uc_ret += 2;
    }

    if (!(ul_byte & 1))
    {
        uc_ret += 1;
    }

    return uc_ret;
}


OAL_STATIC OAL_INLINE oal_void  oal_set_mac_addr(oal_uint8 *puc_mac_addr1, oal_uint8 *puc_mac_addr2)
{
    oal_memcopy(puc_mac_addr1, puc_mac_addr2, 6);
}


OAL_STATIC OAL_INLINE oal_void  oal_set_mac_addr_zero(oal_uint8 *puc_mac_addr)
{
    OAL_MEMZERO(puc_mac_addr, 6);
}


OAL_STATIC OAL_INLINE oal_uint32  oal_compare_mac_addr(oal_uint8 *puc_mac_addr1, oal_uint8 *puc_mac_addr2)

{
    #if 0
    return (puc_mac_addr1[0] ^ puc_mac_addr2[0]) | (puc_mac_addr1[1] ^ puc_mac_addr2[1])
    | (puc_mac_addr1[2] ^ puc_mac_addr2[2]) | (puc_mac_addr1[3] ^ puc_mac_addr2[3])
    | (puc_mac_addr1[4] ^ puc_mac_addr2[4]) | (puc_mac_addr1[5] ^ puc_mac_addr2[5]);
    #endif
    return (oal_uint32)oal_memcmp((void*)puc_mac_addr1, (void*)puc_mac_addr2, 6);
}


OAL_STATIC OAL_INLINE oal_bool_enum_uint8  oal_cmp_seq_num(
                oal_uint32   ul_seq_num1,
                oal_uint32   ul_seq_num2,
                oal_uint32   ul_diff_value)
{
    if (((ul_seq_num1 < ul_seq_num2) && ((ul_seq_num2 - ul_seq_num1) < ul_diff_value))
       ||((ul_seq_num1 > ul_seq_num2) && ((ul_seq_num1- ul_seq_num2) > ul_diff_value)))
    {
        return OAL_TRUE;
    }

    return OAL_FALSE;
}


OAL_STATIC OAL_INLINE oal_int32 oal_strcmp(const oal_int8 *pc_src, const oal_int8 *pc_dst)
{
    oal_int8  c_c1;
    oal_int8  c_c2;
    oal_int32 l_ret = 0;

    do
    {
        c_c1 = *pc_src++;
        c_c2 = *pc_dst++;
        l_ret = c_c1 - c_c2;
        if (l_ret)
        {
            break;
        }
    }while (c_c1);

    return l_ret;
}


OAL_STATIC OAL_INLINE oal_int8 *oal_strim(oal_int8 *pc_s)
{
    oal_uint32 ul_size;
    oal_int8  *pc_end;

    while (' ' == *pc_s)
    {
        ++pc_s;
    }

    ul_size = OAL_STRLEN(pc_s);
    if (!ul_size)
    {
        return pc_s;
    }

    pc_end = pc_s + ul_size - 1;
    while (pc_end >= pc_s && ' ' == *pc_end)
    {
        pc_end--;
    }

    *(pc_end + 1) = '\0';

    return pc_s;
}


OAL_STATIC OAL_INLINE oal_int8  *oal_strcat(oal_int8 *dest, const oal_int8 *src)
{
    oal_int8   *pc_tmp;

    pc_tmp = dest;

    while (*dest)
    {
        dest++;
    }

    while ((*dest++ = *src++) != '\0')
    {
        ; //do nothing
    }

    return pc_tmp;
}


OAL_STATIC OAL_INLINE oal_int8  *oal_strncat(oal_int8 *dest, const oal_int8 *src, oal_int32 l_cnt)
{
    oal_int8   *pc_tmp;

    pc_tmp = dest;

    if (l_cnt <= 0)
    {
        return pc_tmp;
    }

    while (*dest)
    {
        dest++;
    }

    while ((*dest++ = *src++) != '\0')
    {
        if (--l_cnt == 0)
        {
            *dest = '\0';
            break;
        }
    }

    return pc_tmp;
}


OAL_STATIC OAL_INLINE oal_int8*  oal_strstr(oal_int8 *pc_s1, oal_int8 *pc_s2)
{
    return OAL_STRSTR(pc_s1, pc_s2);
}



OAL_STATIC OAL_INLINE oal_uint32  oal_init_lut(oal_uint8  *puc_lut_index_table, oal_uint8 uc_bmap_len)
{
    oal_uint8   uc_lut_idx;

    for (uc_lut_idx = 0; uc_lut_idx < uc_bmap_len; uc_lut_idx++)
    {
        puc_lut_index_table[uc_lut_idx] = 0;
    }

    return OAL_SUCC;
}


OAL_STATIC OAL_INLINE oal_uint8  oal_get_lut_index(
                oal_uint8      *puc_lut_index_table,
                oal_uint8       uc_bmap_len,
                oal_uint16      us_max_lut_size,
                oal_uint16      us_start,
                oal_uint16      us_stop)
{
    oal_uint8       uc_byte     = 0;
    oal_uint8       uc_bit      = 0;
    oal_uint8       uc_temp     = 0;
    oal_uint16      us_index    = 0;

    for (uc_byte = 0; uc_byte < uc_bmap_len; uc_byte++)
    {
        uc_temp = puc_lut_index_table[uc_byte];

        for (uc_bit = 0; uc_bit < 8; uc_bit++)
        {
            if (0x0 == (uc_temp & (1 << uc_bit)))
            {
                us_index = (uc_byte * 8 + uc_bit);
                if ((us_index < us_start) || (us_index >= us_stop))
                {
                    continue;
                }
                if (us_index < us_max_lut_size)
                {
                    puc_lut_index_table[uc_byte] |= (oal_uint8)(1 << uc_bit);

                    return (oal_uint8)us_index;
                }
                else
                {
                    return (oal_uint8)us_max_lut_size;
                }
            }
        }
    }

    return (oal_uint8)us_max_lut_size;
}


OAL_STATIC OAL_INLINE oal_void  oal_del_lut_index(oal_uint8 *puc_lut_index_table, oal_uint8 uc_idx)
{
    oal_uint8 uc_byte = uc_idx >> 3;
    oal_uint8 uc_bit  = uc_idx & 0x07;

    puc_lut_index_table[uc_byte] &= ~(oal_uint8)(1 << uc_bit);
}

/****************************************************************************/
OAL_STATIC OAL_INLINE oal_bool_enum oal_is_active_lut_index(oal_uint8 *puc_lut_idx_status_table, oal_uint16 us_max_lut_size, oal_uint8 uc_idx)
{
    oal_uint8 uc_byte = uc_idx >> 3;
    oal_uint8 uc_bit  = uc_idx & 0x07;

    if(uc_idx >= us_max_lut_size)
    {
        return OAL_FALSE;
    }

    return puc_lut_idx_status_table[uc_byte] & ((oal_uint8)(1 << uc_bit)) ? OAL_TRUE : OAL_FALSE;
}

/****************************************************************************/
OAL_STATIC OAL_INLINE oal_void  oal_set_lut_index_status(oal_uint8 *puc_lut_idx_status_table,  oal_uint16 us_max_lut_size, oal_uint8 uc_idx)
{
    oal_uint8 uc_byte = uc_idx >> 3;
    oal_uint8 uc_bit  = uc_idx & 0x07;

    if(uc_idx >= us_max_lut_size)
    {
        return;
    }

    puc_lut_idx_status_table[uc_byte] |= (oal_uint8)(1 << uc_bit);
}

/****************************************************************************/
OAL_STATIC OAL_INLINE oal_void  oal_reset_lut_index_status(oal_uint8 *puc_lut_idx_status_table,  oal_uint16  us_max_lut_size, oal_uint8 uc_idx)
{
    oal_uint8 uc_byte = uc_idx >> 3;
    oal_uint8 uc_bit  = uc_idx & 0x07;

    if(uc_idx >= us_max_lut_size)
    {
        return;
    }

    puc_lut_idx_status_table[uc_byte] &= ~(oal_uint8)(1 << uc_bit);
}





OAL_STATIC OAL_INLINE oal_uint32* oal_get_virt_addr(oal_uint32 *pul_phy_addr)
{
    /* 空指针无需转化 */
    if (OAL_PTR_NULL == pul_phy_addr)
    {
        return pul_phy_addr;
    }

    return (oal_uint32 *)OAL_PHY_TO_VIRT_ADDR((oal_uint)pul_phy_addr);
}

extern oal_int32 oal_dump_stack_str(oal_uint8 *puc_str, oal_uint32 ul_max_size);

OAL_STATIC OAL_INLINE oal_int8 oal_get_real_rssi(oal_int16 s_scaled_rssi)
{
    /*四舍五入*/
    return OAL_ROUND(s_scaled_rssi, OAL_RSSI_FRACTION_BITS);
}

OAL_STATIC OAL_INLINE oal_void oal_rssi_smooth(oal_int16 *ps_old_rssi, oal_int8 c_new_rssi)
{
    oal_int16 s_delta;

    /*若上报的值超过了合法范围，则不进行平滑操作，函数直接返回*/
    if(c_new_rssi < OAL_RSSI_SIGNAL_MIN || c_new_rssi > OAL_RSSI_SIGNAL_MAX)
    {
        return;
    }

    /* 若上报的值为0，则是描述符未填rssi值,不进行平滑 */
    if(0 == c_new_rssi)
    {
        return;
    }

    /*判断为初始值,芯片上报的rssi作为平滑结果*/
    if (OAL_RSSI_INIT_MARKER == *ps_old_rssi)
    {
        *ps_old_rssi = (oal_int16)c_new_rssi << OAL_RSSI_FRACTION_BITS;
    }

    /*old_rssi四舍五入后计算delta*/
    s_delta = (oal_int16)c_new_rssi - oal_get_real_rssi(*ps_old_rssi);

    if (s_delta > OAL_RSSI_MAX_DELTA)
    {
        s_delta = OAL_RSSI_MAX_DELTA;
    }
    if (s_delta < -OAL_RSSI_MAX_DELTA)
    {
        s_delta = -OAL_RSSI_MAX_DELTA;
    }
    *ps_old_rssi += s_delta;
}


#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of oal_util.h */
