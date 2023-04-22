

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif



/*
 * 1 Header File Including
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/firmware.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/moduleparam.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/rtc.h>
#include <linux/syscalls.h>
#include <linux/dma-mapping.h>
#include <linux/stat.h>
#include <linux/string.h>

#include "wireless_patch.h"
#include "plat_pm.h"
/*
#include "wireless_board.h"
*/

/*
 * 2 Global Variable Definition
 */

PATCH_GLOBALS_STUR              g_st_global[ENUM_INFO_TOTAL];
RINGBUF_STRU                    g_stringbuf;
uint8                          *g_pucDataBuf_t;

/* xmodem 索引 */
uint8                           g_index = 1;

/* log 打印级别 */
int32                           g_debuglevel_patch = 2;

int32                           g_usemalloc = 0;


unsigned short CRC_table[256] =
{
/* CRC 余式表 */
    0X0000, 0X1021, 0X2042, 0X3063, 0X4084, 0X50A5, 0X60C6, 0X70E7,
    0X8108, 0X9129, 0XA14A, 0XB16B, 0XC18C, 0XD1AD, 0XE1CE, 0XF1EF,
    0X1231, 0X0210, 0X3273, 0X2252, 0X52B5, 0X4294, 0X72F7, 0X62D6,
    0X9339, 0X8318, 0XB37B, 0XA35A, 0XD3BD, 0XC39C, 0XF3FF, 0XE3DE,
    0X2462, 0X3443, 0X0420, 0X1401, 0X64E6, 0X74C7, 0X44A4, 0X5485,
    0XA56A, 0XB54B, 0X8528, 0X9509, 0XE5EE, 0XF5CF, 0XC5AC, 0XD58D,
    0X3653, 0X2672, 0X1611, 0X0630, 0X76D7, 0X66F6, 0X5695, 0X46B4,
    0XB75B, 0XA77A, 0X9719, 0X8738, 0XF7DF, 0XE7FE, 0XD79D, 0XC7BC,
    0X48C4, 0X58E5, 0X6886, 0X78A7, 0X0840, 0X1861, 0X2802, 0X3823,
    0XC9CC, 0XD9ED, 0XE98E, 0XF9AF, 0X8948, 0X9969, 0XA90A, 0XB92B,
    0X5AF5, 0X4AD4, 0X7AB7, 0X6A96, 0X1A71, 0X0A50, 0X3A33, 0X2A12,
    0XDBFD, 0XCBDC, 0XFBBF, 0XEB9E, 0X9B79, 0X8B58, 0XBB3B, 0XAB1A,
    0X6CA6, 0X7C87, 0X4CE4, 0X5CC5, 0X2C22, 0X3C03, 0X0C60, 0X1C41,
    0XEDAE, 0XFD8F, 0XCDEC, 0XDDCD, 0XAD2A, 0XBD0B, 0X8D68, 0X9D49,
    0X7E97, 0X6EB6, 0X5ED5, 0X4EF4, 0X3E13, 0X2E32, 0X1E51, 0X0E70,
    0XFF9F, 0XEFBE, 0XDFDD, 0XCFFC, 0XBF1B, 0XAF3A, 0X9F59, 0X8F78,
    0X9188, 0X81A9, 0XB1CA, 0XA1EB, 0XD10C, 0XC12D, 0XF14E, 0XE16F,
    0X1080, 0X00A1, 0X30C2, 0X20E3, 0X5004, 0X4025, 0X7046, 0X6067,
    0X83B9, 0X9398, 0XA3FB, 0XB3DA, 0XC33D, 0XD31C, 0XE37F, 0XF35E,
    0X02B1, 0X1290, 0X22F3, 0X32D2, 0X4235, 0X5214, 0X6277, 0X7256,
    0XB5EA, 0XA5CB, 0X95A8, 0X8589, 0XF56E, 0XE54F, 0XD52C, 0XC50D,
    0X34E2, 0X24C3, 0X14A0, 0X0481, 0X7466, 0X6447, 0X5424, 0X4405,
    0XA7DB, 0XB7FA, 0X8799, 0X97B8, 0XE75F, 0XF77E, 0XC71D, 0XD73C,
    0X26D3, 0X36F2, 0X0691, 0X16B0, 0X6657, 0X7676, 0X4615, 0X5634,
    0XD94C, 0XC96D, 0XF90E, 0XE92F, 0X99C8, 0X89E9, 0XB98A, 0XA9AB,
    0X5844, 0X4865, 0X7806, 0X6827, 0X18C0, 0X08E1, 0X3882, 0X28A3,
    0XCB7D, 0XDB5C, 0XEB3F, 0XFB1E, 0X8BF9, 0X9BD8, 0XABBB, 0XBB9A,
    0X4A75, 0X5A54, 0X6A37, 0X7A16, 0X0AF1, 0X1AD0, 0X2AB3, 0X3A92,
    0XFD2E, 0XED0F, 0XDD6C, 0XCD4D, 0XBDAA, 0XAD8B, 0X9DE8, 0X8DC9,
    0X7C26, 0X6C07, 0X5C64, 0X4C45, 0X3CA2, 0X2C83, 0X1CE0, 0X0CC1,
    0XEF1F, 0XFF3E, 0XCF5D, 0XDF7C, 0XAF9B, 0XBFBA, 0X8FD9, 0X9FF8,
    0X6E17, 0X7E36, 0X4E55, 0X5E74, 0X2E93, 0X3EB2, 0X0ED1, 0X1EF0
};

uint8 powerpin_state;
module_param(g_debuglevel_patch, int, 0);

/*
 * 3 Function Definition
 */
extern int32 pm_uart_send(uint8 *date, int32 len);
extern int32 pm_uart_set_baudrate(int64 baudrate);


int32 pm_uart_set_baudrate(int64 baudrate)
{
    /*get platform driver data*/
    struct pm_drv_data *pm_data = pm_get_drvdata_etc();
    if (NULL == pm_data)
    {
        PS_PRINT_ERR("pm_data is NULL!\n");
        return -1;
    }

    PS_PRINT_DBG("set uart baudrate to %ld\n", baudrate);

    /*call interface supplied by 3 in 1*/
    if (NULL != pm_data->ps_pm_interface->change_baud_rate)
    {
        /*set uart baudrate*/
        return pm_data->ps_pm_interface->change_baud_rate(baudrate, FLOW_CTRL_ENABLE);
    }

    PS_PRINT_ERR("change_baud_rate is NULL!\n");

    return -1;
}


/*****************************************************************************
 Prototype    : patch_Crc16
 Description  : CRC校验
 Input        : int8 *ptr
                uint16 l_count
 Output       :
 Return Value : uint16
 Calls        :
 Called By    :

  History        :
  1.Date         : 2012/11/9
    Author       : kf74033
    Modification : Created function

*****************************************************************************/

unsigned short do_crc_table_1(uint8 *data, uint16 length)
{
    uint16  crc = 0;
    uint8   dataTmp;
    while(length > 0)
    {
        dataTmp = (uint8)(crc >> 8);
        crc = crc <<8;
        crc = crc^CRC_table[dataTmp^(*data)];
        length--;
        data++;
    }
    return crc;
}

uint16 patch_Crc16 (int8 *ptr, uint16 l_count)
{
    uint16      us_crc;
    int32       i;

    us_crc = 0;
#if 1
    while(l_count--)
    {
        us_crc = us_crc ^ (int) *ptr++ << 8;

        for (i = 0; i < 8; i++)
        {
            if(us_crc & 0x8000)
            {
                us_crc = us_crc << 1 ^ 0x1021;
            }
            else
            {
                us_crc = us_crc << 1;
            }
        }
    }
#endif
    return (us_crc & 0xFFFF);
}

/*****************************************************************************
 Prototype    : patch_send
 Description  : send message to device,by sdio or uart
 Input        : uint8 *data
                int32 len
                uint8 expect
                int32 type
 Output       :
 Return Value : int32
 Calls        :
 Called By    :

  History        :
  1.Date         : 2012/11/12
    Author       : kf74033
    Modification : Created function

*****************************************************************************/
int32 patch_send(uint8 *data, int32 len, uint8 expect, int32 type)
{
    int32  i;
    int32  l_ret;

    PS_PRINT_DBG("len = %d, data = %x %x %x %x %x %x %x %x \n",
                len, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);

    for(i = 0; i < HOST_DEV_TIMEOUT; i++)
    {
        l_ret = send_msg_t(data, len, type);
        if (0 > l_ret)
        {
            continue;
            //return -EFAIL;
        }

        l_ret = recv_expect_result_t(expect, type);
        if (0 == l_ret)
        {
            return SUCC;
        }
    }
    return -EFAIL;

}

/*****************************************************************************
 Prototype    : patch_xmodem_send
 Description  : xmodem protocol encapsulation for down file
 Input        : uint8 *data
                int32 len
                uint8 expect
 Output       :
 Return Value : int32
 Calls        :
 Called By    :

  History        :
  1.Date         : 2012/11/12
    Author       : kf74033
    Modification : Created function

*****************************************************************************/

int32 patch_xmodem_send(uint8 *data, int32 len, uint8 expect)
{
    XMODEM_HEAD_PKT_STRU        st_patch_pkt;
    uint16                      us_CRCValue;
    int32                       l_ret;
    int32                       l_sendlen;
    int32                       l_datalen;
    int32                       i;
    uint8                      *flag;
    uint8                       CRCValue_H;
    uint8                       CRCValue_L;

    PS_PRINT_DBG("enter");

    if (NULL == data)
    {
        return -EFAIL;
    }

    l_datalen = len;
    flag      = data;

//    pst_patch_pkt = OS_KMALLOC_GFP(XMODEM_PAK_LEN);
    while(l_datalen > 0)
    {
        l_sendlen = MIN(XMODE_DATA_LEN, l_datalen);
        l_datalen  = l_datalen - l_sendlen ;
//        OS_MEM_SET((void *)pst_patch_pkt, 0, sizeof(XMODEM_CRC_PKT_STRU));
        st_patch_pkt.Head       = SOH;
        st_patch_pkt.PacketNum  = g_index;

        /* 构包 */
//        OS_MEM_CPY(pst_patch_pkt->packet_data, flag, l_sendlen);
//        flag = flag + l_sendlen;

        /* 数据长度不够128个 */
        if (XMODE_DATA_LEN > l_sendlen)
        {
            PS_PRINT_DBG("data_len  %d\n", l_sendlen);
            OS_MEM_SET(&flag[l_sendlen], 0x00, (XMODE_DATA_LEN - l_sendlen));
        }
//        us_CRCValue = patch_Crc16((int8 *)flag, XMODE_DATA_LEN);
        us_CRCValue = do_crc_table_1(flag, XMODE_DATA_LEN);
        CRCValue_H = (us_CRCValue & 0xFF00)>>8;
        CRCValue_L =  us_CRCValue & 0xFF;

        st_patch_pkt.PacketAnt = ~(st_patch_pkt.PacketNum);

        for (i = 0; i < HOST_DEV_TIMEOUT; i++)
        {
            l_ret = pm_uart_send((uint8 *)&st_patch_pkt, 3);
            l_ret = pm_uart_send(flag, XMODE_DATA_LEN);
            l_ret = pm_uart_send(&CRCValue_H, 1);
            l_ret = pm_uart_send(&CRCValue_L, 1);

            l_ret = recv_expect_result_t(expect, ENUM_INFO_UART);
            if (0 > l_ret)
            {
                continue;
            }

#if 0
            if (0 <= l_ret)
            {
                break;
            }
#endif

            if (SUCC == l_ret)
            {
                break;
            }

        }
        if (i >= HOST_DEV_TIMEOUT)
        {
//            OS_MEM_KFREE(pst_patch_pkt);
            return -EFAIL;
        }
        flag = flag + l_sendlen;

        g_index++;
    }
//    OS_MEM_KFREE(pst_patch_pkt);

    return SUCC;
}


/*****************************************************************************
 Prototype    : patch_send_file
 Description  : send file to device,by sdio or uart
 Input        : uint8 *data
                int32 len
                uint8 expect
                int32 type
 Output       :
 Return Value : int32
 Calls        :
 Called By    :

  History        :
  1.Date         : 2012/11/12
    Author       : kf74033
    Modification : Created function

*****************************************************************************/
int32 patch_send_file(uint8 *data, int32 len, uint8 expect, int32 type)
{
    int32   l_ret;
    PS_PRINT_DBG("len = %d, data = %x %x %x %x %x %x %x %x \n",
                len, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);

    switch (type)
    {
        case ENUM_INFO_SDIO:
        {
            l_ret = patch_send(data, len, expect, type);
        }
        break;

        case ENUM_INFO_UART:/* uart READ */
        {
            l_ret = patch_xmodem_send(data, len, expect);
        }
        break;

        default:

        l_ret = -1;
        PS_PRINT_ERR("info type error[%d]!!!\n", type);
        break;

    }

    return l_ret;
}


/*****************************************************************************
 Prototype    : patch_send_data
 Description  : send data to device,by sdio or uart
 Input        : uint8 *data
                int32 len
                uint8 expect
                int32 type
 Output       :
 Return Value : int32
 Calls        :
 Called By    :

  History        :
  1.Date         : 2012/11/12
    Author       : kf74033
    Modification : Created function

*****************************************************************************/
int32 patch_send_data(uint8 *data, int32 len, uint8 expect, int32 type)
{
    return patch_send(data, len, expect, type);
}

static void ringbuf_flush(void)
{
    /* writing maybe still work when interrupt by flush  */
    g_stringbuf.ptail = g_stringbuf.phead;
}

/*****************************************************************************
 Prototype    : uart_recv_data
 Description  : receiver data form device,by uart interface
 Input        : uint8 *data
                int32 len
 Output       :
 Return Value : int32
 Calls        :
 Called By    :

  History        :
  1.Date         : 2012/11/12
    Author       : kf74033
    Modification : Created function

*****************************************************************************/
int32 uart_recv_data(const uint8 *data, int32 len)
{
    uint32 ulbuflen = 0;
    uint32 ulheadtoendlen = 0;
    uint8 *ptail;
    if (unlikely((NULL == data)))
    {
        PS_PRINT_ERR("data is NULL\n ");
        return -EFAIL;
    }
    if ((NULL == g_stringbuf.pbufstart) || (g_stringbuf.pbufstart > g_stringbuf.pbufend))
    {
        PS_PRINT_ERR("buf is NULL,write failed\n ");
        PS_PRINT_ERR("pbufstart=%p,pbufend=%p\n", g_stringbuf.pbufstart, g_stringbuf.pbufend);
        return -EFAIL;
    }
    ptail = g_stringbuf.ptail;
    if ((g_stringbuf.phead < g_stringbuf.pbufstart)
     || (g_stringbuf.phead > g_stringbuf.pbufend)
     || (ptail < g_stringbuf.pbufstart)
     || (ptail > g_stringbuf.pbufend))
    {
        PS_PRINT_ERR("phead or ptail is out of range,write failed\n");
        PS_PRINT_ERR("pbufstart=%p,pbufend=%p\n", g_stringbuf.pbufstart, g_stringbuf.pbufend);
        PS_PRINT_ERR("phead=%p,ptail=%p\n", g_stringbuf.phead, g_stringbuf.ptail);
        return -EFAIL;
    }
    ulbuflen = g_stringbuf.pbufend - g_stringbuf.pbufstart;
    PS_PRINT_DBG("len [%d],\n ", len);
    
    /* phead and ptail are in the same cycle */
    if (g_stringbuf.phead >= ptail)
    {
        /* still in the same cycle */
        if ((g_stringbuf.pbufend - g_stringbuf.phead) >= len)
        {
            OS_MEM_CPY(g_stringbuf.phead, data, len);
            g_stringbuf.phead += len;
            if (OS_WAITQUEUE_ACTIVE(g_st_global[ENUM_INFO_UART].pst_wait))
            {
                OS_WAKE_UP_INTERRUPTIBLE(g_st_global[ENUM_INFO_UART].pst_wait);
                PS_PRINT_DBG("wake up ok");
            }
        }
        else if ((ulbuflen - (g_stringbuf.phead - ptail)) > len)
        {
            ulheadtoendlen = g_stringbuf.pbufend - g_stringbuf.phead;
            OS_MEM_CPY(g_stringbuf.phead, data, ulheadtoendlen);
            OS_MEM_CPY(g_stringbuf.pbufstart, data + ulheadtoendlen, len - ulheadtoendlen);
            g_stringbuf.phead = g_stringbuf.pbufstart + (len - ulheadtoendlen);
            PS_PRINT_INFO("phead back\n");
            if (OS_WAITQUEUE_ACTIVE(g_st_global[ENUM_INFO_UART].pst_wait))
            {
                OS_WAKE_UP_INTERRUPTIBLE(g_st_global[ENUM_INFO_UART].pst_wait);
                PS_PRINT_DBG("wake up ok");
            }
        }
        else
        {
            PS_PRINT_ERR("Not enough mem,len=%d.\n ", len);
        }
    }
    /* phead is in the next cycle */
    /* "ptail - phead = 1" means the buffer is full */
    else if ((ptail - g_stringbuf.phead - 1) > len)
    {
            OS_MEM_CPY(g_stringbuf.phead, data, len);
            g_stringbuf.phead += len;
            if (OS_WAITQUEUE_ACTIVE(g_st_global[ENUM_INFO_UART].pst_wait))
            {
                OS_WAKE_UP_INTERRUPTIBLE(g_st_global[ENUM_INFO_UART].pst_wait);
                PS_PRINT_DBG("wake up ok");
            }
    }
    else
    {
        PS_PRINT_ERR("Not enough mem,len=%d.\n ", len);
    }

    if (g_stringbuf.phead >= g_stringbuf.pbufend)
    {
        g_stringbuf.phead = g_stringbuf.pbufstart;
        PS_PRINT_INFO("phead back\n");
    }
    return SUCC;
}

/*****************************************************************************
 Prototype    : read_msg_etc
 Description  : read msg
 Input        : uint8 *data
                int32 len
                int32 type
 Output       :
 Return Value : int32
 Calls        :
 Called By    :

  History        :
  1.Date         : 2012/11/12
    Author       : kf74033
    Modification : Created function

*****************************************************************************/
int32 read_msg_t(uint8 *data, int32 len, int32 type)
{
    int32 l_len = -1;
    uint32 ultailtoendlen;
    uint8 *phead;
    PS_PRINT_DBG(" entry\n");
    if (unlikely((NULL == data)))
    {
        PS_PRINT_ERR("data is NULL\n ");
        return -EFAIL;
    }
    if (ENUM_INFO_UART == type)
    {
        if ((NULL == g_stringbuf.pbufstart) || (g_stringbuf.pbufstart > g_stringbuf.pbufend))
        {
            PS_PRINT_ERR("buf is NULL,read failed\n ");
            PS_PRINT_ERR("pbufstart=%p,pbufend=%p\n", g_stringbuf.pbufstart, g_stringbuf.pbufend);
            return -EFAIL;
        }
        if ((g_stringbuf.phead < g_stringbuf.pbufstart)
         || (g_stringbuf.phead > g_stringbuf.pbufend)
         || (g_stringbuf.ptail < g_stringbuf.pbufstart)
         || (g_stringbuf.ptail > g_stringbuf.pbufend))
        {
            PS_PRINT_ERR("phead or ptail is out of range, read failed\n");
            PS_PRINT_ERR("pbufstart=%p,pbufend=%p\n", g_stringbuf.pbufstart, g_stringbuf.pbufend);
            PS_PRINT_ERR("phead=%p,ptail=%p\n", g_stringbuf.phead, g_stringbuf.ptail);
            return -EFAIL;
        }
    }
    switch (type)
    {
        case ENUM_INFO_SDIO:
        break;
        case ENUM_INFO_UART:
        {
            OS_WAIT_EVENT_INTERRUPTIBLE_TIMEOUT(*g_st_global[ENUM_INFO_UART].pst_wait,
                                                (g_stringbuf.phead != g_stringbuf.ptail),
                                                 PATCH_INTEROP_TIMEOUT);
            phead = g_stringbuf.phead;
            /* phead and ptail are in the same cycle */
            if (phead > g_stringbuf.ptail)
            {
                if ((phead - g_stringbuf.ptail) > len)
                {
                    OS_MEM_CPY(data, g_stringbuf.ptail, len);
                    l_len = len;
                    g_stringbuf.ptail += len;
                }
                /* not enough data */
                else
                {
                    l_len = phead - g_stringbuf.ptail;
                    OS_MEM_CPY(data, g_stringbuf.ptail, l_len);
                    g_stringbuf.ptail += l_len;
                }
            }
            /* phead is in the next cycle */
            else if (phead < g_stringbuf.ptail)
            {
                ultailtoendlen = g_stringbuf.pbufend - g_stringbuf.ptail;
                if (ultailtoendlen > len)
                {
                    OS_MEM_CPY(data, g_stringbuf.ptail, len);
                    l_len = len;
                    g_stringbuf.ptail += len;
                }
                else
                {
                    OS_MEM_CPY(data, g_stringbuf.ptail, ultailtoendlen);
                    if ((phead - g_stringbuf.pbufstart) > (len - ultailtoendlen))
                    {
                        OS_MEM_CPY(data + ultailtoendlen, g_stringbuf.pbufstart, len - ultailtoendlen);
                        g_stringbuf.ptail = g_stringbuf.pbufstart + (len - ultailtoendlen);
                        l_len = len;
                        PS_PRINT_INFO("ptail back\n");
                    }
                    else
                    {
                        OS_MEM_CPY(data + ultailtoendlen, g_stringbuf.pbufstart, phead - g_stringbuf.pbufstart);
                        l_len = ultailtoendlen + (phead - g_stringbuf.pbufstart);
                        g_stringbuf.ptail = phead;
                    }
                }
            }
            else
            {
                l_len = -1;
                PS_PRINT_WARNING("No data.\n");
            }
            if (g_stringbuf.ptail >= g_stringbuf.pbufend)
            {
                g_stringbuf.ptail = g_stringbuf.pbufstart;
            }
        }

        break;

        default:

        l_len = -1;
        PS_PRINT_ERR("info type error[%d]!!!\n", type);
        break;

    }

    return l_len;

}


/*****************************************************************************
Prototype    : send_msg_etc
Description  : send message to device,by sdio or uart
Input        : uint8 *data
               int32 len
               int32 type
Output       :
Return Value : int32
Calls        :
Called By    :

History        :
1.Date         : 2012/11/12
  Author       : kf74033
  Modification : Created function

*****************************************************************************/
int32 send_msg_t(uint8 *data, int32 len, int32 type)
{
    int32   l_ret = -1;
    PS_PRINT_DBG("len = %d, data = %x %x %x %x %x %x %x %x \n",
              len, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);

    switch (type)
    {
        case ENUM_INFO_SDIO:
            //l_ret = pm_sdio_send(data, len);
            break;

        case ENUM_INFO_UART:
            l_ret = pm_uart_send(data, len);
            break;

        default:
            l_ret = -EFAIL;
            PS_PRINT_ERR("info type error[%d]!!!\n", type);
            break;
    }

    return l_ret;
}

/*****************************************************************************
 Prototype    : recv_expect_result_etc
 Description  : receive result form device
 Input        : uint8 expect
                int32 type
 Output       :
 Return Value : int32
 Calls        :
 Called By    :

  History        :
  1.Date         : 2012/11/1
    Author       : kf74033
    Modification : Created function

*****************************************************************************/
int32 recv_expect_result_t(uint8 expect, int32 type)
{
    uint8 auc_buf[RECV_BUF_LEN];
    int32 l_len;
    int32 i;

    PS_PRINT_DBG(" entry\n");

    OS_MEM_SET(auc_buf, 0, RECV_BUF_LEN);
    for (i = 0; i < HOST_DEV_TIMEOUT; i++)
    {
        l_len = read_msg_t(auc_buf, 1, type);
        if (0 > l_len)
        {
            PS_PRINT_ERR("recv result fail\n");
            return -EFAIL;
        }
        if (auc_buf[0] == expect)
        {
            PS_PRINT_DBG(" send SUCC [%x]\n", expect);
            return SUCC;
        }
        /*
         * NAK: 文件传输时重发标识
         * MSG_FORM_DRV_N:其他重发标识
         */
        else if ((MSG_FORM_DRV_N == auc_buf[0]) || (NAK == auc_buf[0])
              || (MSG_FORM_DRV_C == auc_buf[0]))
        {
            PS_PRINT_ERR(" send again [0x%x]\n", auc_buf[0]);
            return -EFAIL;
        }
        else
        {
            /* 对于错误的结果，有十次的机会， */
            if (ENUM_INFO_SDIO == type)
            {
                PATCH_SEND_N_SDIO;
            }
            else
            {
                PATCH_SEND_N_UART;
            }
            PS_PRINT_WARNING(" error result[0x%x], expect [0x%x], read result again\n", auc_buf[0], expect);
        }
    }
        return -EFAIL;
}

/*****************************************************************************
 Prototype    : patch_string_to_num
 Description  : string to number
 Input        : int8 *string
 Output       :
 Return Value : int32
 Calls        :
 Called By    :

  History        :
  1.Date         : 2012/11/14
    Author       : kf74033
    Modification : Created function

*****************************************************************************/
int32 patch_string_to_num(uint8 *string)
{
    int32       i;
    int32       l_num;
    PS_PRINT_DBG(" entry\n");
    if (NULL == string)
    {
        return -EFAIL;
    }

    l_num = 0;
    for (i = 0; (string[i] >= '0') && (string[i] <= '9'); i++)
    {
        l_num = (l_num * 10) + (string[i] - '0');
    }

    return l_num;
}

/*****************************************************************************
 Prototype    : patch_wait_g_form_dev
 Description  : wait go'command form device
 Input        : int32 type
 Output       :
 Return Value : int32
 Calls        :
 Called By    :

  History        :
  1.Date         : 2012/11/14
    Author       : kf74033
    Modification : Created function

*****************************************************************************/
int32 patch_wait_g_form_dev(int32 type)
{
    int32 l_ret;
    int32 i;

    PS_PRINT_DBG(" entry\n");

    for (i = 0; i < HOST_DEV_TIMEOUT; i++)
    {
        l_ret = recv_expect_result_t(MSG_FORM_DRV_G, type);
        if (0 == l_ret)
        {
            PS_PRINT_DBG(" device finish G\n" );
            return SUCC;
        }
    }
    return -EFAIL;
}

/*****************************************************************************
 Prototype    : patch_wait_g_form_dev
 Description  : wait go'command form device
 Input        : int32 type
 Output       :
 Return Value : int32
 Calls        :
 Called By    :

  History        :
  1.Date         : 2012/11/14
    Author       : kf74033
    Modification : Created function

*****************************************************************************/
int32 patch_wait_g_retry_form_dev(int32 type)
{
    int32 l_ret;
    int32 i;

    PS_PRINT_DBG(" entry\n");

    for (i = 0; i < HOST_DEV_TIMEOUT; i++)
    {
        l_ret = recv_expect_result_t(MSG_FORM_DRV_G, type);
        if (0 == l_ret)
        {
            PS_PRINT_DBG(" device finish G\n" );
            return SUCC;
        }
        else
        {
            if (ENUM_INFO_UART == type)
            {
                PATCH_SEND_N_UART;
            }
            PS_PRINT_WARNING("receive G failed\n" );
        }
    }
    return -EFAIL;
}


/*****************************************************************************
 Prototype    : patch_send_char
 Description  : send char to device
 Input        : int8 num
                int32 wait
                int32 type
 Output       :
 Return Value : int32
 Calls        :
 Called By    :

  History        :
  1.Date         : 2012/11/14
    Author       : kf74033
    Modification : Created function

*****************************************************************************/
int32 patch_send_char(int8 num, int32 wait, int32 type)
{
    int32   l_ret;
    uint8   auc_buf[8];
    int32   i;
    PS_PRINT_DBG("enter");

    OS_MEM_SET(auc_buf, num, 8);
    PS_PRINT_DBG("send [0x%x], wait[%d]\n", num, wait);
    for(i = 0; i < HOST_DEV_TIMEOUT; i++)
    {
        /*
         * sdio 接口发送时，会四字节对齐，发送四个
         * uart 接口发送时，只发送一个
         */
        l_ret = send_msg_t(auc_buf, 1, type);
        if (0 > l_ret)
        {
            PS_PRINT_ERR("Send fail\n");
            return l_ret;
        }

        if (WAIT_RESPONSE == wait)
        {
            l_ret = recv_expect_result_t(ACK, type);
            if (0 > l_ret)
            {
                continue;
            }
        }

        return l_ret;

    }

    return -EFAIL;;

}

/*****************************************************************************
 Prototype    : patch_read_patch
 Description  : read patch
 Input        : int32 len
                OS_KERNEL_FILE_STRU *fp
 Output       : int8 *buf
 Return Value : int32
 Calls        :
 Called By    :

  History        :
  1.Date         : 2012/11/14
    Author       : kf74033
    Modification : Created function

*****************************************************************************/
int32 patch_read_patch(int8 *buf, int32 len, OS_KERNEL_FILE_STRU *fp)
{
    int32 rdlen;

    if ((IS_ERR(fp)) || (NULL == buf))
    {
        fp = NULL;
        PS_PRINT_ERR("buf/fp is NULL\n");
        return -EFAIL;
    }

    rdlen = kernel_read(fp, fp->f_pos, buf, len);
    if (rdlen > 0)
    {
        fp->f_pos += rdlen;
    }

    return rdlen;
}

/*****************************************************************************
 Prototype    : patch_down_file
 Description  : begin download patch file
 Input        : uint8 *puc_file
                int32 type
 Output       :
 Return Value : int32
 Calls        :
 Called By    :

  History        :
  1.Date         : 2012/11/1
    Author       : kf74033
    Modification : Created function

*****************************************************************************/
int32 patch_down_file(uint8 *puc_file, int32 type)
{
    OS_KERNEL_FILE_STRU        *fp;
    uint8                      *auc_buf;
    int32                       l_len;
    int32                       l_ret;
    int32                       l_count;

    PS_PRINT_DBG("enter");

    if (NULL == puc_file)
    {
        return -EFAIL;
    }

    fp = filp_open(puc_file, O_RDONLY, 0);
    if (IS_ERR(fp))
    {
    fp = NULL;
    PS_PRINT_ERR("filp_open %s fail!!\n", puc_file);
        return -EFAIL;
    }

//    auc_buf = OS_KMALLOC_GFP(READ_PATCH_BUF_LEN);
//    auc_buf = g_pucDataBuf_etc;
    if (NULL == g_pucDataBuf_t)
    {
        filp_close(fp, NULL);
        fp = NULL;
        return -EFAIL;
    }

    l_count = 1;
    g_index = 1;
    while(1)
    {
//        OS_MEM_SET(g_pucDataBuf_etc, 0, READ_PATCH_BUF_LEN);
#if 1
        l_len = patch_read_patch(g_pucDataBuf_t, READ_PATCH_BUF_LEN, fp);
#else
        l_len = 128;
#endif
        PS_PRINT_DBG("kernel_read len[%d] [%d]\n", l_len, l_count);
        /* 正常读取文件 */
        if ((0 < l_len) && (l_len <= READ_PATCH_BUF_LEN))
        {
            l_ret = patch_send_file(g_pucDataBuf_t, l_len, ACK, type);
            PS_PRINT_DBG("patch_send_file:%d", l_ret);
            if(-EFAIL == l_ret)
            {
                PS_PRINT_ERR(" uart send data[%d] fail\n", l_count);
                break;
            }
        }
        /* 文件已经读取完成*/
        else if(0 == l_len)
        {
            if (ENUM_INFO_SDIO == type)
            {
                PATCH_SEND_EOT_SDIO;
            }
            else
            {
                PATCH_SEND_EOT_UART;
                g_index = 1;
            }
            l_ret = SUCC;

            PS_PRINT_DBG("read file[%d] [%d] send EOT\n", l_count, l_len);
            break;
        }
        /* 读取文件出错 */
        else
        {
            if (ENUM_INFO_SDIO == type)
            {
                PATCH_SEND_CAN_SDIO;
            }
            else
            {
                PATCH_SEND_CAN_UART;
                g_index = 1;
            }
            l_ret = -EFAIL;
            PS_PRINT_ERR("read file[%d] [%d]\n", l_count, l_len);
            break;
        }
        l_count++;
    }

//    OS_MEM_KFREE(auc_buf);
    auc_buf = NULL;

    filp_close(fp, NULL);
    fp = NULL;
    PS_PRINT_DBG("%s send finish\n", puc_file);

    return l_ret;
}

/*****************************************************************************
 Prototype    : patch_readm_fileopen
 Description  : creat and open file to save mem
 Input        : int32 type
 Output       :
 Return Value : OS_KERNEL_FILE_STRU *
 Calls        :
 Called By    :

  History        :
  1.Date         : 2012/11/1
    Author       : kf74033
    Modification : Created function

*****************************************************************************/
OS_KERNEL_FILE_STRU * patch_readm_fileopen(int32 type)
{
    OS_KERNEL_FILE_STRU *fp;
    struct timeval tv;
    struct rtc_time tm;
    char filename[50] = {0};
    do_gettimeofday(&tv);
    rtc_time_to_tm(tv.tv_sec, &tm);

    PS_PRINT_INFO("%4d-%02d-%02d  %02d:%02d:%02d\n",
                   tm.tm_year + 1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    if (ENUM_INFO_SDIO == type)
    {
        snprintf(filename, sizeof(filename) - 1, "/data/memdump/readm_wifi%04d%02d%02d%02d%02d%02d.bin",
                    tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                    tm.tm_hour, tm.tm_min, tm.tm_sec);
    }
    else
    {
        snprintf(filename, sizeof(filename) - 1, "/data/memdump/readm_bfg%04d%02d%02d%02d%02d%02d.bin",
                    tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                    tm.tm_hour, tm.tm_min, tm.tm_sec);
    }
    PS_PRINT_INFO("filename = %s",filename);
    fp = filp_open(filename, O_RDWR | O_CREAT, 0644);
    return fp;

}

/*****************************************************************************
 Prototype    : patch_recv_mem
 Description  : receive memory information form device
 Input        : int32 len
                int32 type
 Output       :
 Return Value : int32
 Calls        :
 Called By    :

  History        :
  1.Date         : 2012/11/1
    Author       : kf74033
    Modification : Created function

*****************************************************************************/
int32 patch_recv_mem(OS_KERNEL_FILE_STRU *fp, int32 len, int32 type)
{
    uint8 *pdatabuf;
    uint8 buf;
    int32 l_ret;
    int32 count;
    mm_segment_t fs;
    uint8 retry = 3;
    int32 lenbuf = 0;
    if (IS_ERR(fp))
    {
        PS_PRINT_ERR("fp is error,fp = 0x%p\n", fp);
        return -EFAIL;
    }

    PS_PRINT_DBG("expect recv len is [%d]\n", len);
    count = 0;

    l_ret = read_msg_t(&buf, 1, type);
    pdatabuf = OS_KMALLOC_GFP(len);
    if (NULL == pdatabuf)
    {
        filp_close(fp, NULL);
        return -EFAIL;
    }
    OS_MEM_SET(pdatabuf, 0, len);
    if ('a' != buf)
    {
        PS_PRINT_DBG("receive a fail, readmem anyway\n");
    }
//#if 0

    fs = get_fs();
    set_fs(KERNEL_DS);
    l_ret = vfs_llseek(fp, 0, SEEK_END);
/*
    PS_PRINT_INFO("pos = %ld", fp->f_pos);
*/
    while (len > lenbuf)//RECV_BUF_LEN)
    {
        l_ret = read_msg_t(pdatabuf + lenbuf, len - lenbuf, type);
        if (l_ret > 0)
        {
            lenbuf += l_ret;
        }
        else
        {
            retry--;
            lenbuf = 0;
            if (0 == retry)
            {
                l_ret = -EFAIL;
                PS_PRINT_ERR("time out\n");
                break;
            }
            if (ENUM_INFO_SDIO == type)
            {
                PATCH_SEND_N_SDIO;
            }
            else
            {
                ringbuf_flush();
                PATCH_SEND_N_UART;
            }
        }
    }
    if (len <= lenbuf)
    {
        vfs_write(fp, pdatabuf, len, &fp->f_pos);
    }
    filp_close(fp, NULL);
    set_fs(fs);
    if (ENUM_INFO_SDIO == type)
    {
        PATCH_SEND_A_SDIO;
    }
    else
    {
        ringbuf_flush();
        PATCH_SEND_A_UART;
    }
    OS_MEM_KFREE(pdatabuf);
    return l_ret;
}

/*****************************************************************************
 Prototype    : patch_int_para_send
 Description  : down integer to device
 Input        : int8 *name
                uint8 *Value
                int32 type
 Output       :
 Return Value : int32
 Calls        :
 Called By    :

  History        :
  1.Date         : 2012/11/1
    Author       : kf74033
    Modification : Created function

*****************************************************************************/
int32 patch_int_para_send(uint8 *name, uint8 *Value, int32 type)
{
    int32       l_ret;
    int32       data_len;
    int32       Value_len;
    int32       i;
    int32       n;
    uint8       auc_num[INT32_STR_LEN];
    uint8       data[DATA_BUF_LEN];

    PS_PRINT_DBG("entry ");

    Value_len = OS_STR_LEN((int8 *)Value);

    OS_MEM_SET(auc_num, 0, INT32_STR_LEN);
    OS_MEM_SET(data, 0, DATA_BUF_LEN);

    data_len = 0;
    data_len = OS_STR_LEN(name);
    OS_MEM_CPY(data, name, data_len);

    data[data_len] = COMPART_KEYWORD;
    data_len = data_len + 1;

    for (i = 0,n = 0; (i <= Value_len) && (n < INT32_STR_LEN); i++)
    {
        if ((',' == Value[i]) || (i == Value_len ))
        {
            PS_PRINT_DBG("auc_num = %s, i = %d, n = %d\n", auc_num,  i, n);
            if (0 == n)
            {
                continue;
            }
            OS_MEM_CPY((uint8 *)&data[data_len], auc_num, n);
            data_len = data_len + n;

            data[data_len] = COMPART_KEYWORD;
            data_len = data_len + 1;

            OS_MEM_SET(auc_num, 0, INT32_STR_LEN);
            n = 0;
        }
        else if (0x20 == Value[i])
        {
            continue;
        }
        else
        {
            auc_num[n] = Value[i];
            n++;
        }

    }

    PS_PRINT_DBG("data_len = %d, \n", data_len);
    PS_PRINT_DBG("data = %s, \n", data);

//    l_ret = patch_send_data(data, data_len, MSG_FORM_DRV_A, type);
    if (ENUM_INFO_UART == type)
    {
        ringbuf_flush();
    }
    l_ret = send_msg_t(data, data_len, type);

    return l_ret;
}

/*****************************************************************************
 Prototype    : patch_file_type
 Description  : down parameter
 Input        : uint8 *Key
                uint8 * Value
 Output       :
 Return Value : int32
 Calls        :
 Called By    :

  History        :
  1.Date         : 2012/11/1
    Author       : kf74033
    Modification : Created function

*****************************************************************************/
int32 patch_number_type(uint8 *Key, uint8 *Value, int32 type)
{
    int32       l_ret = -EFAIL;
    int32       num;
    uint8      *flag;
    uint8       pinstate;
    OS_KERNEL_FILE_STRU *fp;

    PS_PRINT_DBG("entry");
    
    if (!OS_MEM_CMP(Key, VER_CMD_KEYWORD, OS_STR_LEN(VER_CMD_KEYWORD)))
    {
        PS_PRINT_DBG("version \n");
        return SUCC;
    }

    if (!OS_STR_CMP((int8 *)Key, PLL_CMD_KEYWORD)
        || !OS_STR_CMP((int8 *)Key, JUMP_CMD_KEYWORD)
        || !OS_STR_CMP((int8 *)Key, WMEM_CMD_KEYWORD))
    {
        l_ret = patch_int_para_send(Key, Value, type);
        if (0 > l_ret)
        {
            PS_PRINT_ERR("send %s,%s fail \n", Key, Value);
            return l_ret;
        }
        /* G 是device 相应操作完成标志 */
        PS_PRINT_DBG("recv g form device\n");
        l_ret = patch_wait_g_form_dev(type);
        if (0 > l_ret)
        {
            PS_PRINT_ERR("recv g FAIL [%d]\n", l_ret);
            return -EFAIL;
        }
    }
    else if (!OS_STR_CMP((int8 *)Key, BRT_CMD_KEYWORD))
    {

        /* 修改波特率 */
        l_ret = patch_int_para_send(Key, Value, type);
        if (0 > l_ret)
        {
            PS_PRINT_ERR("send %s,%s fail \n", Key, Value);
            return l_ret;
        }
        PS_PRINT_DBG("change baudrate\n");
        msleep(1);
        //recv_expect_result_etc(MSG_FORM_DRV_A, type);

        num = patch_string_to_num(Value);

        /* 增加调用修改波特率函数 */

        l_ret = pm_uart_set_baudrate(num);
        if (0 > l_ret)
        {
            PS_PRINT_ERR(" modify baudrate fail!!\n");
            return -EFAIL;
        }
        ringbuf_flush();
        msleep(10);
        PATCH_SEND_N_UART;
        /* G 是device 相应操作完成标志 */
        PS_PRINT_DBG("recv g form device\n");
        l_ret = patch_wait_g_retry_form_dev(type);

        if (0 > l_ret)
        {
            return -EFAIL;
        }

    }
    else if (!OS_STR_CMP((int8 *)Key, RMEM_CMD_KEYWORD))
    {   
        /* not enough mem to read mem */
        if ((0 == g_usemalloc) && (ENUM_INFO_UART == type))
        {
            return SUCC;
        }
        powerpin_state &= PINSTATE_MASK;
        pinstate = (powerpin_state & (powerpin_state >> PINSTATE_NUM));
        /* disabled */
        if ((PINENABLE == pinstate) 
        || (0 == (pinstate & BFG_PINDISABLE) && (ENUM_INFO_UART == type))
        || (0 == (pinstate & WLAN_PINDISABLE) && (ENUM_INFO_SDIO == type)))
        {
            PS_PRINT_INFO("pinstart bfg_pindisable\n");
            return SUCC;
        }
        fp = patch_readm_fileopen(type);
        if (IS_ERR(fp))
        {
/*
            PS_PRINT_ERR("create file error,fp = 0x%p, errorno=%d\n", fp, PTR_ERR(fp));
*/
            return SUCC;
        }
        l_ret = patch_int_para_send(Key, Value, type);
        if (0 > l_ret)
        {
            PS_PRINT_ERR("send %s,%s fail \n", Key, Value);
            filp_close(fp, NULL);
            return l_ret;
        }
        PS_PRINT_DBG("recv mem form device\n");
        flag = OS_STR_CHR(Value, ',');
        if (NULL == flag)
        {
            PS_PRINT_ERR("RECV LEN ERROR..\n");
            filp_close(fp, NULL);
            return -EFAIL;
        }
        flag++;
        PS_PRINT_DBG("recv len [%s]\n", flag);
        while(COMPART_KEYWORD == *flag)
        {
            flag++;
        }
        num = patch_string_to_num(flag);

        PS_PRINT_DBG("recv len [%d]\n", num);

        l_ret = patch_recv_mem(fp, num, type);
    }
    return l_ret;
}

/*****************************************************************************
 Prototype    : patch_quit_type
 Description  : down quit command
 Input        : uint8 *Key
                uint8 * Value
 Output       :
 Return Value : int32
 Calls        :
 Called By    :

  History        :
  1.Date         : 2012/11/1
    Author       : kf74033
    Modification : Created function

*****************************************************************************/
int32 patch_quit_type(int32 type)
{
    int32   l_ret;
    int32   l_len;
    uint8   buf[8];
    PS_PRINT_DBG("entry\n");

    OS_MEM_SET(buf, 0, 8);

    OS_MEM_CPY(buf, (uint8 *)QUIT_CMD, OS_STR_LEN(QUIT_CMD));
    l_len = OS_STR_LEN(QUIT_CMD);

    buf[l_len] = COMPART_KEYWORD;
    l_len++;

    if (ENUM_INFO_UART == type)
    {
        ps_patch_to_nomal();
    }

    l_ret = send_msg_t(buf, l_len, type);

    return l_ret;
}

/*****************************************************************************
 Prototype    : patch_file_type
 Description  : down addr and file
 Input        : uint8 *Key
                uint8 * Value
 Output       :
 Return Value : int32
 Calls        :
 Called By    :

  History        :
  1.Date         : 2012/11/1
    Author       : kf74033
    Modification : Created function

*****************************************************************************/
int32 patch_file_type(uint8 *Key, uint8 *Value, int32 type)
{
    int32       i;
    int32       n;
    int32       l_ret;
    int32       l_len;
    uint8       auc_addr[INT32_STR_LEN];
    uint8       data[DATA_BUF_LEN];
    int32       data_len;
    uint8 *tmp;
    uint8 *tmp1;

    PS_PRINT_DBG("Key = %s, Value = %s\n", Key, Value);

    /*
     * 根据关键字的最后一个字符，确定发送地址之后，device的返回值
     * 所以配置文件的关键字不能随意修改
     */
    OS_MEM_SET(data, 0, DATA_BUF_LEN);
    data_len = OS_STR_LEN(Key);
    OS_MEM_CPY(data, Key, data_len);

    data[data_len] = COMPART_KEYWORD;
    data_len++;

    /*兼容wifibootloader配置设置*/
    tmp1 = Value;
    while( ' ' == *tmp1)
    {
        tmp1++;
    }
    if ('1' != *tmp1)
    {
        PS_PRINT_ERR("para [%s] not begin with 1", tmp1);
        return -EFAIL;
    }
    tmp = OS_STR_CHR(tmp1,',');
    if (NULL == tmp)
    {
        PS_PRINT_ERR("has no ',' string:[%s]",tmp);
        return -EFAIL;
    }
    tmp++;
    PS_PRINT_DBG("tmp is %s\n", tmp);

    OS_MEM_SET(auc_addr, 0, INT32_STR_LEN);
    for (i = 0,n = 0; tmp[i] != ',' && n < INT32_STR_LEN; i++)
    {
        if ((',' == tmp[i]) || (COMPART_KEYWORD == tmp[i]))
        {
            break;
        }
        else
        {
            auc_addr[n] = tmp[i];
            n++;
        }
    }
    OS_MEM_CPY((uint8 *)&data[data_len], auc_addr, n);
    data_len = data_len + n;

    data[data_len] = COMPART_KEYWORD;
    data_len++;

    PS_PRINT_DBG("data is %s\n", data);

    l_ret = patch_send_data(data, data_len, MSG_FORM_DRV_C, type);
    if (0 > l_ret)
    {
        PS_PRINT_ERR(" SEND %s addr error\n", Key);

        return -EFAIL;
    }

    /* 删除头部的空格 */
    l_len = OS_STR_LEN((int8 *)tmp);
    for(i = i + 1; i < l_len; i++)
    {
        if (('/' == tmp[i]) || ('.' == tmp[i]))  /* 兼容绝对路径和相对路径 */
        {
            break;
        }
    }

    PS_PRINT_DBG("file path is %s\n", &tmp[i]);

    l_ret = patch_down_file (&tmp[i], type);
    PS_PRINT_DBG("patch_down_file:%d", l_ret);
    if (0 > l_ret)
    {
        PS_PRINT_ERR(" SEND %s file error\n", Key);

        return l_ret;
    }
    /* G 是 DEVICE 完成相应操作标志 */
    l_ret = patch_wait_g_form_dev(type);

    return l_ret;

}
/*****************************************************************************
 Prototype    : patch_device_respond
 Description  : wait respond form device
 Input        : void

 Output       :
 Return Value : int32
 Calls        :
 Called By    :

  History        :
  1.Date         : 2012/11/1
    Author       : kf74033
    Modification : Created function

*****************************************************************************/
int32 patch_device_respond(int32 type)
{
    int32   l_ret;
    int32   i;

    PS_PRINT_DBG("entry");
    for(i = 0; i < HOST_DEV_TIMEOUT; i++)
    {
        if (ENUM_INFO_SDIO == type)
        {
//            PATCH_SEND_N_SDIO;
        }
        else
        {
            PATCH_SEND_N_UART;
        }
        OS_MEM_SET(g_st_global[type].auc_DevVersion, 0, VERSION_LEN);
        msleep(1);
        l_ret = read_msg_t(g_st_global[type].auc_DevVersion, VERSION_LEN, type);
        if (0 > l_ret)
        {
            PS_PRINT_ERR("read fail![%d]\n", i);
            continue;
        }
        else if (!OS_MEM_CMP((int8 *)g_st_global[type].auc_DevVersion,
                              (int8 *)g_st_global[type].auc_CfgVersion,
                              OS_STR_LEN(g_st_global[type].auc_CfgVersion)))
        {

            if (ENUM_INFO_SDIO == type)
            {
                PATCH_SEND_A_SDIO;
                msleep(10); 
            }
            else
            {
                PATCH_SEND_A_UART;
            }
            PS_PRINT_INFO("Device Version = [%s], CfgVersion = [%s].\n",
                        g_st_global[type].auc_DevVersion, g_st_global[type].auc_CfgVersion);
            return SUCC;
        }
        else
        {
            if (ENUM_INFO_SDIO == type)
            {
//                PATCH_SEND_N_SDIO;
            }
            else
            {
                PATCH_SEND_N_UART;
            }

            PS_PRINT_INFO("Device Version = [%s], CfgVersion = [%s].\n",
                        g_st_global[type].auc_DevVersion, g_st_global[type].auc_CfgVersion);
        }
    }

    PS_PRINT_ERR("read device version fail![%d]\n", i);

    return -EFAIL;
}

/*****************************************************************************
 Prototype    : patch_del_space
 Description  : delete space
 Input        : uint8 *string
                int32 *len

 Output       :
 Return Value : uint8 *
 Calls        :
 Called By    :

  History        :
  1.Date         : 2012/10/19
    Author       : kf74033
    Modification : Created function

*****************************************************************************/
uint8 *patch_del_space(uint8 *string, int32 *len)
{
    int i;
    PS_PRINT_DBG("entry");
    if ((NULL == string) || (NULL == len))
    {
        return NULL;
    }

    /* 删除尾部的空格 */
    for(i = *len - 1; i >= 0; i--)
    {
        if (COMPART_KEYWORD != string[i])
        {
            break;
        }
        string[i] = '\0';
    }
    /* 出错 */
    if (i < 0)
    {
        PS_PRINT_ERR(" string is Space bar\n");
        return NULL;
    }
    /* 在for语句中减去1，这里加上1 */
    *len = i + 1;

    /* 删除头部的空格 */
    for(i = 0; i < *len; i++)
    {
        if (' ' != string[i])
        {
            /* 减去空格的个数 */
            *len = *len - i;
            return &string[i];
        }
    }

    return NULL;
}

/*****************************************************************************
 Prototype    : patch_read_cfg
 Description  : read config file
 Input        : uint8 *cfg_path
                uint8 *read_buf

 Output       :
 Return Value : int32
 Calls        :
 Called By    :

  History        :
  1.Date         : 2012/11/1
    Author       : kf74033
    Modification : Created function

*****************************************************************************/
int32 patch_read_cfg(uint8 *cfg_path, uint8 *read_buf)
{
    OS_KERNEL_FILE_STRU    *fp;
    int32                   l_ret;

    PS_PRINT_DBG("entry\n");
    if ((NULL == cfg_path) || (NULL == read_buf))
    {
        PS_PRINT_ERR("para is NULL\n");
        return -EFAIL;
    }

    fp = filp_open(cfg_path, O_RDONLY, 0);
    if (IS_ERR(fp))
    {
        fp = NULL;
        PS_PRINT_ERR("filp_open %s fail\n", cfg_path);

        return -EFAIL;
    }

    OS_MEM_SET(read_buf, 0, READ_CFG_BUF_LEN);
    l_ret = kernel_read(fp, fp->f_pos, read_buf, READ_CFG_BUF_LEN);

    filp_close(fp, NULL);
    fp = NULL;

    return l_ret;

}

/*****************************************************************************
 Prototype    : patch_parse_cmd
 Description  : parse config command
 Input        : uint8 *buf
                uint8 *cmd_name
                uint8 *cmd_para
 Output       :
 Return Value : int32
 Calls        :
 Called By    :

  History        :
  1.Date         : 2012/11/1
    Author       : kf74033
    Modification : Created function

*****************************************************************************/
int32 patch_parse_cmd(uint8 *buf, uint8 *cmd_name, uint8 *cmd_para)
{
    int32       l_ret;
    int32       l_cmdlen;
    int32       l_paralen;
    uint8      *begin;
    uint8      *end;
    uint8      *link;
    uint8      *handle;

#ifdef HISI_GET_BOARD_ID
    int32       l_offset = 0;
#endif

    begin = buf;
    if((NULL == buf) || (NULL == cmd_name) || (NULL == cmd_para))
    {
        PS_PRINT_ERR("buf/cmd_name/cmd_para is NULL\n");
        return ERROR_TYPE_CMD;
    }

    /* 注释行 */
    if ('@' == buf[0])
    {
        return ERROR_TYPE_CMD;
    }

    /* 错误行，或者退出命令行 */
    link = OS_STR_CHR((int8 *)begin, '=');
    if (NULL == link)
    {
        /* 退出命令行 */
        if (NULL != OS_STR_STR((int8 *)buf, QUIT_CMD_KEYWORD))
        {
            return QUIT_TYPE_CMD;
        }

        return ERROR_TYPE_CMD;
    }

    /* 错误行，没有结束符 */
    end = OS_STR_CHR(link, ';');
    if (NULL == end)
    {
        return ERROR_TYPE_CMD;
    }

    l_cmdlen = link - begin;

    /* 删除关键字的两边空格 */
    handle = patch_del_space((uint8 *)begin, &l_cmdlen);
    if (NULL == handle)
    {
        return ERROR_TYPE_CMD;
    }

    /* 判断命令类型 */
    if (!OS_MEM_CMP(handle, (uint8 *)FILE_TYPE_CMD_KEY, OS_STR_LEN((uint8 *)FILE_TYPE_CMD_KEY)))
    {
        handle = OS_STR_STR(handle, (uint8 *)FILE_TYPE_CMD_KEY) + OS_STR_LEN(FILE_TYPE_CMD_KEY);
        l_cmdlen = l_cmdlen - OS_STR_LEN(FILE_TYPE_CMD_KEY);
        l_ret = FILE_TYPE_CMD;
    }
    else if (!OS_MEM_CMP(handle, (uint8 *)NUM_TYPE_CMD_KEY, OS_STR_LEN(NUM_TYPE_CMD_KEY)))
    {
        handle = OS_STR_STR(handle, (uint8 *)NUM_TYPE_CMD_KEY) + OS_STR_LEN(NUM_TYPE_CMD_KEY);
        l_cmdlen = l_cmdlen - OS_STR_LEN(NUM_TYPE_CMD_KEY);
        l_ret = NUM_TYPE_CMD;
    }
    else
    {
        return ERROR_TYPE_CMD;
    }
    OS_MEM_CPY(cmd_name, handle, l_cmdlen);

    /* 删除值两边空格 */
    begin = link + 1;
    l_paralen = end - begin;

    handle = patch_del_space((uint8 *)begin, &l_paralen);
    if (NULL == handle)
    {
        return ERROR_TYPE_CMD;
    }
    OS_MEM_CPY(cmd_para, handle, l_paralen);
#ifdef HISI_GET_BOARD_ID
    if (!OS_MEM_CMP(buf, (uint8 *)FILE_TYPE_RW_KEY, OS_STR_LEN((uint8 *)FILE_TYPE_RW_KEY))
            && !OS_STR_STR(buf, UART_KEY))
    {
        // dts get rw file name failed
        l_paralen = strlen(g_st_boardid.rw_file_name);
        if (0 == l_paralen)
        {
            PS_PRINT_WARNING("Get the dts rw file name failed\n");
            return l_ret;
        }

        PS_PRINT_INFO("cmd_para:{%s}\n", cmd_para);
        end = OS_STR_CHR((int8 *)cmd_para, ',');
        if (NULL == end)
        {
            return l_ret;
        }

        l_offset = (uint8 *)end - cmd_para + 1;
        // out of range
        if ((l_offset + l_paralen + 1) > PARA_LEN)
        {
            return l_ret;
        }

        snprintf(cmd_para + l_offset, l_paralen + 1, "%s", g_st_boardid.rw_file_name);
        PS_PRINT_INFO("cmd_para:{%s}\n", cmd_para);
    }
#endif
    return l_ret;

}

/*****************************************************************************
 Prototype    : patch_parse_cfg
 Description  : parse config file
 Input        : uint8 *buf
                int32 len
                int32 type
 Output       :
 Return Value : int32
 Calls        :
 Called By    :

  History        :
  1.Date         : 2012/11/1
    Author       : kf74033
    Modification : Created function

*****************************************************************************/
void *patch_malloc_cmd_buf(uint8 *buf, int32 type)
{
    int32           l_len;
    uint8          *flag;
    uint8          *p_buf;

    /* 统计命令个数 */
    flag    = buf;
    g_st_global[type].l_count = 0;
    while(NULL != flag)
    {
        /* 一个正确的命令行结束符为 ; */
        flag = OS_STR_CHR(flag, CMD_LINE_SIGN);
        if (NULL == flag)
        {
            break;
        }
        g_st_global[type].l_count++;
        flag++;
    }
    PS_PRINT_DBG("l_count = %d\n", g_st_global[type].l_count);

    /* 申请存储命令空间 */
    /* 比实际多分一个命令空间。这样的防止配置文件最后一个命令不是退出命令 */
    l_len = (g_st_global[type].l_count + 1) * sizeof(struct cmd_type_st);
    p_buf = OS_KMALLOC_GFP(l_len);
    if (NULL == p_buf)
    {
        PS_PRINT_ERR("kmalloc cmd_type_st fail\n");
        return NULL;
    }
    OS_MEM_SET((void *)p_buf, 0, l_len);

    return p_buf;
}

/*****************************************************************************
 Prototype    : patch_parse_cfg
 Description  : parse config file
 Input        : uint8 *buf
                int32 len
                int32 type
 Output       :
 Return Value : int32
 Calls        :
 Called By    :

  History        :
  1.Date         : 2012/11/1
    Author       : kf74033
    Modification : Created function

*****************************************************************************/
int32 patch_parse_cfg(uint8 *buf, int32 buf_len, int32 type)
{
    int32           i;
    int32           l_len;
    uint8          *flag;
    uint8          *begin;
    uint8          *end;
    int32           cmd_type;
    uint8           cmd_name[CMD_LEN];
    uint8           cmd_para[PARA_LEN];

    g_st_global[type].pst_cmd = (struct cmd_type_st *)patch_malloc_cmd_buf(buf, type);
    if (NULL == g_st_global[type].pst_cmd)
    {
        PS_PRINT_ERR(" patch_malloc_cmd_buf fail\n");
        return -EFAIL;
    }

    /* 解析CMD BUF*/
    flag = buf;
    /* 申请的存储命令空间够用，buf没有读取完成 */
    // l_len = buf_len - 1;
    l_len = buf_len;
    i = 0;
    while((i < g_st_global[type].l_count) && (flag < &buf[l_len]))
    {
        /*
         *获取配置文件中的一行,配置文件必须是unix格式.
         *配置文件中的某一行含有字符 @ 则认为该行为注释行
         */
        begin = flag;
        end   = OS_STR_CHR(flag, '\n');
        if(NULL == end)
        {
            PS_PRINT_INFO("end is null\n");
            break;
        }
        if (end == begin)           /* 该行只有一个换行符 */
        {
            PS_PRINT_DBG("blank line\n");
            flag = end + 1;
            continue;
        }
        *end = '\0';
        PS_PRINT_DBG("operation string is [%s]\n", begin);
        OS_MEM_SET(cmd_name, 0, CMD_LEN);
        OS_MEM_SET(cmd_para, 0, PARA_LEN);

        cmd_type = patch_parse_cmd(begin,cmd_name, cmd_para);
        if (ERROR_TYPE_CMD != cmd_type)/* 正确的命令类型，增加 */
        {
//            PS_PRINT_ERR("cmd[%d]type[%d], name[%s], para[%s]\n",
//                        i, cmd_type, cmd_name, cmd_para);
            g_st_global[type].pst_cmd[i].cmd_type = cmd_type;
            OS_MEM_CPY(g_st_global[type].pst_cmd[i].cmd_name, cmd_name, CMD_LEN);
            OS_MEM_CPY(g_st_global[type].pst_cmd[i].cmd_para, cmd_para, PARA_LEN);
            /* 获取配置版本号 */
            if (!OS_MEM_CMP(g_st_global[type].pst_cmd[i].cmd_name,
                            VER_CMD_KEYWORD,
                            OS_STR_LEN(VER_CMD_KEYWORD)))
            {
                OS_MEM_CPY(g_st_global[type].auc_CfgVersion,
                           g_st_global[type].pst_cmd[i].cmd_para,
                           //OS_STR_LEN(g_st_global[type].pst_cmd[i].cmd_para));
							VERSION_LEN);


                PS_PRINT_DBG("g_CfgVersion = [%s],[%s], \n",
                                       g_st_global[type].auc_CfgVersion, g_st_global[type].pst_cmd[i].cmd_para);
            }
            i++;
            PS_PRINT_DBG("cmd_name:%s,cmd_para:%s",cmd_name,cmd_para);
        }
        flag = end + 1;
    }
    PS_PRINT_DBG("Read cmd OK\n");

    /* 如果最后一个命令不是退出命令，则增加一个退出命令 */
    if (QUIT_TYPE_CMD != g_st_global[type].pst_cmd[i-1].cmd_type)
    {
        g_st_global[type].pst_cmd[i].cmd_type = QUIT_TYPE_CMD;
        i++;            /* 方便统计命令个数 */
    }

    /* 根据实际命令个数，修改最终的命令个数 */
    g_st_global[type].l_count = i;
    PS_PRINT_DBG("type[%d], cmd count[%d]\n", type, g_st_global[type].l_count);

    return SUCC;
}

/*****************************************************************************
 Prototype    : patch_get_cfg
 Description  : get patch config command
 Input        : uint8 *cfg
                int32 type

 Output       :
 Return Value : int32
 Calls        :
 Called By    :

  History        :
  1.Date         : 2012/11/1
    Author       : kf74033
    Modification : Created function

*****************************************************************************/
int32 patch_get_cfg(uint8 *cfg, int32 type)
{
    uint8 *buf;
    int32   l_readlen;
    int32   l_ret;

    buf = OS_KMALLOC_GFP(READ_CFG_BUF_LEN);
    if (NULL == buf)
    {
        return -EFAIL;
    }

    /* 配置文件必须小于1024 */
    l_readlen = patch_read_cfg(cfg, buf);
    if(0 > l_readlen)
    {
        PS_PRINT_ERR("read cfg error\n");
        OS_MEM_KFREE(buf);
        buf = NULL;
        return -EFAIL;
    }

    l_ret = patch_parse_cfg(buf, l_readlen, type);
    if(0 > l_ret)
    {
        PS_PRINT_ERR("parse cfg error\n");
    }

    OS_MEM_KFREE(buf);
    buf = NULL;
    return l_ret;


}
EXPORT_SYMBOL(patch_get_cfg);
/*****************************************************************************
 Prototype    : patch_download_info
 Description  : download patch
 Input        : int32 type
 Output       :

 Return Value : int32
 Calls        :
 Called By    :

 History         :
  1.Date         : 2012/11/1
    Author       : kf74033
    Modification : Created function

*****************************************************************************/
int32 patch_execute_cmd(int32 cmd_type, uint8 *cmd_name, uint8 *cmd_para,int32 type)
{
    int32 l_ret;

    /* 清空上次操作遗留下来的数据，读取结果时以长度为判断，buf就不用清空了 */
    g_st_global[type].l_Recvbuf1_len = 0;
    g_st_global[type].l_Recvbuf2_len = 0;

    switch(cmd_type)
    {
         case FILE_TYPE_CMD:
            PS_PRINT_DBG(" command type FILE_TYPE_CMD\n");
            l_ret = patch_file_type(cmd_name, cmd_para, type);
            break;
         case NUM_TYPE_CMD:
            PS_PRINT_DBG(" command type NUM_TYPE_CMD\n");
            l_ret = patch_number_type(cmd_name, cmd_para, type);

            break;
         case QUIT_TYPE_CMD:
            PS_PRINT_DBG(" command type QUIT_TYPE_CMD\n");
            l_ret = patch_quit_type(type);
            break;

         default:
            PS_PRINT_ERR("command type error[%d]\n", cmd_type);
            l_ret = -EFAIL;
            break;
    }

    return l_ret;
}

/*****************************************************************************
 Prototype    : patch_download_info
 Description  : download patch
 Input        : int32 type
 Output       :

 Return Value : int32
 Calls        :
 Called By    :

 History         :
  1.Date         : 2012/11/1
    Author       : kf74033
    Modification : Created function

*****************************************************************************/
int32 patch_download_export(uint8 *keyword, int32 type)
{
    int32       i;
    int32       l_ret;


    for (i = 0; i < g_st_global[type].l_count; i++)
    {
        if (!OS_STR_CMP(g_st_global[type].pst_cmd[i].cmd_name,
                         keyword))
                         //OS_STR_LEN(keyword)))
        {
            break;
        }
    }
    if (i >= g_st_global[type].l_count)
    {
        return -EFAIL;
    }
    l_ret = patch_execute_cmd(g_st_global[type].pst_cmd[i].cmd_type,
                      g_st_global[type].pst_cmd[i].cmd_name,
                      g_st_global[type].pst_cmd[i].cmd_para,
                      type);
    return l_ret;

}
EXPORT_SYMBOL(patch_download_export);

int32 patch_check_version(int32 type)
{
    int32             l_ret;
    int32             i = 0;
    int8  version_buff[VERSION_LEN] = {0};
    snprintf(version_buff, VERSION_LEN,"%s%c",VER_CMD_KEYWORD,COMPART_KEYWORD);
    PS_PRINT_DBG("check version cmd:%s", version_buff);
    for(i = 0; i < HOST_DEV_TIMEOUT; i++)
    {
        l_ret = send_msg_t(version_buff, VERSION_LEN, type);
        if (0 > l_ret)
        {
            continue;
            //return -EFAIL;
        }

        l_ret = patch_device_respond(type);
        if (0 == l_ret)
        {
            return SUCC;
        }
    }
    return l_ret;
}
/*****************************************************************************
 Prototype    : patch_download_patch
 Description  : download patch
 Input        : int32 type
 Output       :

 Return Value : int32
 Calls        :
 Called By    :

 History         :
  1.Date         : 2012/11/1
    Author       : kf74033
    Modification : Created function

*****************************************************************************/
int32 patch_download_patch(int32 type)
{
    int32                   l_ret;
    int32                   i;
    uint32                  ul_alloc_len = READ_DATA_BUF_LEN;
//    g_debuglevel_patch =12;
/*
    dma_addr_t              puc_phyaddr;
*/
    PS_PRINT_INFO("type:%d\n", type);
    if (ENUM_INFO_UART == type)
    {
        g_stringbuf.pbufstart = kmalloc(ul_alloc_len, GFP_KERNEL);
        if (NULL == g_stringbuf.pbufstart)
        {
            ul_alloc_len = READ_DATA_REALLOC_BUF_LEN;
            g_stringbuf.pbufstart = kmalloc(ul_alloc_len, GFP_KERNEL);
            if (NULL == g_stringbuf.pbufstart)
            {
                g_usemalloc = 0;
                PS_PRINT_ERR("ringbuf KMALLOC SIZE(%d) failed.\n", ul_alloc_len);
                g_stringbuf.pbufstart = g_st_global[type].auc_Recvbuf1;
                g_stringbuf.pbufend = RECV_BUF_LEN + g_stringbuf.pbufstart;

                return -EFAIL;
            }

            powerpin_state &= 0xFF - BFG_PINDISABLE;
            PS_PRINT_INFO("ringbuf kmalloc size(%d) suc.\n", ul_alloc_len);
        }

        g_stringbuf.pbufend = ul_alloc_len + g_stringbuf.pbufstart;
        g_usemalloc = 1;

        g_stringbuf.phead = g_stringbuf.pbufstart;
        g_stringbuf.ptail = g_stringbuf.pbufstart;
    }


    l_ret = patch_check_version(type);
    if (0 > l_ret)
    {
        PS_PRINT_ERR("checkout device version error");
        if ((ENUM_INFO_UART == type) && (1 == g_usemalloc))
        {
            //dma_free_coherent(NULL, READ_DATA_BUF_LEN, g_stringbuf.pbufstart, puc_phyaddr);
            kfree(g_stringbuf.pbufstart);
            g_stringbuf.pbufstart   = NULL;
            g_stringbuf.pbufend     = NULL;
            g_stringbuf.phead       = NULL;
            g_stringbuf.ptail       = NULL;
        }
        return -EFAIL;

    }

    /* 执行条件:: 命令行没有读完，命令不是错误命令  */
    for (i = 0; i < g_st_global[type].l_count; i++)
    {
        PS_PRINT_DBG("cmd[%d]type[%d], name[%s], para[%s]\n",
                    i,
                    g_st_global[type].pst_cmd[i].cmd_type,
                    g_st_global[type].pst_cmd[i].cmd_name,
                    g_st_global[type].pst_cmd[i].cmd_para);

        l_ret = patch_execute_cmd(g_st_global[type].pst_cmd[i].cmd_type,
                                  g_st_global[type].pst_cmd[i].cmd_name,
                                  g_st_global[type].pst_cmd[i].cmd_para,
                                  type);
        if (0 > l_ret)
        {
            if ((ENUM_INFO_UART == type) && (1 == g_usemalloc))
            {
                //dma_free_coherent(NULL, READ_DATA_BUF_LEN, g_stringbuf.pbufstart, puc_phyaddr);
                kfree(g_stringbuf.pbufstart);
                g_stringbuf.pbufstart   = NULL;
                g_stringbuf.pbufend     = NULL;
                g_stringbuf.phead       = NULL;
                g_stringbuf.ptail       = NULL;
                g_usemalloc = 0;
            }
            if (ENUM_INFO_SDIO == type)
            {
                powerpin_state &= 0xFF - WLAN_PINDISABLE;
            }
            else if (ENUM_INFO_UART == type)
            {
                powerpin_state &= 0xFF - BFG_PINDISABLE;
            }
            return l_ret;
        }
    }
    if (ENUM_INFO_SDIO == type)
    {
        powerpin_state &= 0xFF - WLAN_PINDISABLE;
    }
    else if (ENUM_INFO_UART == type)
    {
        powerpin_state &= 0xFF - BFG_PINDISABLE;
    }
    PS_PRINT_DBG("DOWN PATCH SUCC\n");

    if ((ENUM_INFO_UART == type) && (1 == g_usemalloc))
    {
        //dma_free_coherent(NULL, READ_DATA_BUF_LEN, g_stringbuf.pbufstart, puc_phyaddr);
        kfree(g_stringbuf.pbufstart);
        g_usemalloc = 0;
    }
    g_stringbuf.pbufstart   = NULL;
    g_stringbuf.pbufend     = NULL;
    g_stringbuf.phead       = NULL;
    g_stringbuf.ptail       = NULL;
    return SUCC;
}
EXPORT_SYMBOL(patch_download_patch);
/*****************************************************************************
 Prototype    : create_recover_file
 Description  : create recover flag file at /data/hwlogdir
 Input        :
 Output       :
 Return Value : int32. if file create succ or is exist return 0,
              : else return 1;
 Calls        :
 Called By    :

 History         :
  1.Date         : 2015/07/17
    Author       :
    Modification : Created function

*****************************************************************************/
int32 create_recover_flag(void)
{
    struct file *fp;

    if(is_file_exist(SDIO_RECOVER_FLAG_FILE))
    {
        PS_PRINT_DBG("file %s is already exist\n", SDIO_RECOVER_FLAG_FILE);
        return 0;
    }
    else
    {
        fp = filp_open(SDIO_RECOVER_FLAG_FILE, O_RDWR | O_CREAT, 0664);
        if(IS_ERR(fp))
        {
/*
            PS_PRINT_ERR("create/open file %s failed, fd:%ld\n", SDIO_RECOVER_FLAG_FILE, fp);
*/
            return 1;
        }
        filp_close(fp, NULL);
    }
    PS_PRINT_DBG("create/open file %s succ\n", SDIO_RECOVER_FLAG_FILE);
    return 0;
}
EXPORT_SYMBOL(create_recover_flag);

/*****************************************************************************
 Prototype    : is_file_exist
 Description  : judge if file is exist in filesystem.
 Input        :
 Output       :
 Return Value : int32. if file is exist return 1, else return 0;
 Calls        :
 Called By    :

 History         :
  1.Date         : 2015/07/17
    Author       :
    Modification : Created function

*****************************************************************************/
int32 is_file_exist(char * filename)
{
    int ret;
    mm_segment_t old_fs = get_fs();
    struct kstat kst;

    set_fs(KERNEL_DS);
    if(vfs_stat(filename, &kst))
    {
        PS_PRINT_WARNING("file %s not exist\n", filename);
        ret = 0;
    }
    else
    {
        PS_PRINT_DBG("file %s is exist\n", filename);
        ret = 1;
    }
    set_fs(old_fs);
    return ret;
}

/*****************************************************************************
 Prototype    : patch_init
 Description  : patch module initialization
 Input        :
 Output       :
 Return Value : int32
 Calls        :
 Called By    :

 History         :
  1.Date         : 2012/11/14
    Author       : kf74033
    Modification : Created function

*****************************************************************************/
int32 patch_init(int32 type)
#if 1
{
    int32 l_ret;

    PS_PRINT_INFO("entry\n");

  //  OS_MEM_SET(g_st_global, 0, ENUM_INFO_TOTAL * sizeof(PATCH_GLOBALS_STUR));

   if (ENUM_INFO_SDIO == type)
   {
        if(is_file_exist(SDIO_RECOVER_FLAG_FILE) && is_file_exist(SDIO_RECOVER_CFG_FILE))
        {
            PS_PRINT_WARNING("use recover_cfg file\n");
            OS_MEM_CPY(g_st_global[ENUM_INFO_SDIO].auc_Cfgpath, SDIO_RECOVER_CFG_FILE, OS_STR_LEN(SDIO_RECOVER_CFG_FILE));
        }
        else
        {
            PS_PRINT_WARNING("use normal cfg_file\n");
            OS_MEM_CPY(g_st_global[ENUM_INFO_SDIO].auc_Cfgpath, SDIO_CFG_FILE, OS_STR_LEN(SDIO_CFG_FILE));
        }
   }
   else
   {
        OS_MEM_CPY(g_st_global[ENUM_INFO_UART].auc_Cfgpath, UART_CFG_FILE, OS_STR_LEN(UART_CFG_FILE));
   }

    g_st_global[type].pst_wait = OS_KMALLOC_GFP(sizeof(OS_WAIT_QUEUE_HEAD_T_STRU));
    if (NULL == g_st_global[type].pst_wait)
    {
//      patch_exit();
        return -EFAIL;
    }
    OS_INIT_WAITQUEUE_HEAD(g_st_global[type].pst_wait);

    l_ret = patch_get_cfg(g_st_global[type].auc_Cfgpath, type);
    if(0 > l_ret)
    {
//      patch_exit();
        PS_PRINT_ERR("get [%s] command is fail\n", g_st_global[type].auc_Cfgpath);
        return -EFAIL;
    }

    if(NULL == g_pucDataBuf_t)
    {
        g_pucDataBuf_t = OS_KMALLOC_GFP(READ_PATCH_BUF_LEN);
        if(NULL == g_pucDataBuf_t)
        {
            PS_PRINT_ERR("g_pucDataBuf_etc KMALLOC failed");
            g_pucDataBuf_t = NULL;
            return -EFAIL;
        } else {
            PS_PRINT_DBG("g_pucDataBuf_etc KMALLOC succ");
        }
    }
    return SUCC;

}
#else
{
    int32 l_ret;
    int32 i;

    PS_PRINT_DBG("entry\n");

  //  OS_MEM_SET(g_st_global, 0, ENUM_INFO_TOTAL * sizeof(PATCH_GLOBALS_STUR));

    OS_MEM_CPY(g_st_global[ENUM_INFO_SDIO].auc_Cfgpath, SDIO_CFG_FILE, OS_STR_LEN(SDIO_CFG_FILE));
    OS_MEM_CPY(g_st_global[ENUM_INFO_UART].auc_Cfgpath, UART_CFG_FILE, OS_STR_LEN(UART_CFG_FILE));

    for (i = 0; i < ENUM_INFO_TOTAL; i++)
    {
        g_st_global[i].pst_wait = OS_KMALLOC_GFP(sizeof(OS_WAIT_QUEUE_HEAD_T_STRU));
        if (NULL == g_st_global[i].pst_wait)
        {
            patch_exit();
            return -EFAIL;
        }
        OS_INIT_WAITQUEUE_HEAD(g_st_global[i].pst_wait);

        l_ret = patch_get_cfg(g_st_global[i].auc_Cfgpath, i);
        if(0 > l_ret)
        {
            patch_exit();
            PS_PRINT_ERR("get [%s] command is fail\n", g_st_global[i].auc_Cfgpath);
            return -EFAIL;
        }
    }

    return SUCC;

}
#endif
EXPORT_SYMBOL(patch_init);

/*****************************************************************************
 Prototype    : patch_exit
 Description  : patch module exit
 Input        :
 Output       :
 Return Value : int32
 Calls        :
 Called By    :

 History         :
  1.Date         : 2012/11/14
    Author       : kf74033
    Modification : Created function

*****************************************************************************/
int32 patch_exit(void)
{
    int32 i;
    for (i = 0; i < ENUM_INFO_TOTAL; i++)
    {
        g_st_global[i].l_count = 0;
        if (NULL != g_st_global[i].pst_cmd)
        {
            OS_MEM_KFREE(g_st_global[i].pst_cmd);
            g_st_global[i].pst_cmd = NULL;
        }

        if (NULL != g_st_global[i].pst_wait)
        {
            OS_MEM_KFREE(g_st_global[i].pst_wait);
            g_st_global[i].pst_wait = NULL;
        }
    }

	if(NULL != g_pucDataBuf_t)
	{
		OS_MEM_KFREE(g_pucDataBuf_t);
		g_pucDataBuf_t = NULL;
	}

    return SUCC;
}
EXPORT_SYMBOL(patch_exit);


