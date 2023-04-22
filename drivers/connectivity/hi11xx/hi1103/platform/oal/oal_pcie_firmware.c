

#ifdef __cplusplus
    #if __cplusplus
        extern "C" {
    #endif
#endif

#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE

#define  HISI_LOG_TAG "[PCIEF]"
#include "oal_util.h"

#include "oal_net.h"
#include "oal_ext_if.h"

#include "oam_ext_if.h"

#include "oal_pcie_host.h"
#include "oal_pcie_linux.h"
#include "oal_pcie_pm.h"
#include "oal_hcc_host_if.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_OAL_PCIE_FIRMWARE_C

#define STR_WRITEM_OK           ("WRITEM OK:")
#define STR_FILES_READY         ("READY:FILE")
#define STR_FILES_OK            ("FILES OK:")

#define COMPART_CHAR            (' ')

#define FILES_CMD_SEND          (1)
#define FILES_BIN_SEND          (2)

#define FIRMWARE_MAXPARAMNUM    (16)

typedef oal_int32 (*FIRMWARE_CMDSENDFUNC)(oal_pcie_res *pst_pcie_res, oal_uint8* buff, oal_int32 len);
typedef oal_int32 (*FIRMWARE_CMDREADFUNC)(oal_pcie_res *pst_pcie_res, oal_uint8* buff, oal_int32 len, oal_uint32 timeout);

typedef struct FirmwareCmd
{
    oal_int8               acCmdName[15];
    oal_uint8              ucArgs;
    FIRMWARE_CMDSENDFUNC   pCmdSend;
    FIRMWARE_CMDREADFUNC   pCmdRead;
}FIRMWARECMD_STRU;


oal_int32 oal_pcie_firmware_cmd_send_quit(oal_pcie_res *pst_pcie_res, oal_uint8* buff, oal_int32 len);
oal_int32 oal_pcie_firmware_cmd_read_quit(oal_pcie_res *pst_pcie_res, oal_uint8* buff, oal_int32 len, oal_uint32 timeout);

oal_int32 oal_pcie_firmware_cmd_send_files(oal_pcie_res *pst_pcie_res, oal_uint8* buff, oal_int32 len);
oal_int32 oal_pcie_firmware_cmd_read_files(oal_pcie_res *pst_pcie_res, oal_uint8* buff, oal_int32 len, oal_uint32 timeout);

oal_int32 oal_pcie_firmware_cmd_send_version(oal_pcie_res *pst_pcie_res, oal_uint8* buff, oal_int32 len);
oal_int32 oal_pcie_firmware_cmd_read_version(oal_pcie_res *pst_pcie_res, oal_uint8* buff, oal_int32 len, oal_uint32 timeout);

oal_int32 oal_pcie_firmware_cmd_send_writem(oal_pcie_res *pst_pcie_res, oal_uint8* buff, oal_int32 len);
oal_int32 oal_pcie_firmware_cmd_read_writem(oal_pcie_res *pst_pcie_res, oal_uint8* buff, oal_int32 len, oal_uint32 timeout);

oal_int32 oal_pcie_firmware_cmd_send_readm(oal_pcie_res *pst_pcie_res, oal_uint8* buff, oal_int32 len);
oal_int32 oal_pcie_firmware_cmd_read_readm(oal_pcie_res *pst_pcie_res, oal_uint8* buff, oal_int32 len, oal_uint32 timeout);

oal_int32 oal_pcie_firmware_cmd_send_jump(oal_pcie_res *pst_pcie_res, oal_uint8* buff, oal_int32 len);
oal_int32 oal_pcie_firmware_cmd_read_jump(oal_pcie_res *pst_pcie_res, oal_uint8* buff, oal_int32 len, oal_uint32 timeout);


const FIRMWARECMD_STRU g_astFirmwareCmdTable[] =
{
    {
        "QUIT",
        0,
        oal_pcie_firmware_cmd_send_quit,
        oal_pcie_firmware_cmd_read_quit
    },
    {
        "FILES",
        1,
        oal_pcie_firmware_cmd_send_files,
        oal_pcie_firmware_cmd_read_files
    },
    {
        "VERSION",
        0,
        oal_pcie_firmware_cmd_send_version,
        oal_pcie_firmware_cmd_read_version
    },
    {
        "WRITEM",
        3,
        oal_pcie_firmware_cmd_send_writem,
        oal_pcie_firmware_cmd_read_writem
    },
    {
        "READM",
        2,
        oal_pcie_firmware_cmd_send_readm,
        oal_pcie_firmware_cmd_read_readm
    },
    {
        "JUMP",
        1,
        oal_pcie_firmware_cmd_send_jump,
        oal_pcie_firmware_cmd_read_jump
    },
};

const oal_uint32 g_ulFirmwareCmdNum = sizeof(g_astFirmwareCmdTable) / sizeof(FIRMWARECMD_STRU);

FIRMWARECMD_STRU *g_pstCurrentFirmwareCmd = NULL;
oal_uint32        g_aulParamBuf[FIRMWARE_MAXPARAMNUM] = {0};
oal_uint32        g_ul_files_flag = FILES_CMD_SEND;

oal_int32 oal_pcie_firmware_cmd_send_quit(oal_pcie_res *pst_pcie_res, oal_uint8* buff, oal_int32 len)
{
#define DEV_PCIE_SDIO_SEL_REG   (0x50000204)
#define PCIE_SDIO_SEL_REG_VALUE (0x7070)
    oal_pci_dev_stru *pst_pcie_dev = NULL;
    pci_addr_map      addr_map;
    oal_int32         ret;
    oal_uint16        old;
    oal_uint16        value = PCIE_SDIO_SEL_REG_VALUE;

    pst_pcie_dev = PCIE_RES_TO_DEV(pst_pcie_res);
    if(NULL == pst_pcie_dev)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pcie_dev is null");
        return -OAL_ENODEV;
    }

    PCI_PRINT_LOG(PCI_LOG_DBG, "pcie quit cmd send");

    ret = oal_pcie_inbound_ca_to_va(pst_pcie_res, DEV_PCIE_SDIO_SEL_REG, &addr_map);
    if(OAL_SUCC != ret)
    {
        OAL_IO_PRINT("pcie send quit 0x%8x unmap, failed!\n", DEV_PCIE_SDIO_SEL_REG);
        return -OAL_EFAUL;
    }

#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    oal_pci_cache_inv(pst_pcie_dev, (oal_void*)addr_map.pa, sizeof(value));
#endif
    old = oal_readw((void*)addr_map.va);

    oal_writew(value, (void*)addr_map.va);
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    oal_pci_cache_flush(pst_pcie_dev, (oal_void*)addr_map.pa, sizeof(value));
    oal_pci_cache_inv(pst_pcie_dev, (oal_void*)addr_map.pa, sizeof(value));
#endif
    oal_msleep(10);/*wait tcxo effect*/
    PCI_PRINT_LOG(PCI_LOG_INFO, "pcie quit cmd,  change 0x%8x from 0x%4x to 0x%4x callback-read= 0x%4x\n", \
                      DEV_PCIE_SDIO_SEL_REG, old, value, oal_readw((void*)addr_map.va));

    return OAL_SUCC;
}

oal_int32 oal_pcie_firmware_cmd_read_quit(oal_pcie_res *pst_pcie_res, oal_uint8* buff, oal_int32 len, oal_uint32 timeout)
{
    PCI_PRINT_LOG(PCI_LOG_DBG, "pcie quit cmd read");
    return OAL_SUCC;
}

oal_int32 oal_pcie_firmware_cmd_send_files(oal_pcie_res *pst_pcie_res, oal_uint8* buff, oal_int32 len)
{
    oal_pci_dev_stru *pst_pcie_dev = NULL;
    pci_addr_map      addr_map;
    oal_uint32        cpu_addr;
    oal_int32         ret;

    PCI_PRINT_LOG(PCI_LOG_DBG, "pcie files cmd send");

    pst_pcie_dev = PCIE_RES_TO_DEV(pst_pcie_res);
    if(NULL == pst_pcie_dev)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pcie_dev is null");
        g_ul_files_flag = FILES_CMD_SEND;
        return -OAL_ENODEV;
    }

    if (FILES_CMD_SEND == g_ul_files_flag)
    {
        PCI_PRINT_LOG(PCI_LOG_DBG, "pcie send files cmd");
        return OAL_SUCC;
    }
    else if (FILES_BIN_SEND == g_ul_files_flag)
    {
        PCI_PRINT_LOG(PCI_LOG_DBG, "pcie send files bin");

        if (1 != g_aulParamBuf[0])
        {
            OAL_IO_PRINT("pcie file number must be 1, current is %u\n", g_aulParamBuf[0]);
            g_ul_files_flag = FILES_CMD_SEND;
            return -OAL_EFAUL;
        }

        cpu_addr = g_aulParamBuf[1];
        ret = oal_pcie_inbound_ca_to_va(pst_pcie_res, (cpu_addr + len -1), &addr_map);
        if(OAL_SUCC != ret)
        {
            OAL_IO_PRINT("pcie send file len %u outof memory, addr 0x%8x!\n", len, cpu_addr);
            g_ul_files_flag = FILES_CMD_SEND;
            return -OAL_EFAUL;
        }

        ret = oal_pcie_inbound_ca_to_va(pst_pcie_res, cpu_addr, &addr_map);
        if(OAL_SUCC != ret)
        {
            OAL_IO_PRINT("pcie send file addr 0x%8x unmap!\n", cpu_addr);
            g_ul_files_flag = FILES_CMD_SEND;
            return -OAL_EFAUL;
        }

        oal_pcie_memcopy((oal_ulong)addr_map.va, (oal_ulong)buff, len);
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
        oal_pci_cache_flush(pst_pcie_dev, (oal_void*)addr_map.pa, len);
        oal_pci_cache_inv(pst_pcie_dev, (oal_void*)addr_map.pa, len);
#endif

        return OAL_SUCC;
    }
    else
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pcie files flag %u error", g_ul_files_flag);
        g_ul_files_flag = FILES_CMD_SEND;
        return -OAL_EINVAL;
    }
}

oal_int32 oal_pcie_firmware_cmd_read_files(oal_pcie_res *pst_pcie_res, oal_uint8* buff, oal_int32 len, oal_uint32 timeout)
{
    oal_uint32 size;

    PCI_PRINT_LOG(PCI_LOG_DBG, "pcie files cmd read");

    if (FILES_CMD_SEND == g_ul_files_flag)
    {
        PCI_PRINT_LOG(PCI_LOG_DBG, "pcie read files READY ack");

        size = OAL_MIN(strlen(STR_FILES_READY), len-1);
        strncpy(buff, STR_FILES_READY, size);
        buff[len - 1] = '\0';

        g_ul_files_flag = FILES_BIN_SEND;
        return OAL_SUCC;
    }
    else if (FILES_BIN_SEND == g_ul_files_flag)
    {
        PCI_PRINT_LOG(PCI_LOG_DBG, "pcie read files FILES OK ack");

        size = OAL_MIN(strlen(STR_FILES_OK), len-1);
        strncpy(buff, STR_FILES_OK, size);
        buff[len - 1] = '\0';

        g_ul_files_flag = FILES_CMD_SEND;
        return OAL_SUCC;
    }
    else
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pcie files flag %u error", g_ul_files_flag);

        g_ul_files_flag = FILES_CMD_SEND;

        return -OAL_EINVAL;
    }
}

oal_int32 oal_pcie_firmware_cmd_send_version(oal_pcie_res *pst_pcie_res, oal_uint8* buff, oal_int32 len)
{
    PCI_PRINT_LOG(PCI_LOG_DBG, "pcie version cmd send");
    return OAL_SUCC;
}

oal_int32 oal_pcie_firmware_cmd_read_version(oal_pcie_res *pst_pcie_res, oal_uint8* buff, oal_int32 len, oal_uint32 timeout)
{
#define DEV_VERSION_CPU_ADDR        (0x00002700)
#define DEV_VERSION_CPU_ADDR_PIOLT  (0x00000180)
    oal_pci_dev_stru *pst_pcie_dev = NULL;
    pci_addr_map      addr_map;
    oal_int32         ret;
    oal_uint64        version_address = 0x0;

    pst_pcie_dev = PCIE_RES_TO_DEV(pst_pcie_res);
    if(NULL == pst_pcie_dev)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pcie_dev is null");
        return -OAL_ENODEV;
    }

    if(PCIE_REVISION_5_00A == pst_pcie_res->revision)
    {
        version_address = DEV_VERSION_CPU_ADDR_PIOLT;
    }
    else
    {
        version_address = DEV_VERSION_CPU_ADDR;
    }

    ret = oal_pcie_inbound_ca_to_va(pst_pcie_res, (version_address + len -1), &addr_map);
    if(OAL_SUCC != ret)
    {
        OAL_IO_PRINT("pcie read version len %u outof memory, addr 0x%8llx!\n", len, version_address);
        return -OAL_EFAUL;
    }

    ret = oal_pcie_inbound_ca_to_va(pst_pcie_res, version_address, &addr_map);
    if(OAL_SUCC != ret)
    {
        OAL_IO_PRINT("pcie read version addr 0x%8llx unmap!\n", version_address);
        return -OAL_EFAUL;
    }

#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    oal_pci_cache_inv(pst_pcie_dev, (oal_void*)addr_map.pa, len);
#endif

    oal_pcie_memcopy((oal_ulong)buff, (oal_ulong)addr_map.va, len);

    if(PCI_DBG_CONDTION())
    {
        oal_print_hex_dump((oal_uint8 *)addr_map.va, len, 32, "version: ");
    }

    return OAL_SUCC;
}

oal_int32 oal_pcie_firmware_cmd_send_writem(oal_pcie_res *pst_pcie_res, oal_uint8* buff, oal_int32 len)
{
    oal_pci_dev_stru *pst_pcie_dev = NULL;
    pci_addr_map      addr_map;
    oal_int32         ret;
    oal_uint32        value, cpu_addr, reg_width;

    pst_pcie_dev = PCIE_RES_TO_DEV(pst_pcie_res);
    if(NULL == pst_pcie_dev)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pcie_dev is null");
        return -OAL_ENODEV;
    }

    PCI_PRINT_LOG(PCI_LOG_DBG, "pcie writem cmd send");

    reg_width = g_aulParamBuf[0];
    cpu_addr  = g_aulParamBuf[1];
    value     = g_aulParamBuf[2];

    PCI_PRINT_LOG(PCI_LOG_INFO, "writem:reg width %u, addr 0x%x, value 0x%x", reg_width, cpu_addr, value);

    ret = oal_pcie_inbound_ca_to_va(pst_pcie_res, cpu_addr, &addr_map);
    if(OAL_SUCC != ret)
    {
        OAL_IO_PRINT("pcie send quit 0x%8x unmap, failed!\n", cpu_addr);
        return -OAL_EFAUL;
    }

    switch (reg_width)
    {
        case 1:
            *(oal_uint8 *)addr_map.va  = (oal_uint8)value;
            ret = OAL_SUCC;
            break;
        case 2:
            *(oal_uint16 *)addr_map.va = (oal_uint16)value;
            ret = OAL_SUCC;
            break;
        case 4:
            *(oal_uint32 *)addr_map.va = (oal_uint32)value;
            ret = OAL_SUCC;
            break;
        default:
            PCI_PRINT_LOG(PCI_LOG_ERR, "pcie writem cmd, reg width %u not support", reg_width);
            ret = -OAL_EINVAL;
            break;
    }

#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    oal_pci_cache_flush(pst_pcie_dev, (oal_void*)addr_map.pa, sizeof(value));
    oal_pci_cache_inv(pst_pcie_dev, (oal_void*)addr_map.pa, sizeof(value));
#endif

    PCI_PRINT_LOG(PCI_LOG_DBG, "pcie writem cmd, write 0x%8x to 0x%4x callback-read= 0x%4x\n", \
                      cpu_addr, value, oal_readl((void*)addr_map.va));

    return ret;
}

oal_int32 oal_pcie_firmware_cmd_read_writem(oal_pcie_res *pst_pcie_res, oal_uint8* buff, oal_int32 len, oal_uint32 timeout)
{
    oal_uint32 size;

    PCI_PRINT_LOG(PCI_LOG_DBG, "pcie writem cmd read");

    size = OAL_MIN(strlen(STR_WRITEM_OK), len-1);
    strncpy(buff, STR_WRITEM_OK, size);
    buff[len - 1] = '\0';

    return OAL_SUCC;
}

oal_int32 oal_pcie_firmware_cmd_send_readm(oal_pcie_res *pst_pcie_res, oal_uint8* buff, oal_int32 len)
{
    oal_uint32 cpu_addr;
    oal_uint32 ulLen;

    PCI_PRINT_LOG(PCI_LOG_DBG, "pcie readm cmd send");

    cpu_addr = g_aulParamBuf[0];
    ulLen    = g_aulParamBuf[1];

    /* 起始地址和长度需要四字节对齐 */
    if ((0 != (cpu_addr % 4)) || (0 != (ulLen % 4)) || (0 == ulLen))
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pcie readm cmd error, addr 0x%x len %u", cpu_addr, ulLen);
        return -OAL_EINVAL;
    }

    return OAL_SUCC;
}

oal_int32 oal_pcie_firmware_cmd_read_readm(oal_pcie_res *pst_pcie_res, oal_uint8* buff, oal_int32 len, oal_uint32 timeout)
{
#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    oal_pci_dev_stru *pst_pcie_dev = NULL;
#endif
    pci_addr_map      addr_map;
    oal_uint32        cpu_addr;
    oal_uint32        ulLen;
    oal_int32         ret;

    PCI_PRINT_LOG(PCI_LOG_DBG, "pcie readm cmd read");

    cpu_addr = g_aulParamBuf[0];
    ulLen    = g_aulParamBuf[1];

    if (ulLen < len)
    {
        OAL_IO_PRINT("pcie readm read len %u larger than buf len %d!\n", ulLen, len);
        return -OAL_EFAUL;
    }

    ret = oal_pcie_inbound_ca_to_va(pst_pcie_res, cpu_addr+ulLen-1, &addr_map);
    if(OAL_SUCC != ret)
    {
        OAL_IO_PRINT("pcie read readm len %u out of memory, addr 0x%8x!\n", ulLen, cpu_addr);
        return -OAL_EFAUL;
    }

    ret = oal_pcie_inbound_ca_to_va(pst_pcie_res, cpu_addr, &addr_map);
    if(OAL_SUCC != ret)
    {
        OAL_IO_PRINT("pcie read readm 0x%8x unmap, failed!\n", cpu_addr);
        return -OAL_EFAUL;
    }

#ifdef CONFIG_PCIE_MEM_WR_CACHE_ENABLE
    oal_pci_cache_inv(pst_pcie_dev, (oal_void*)addr_map.pa, ulLen);
#endif

    oal_pcie_memcopy((oal_ulong)buff, (oal_ulong)addr_map.va, ulLen);

    return ulLen;
}

oal_int32 oal_pcie_firmware_cmd_send_jump(oal_pcie_res *pst_pcie_res, oal_uint8* buff, oal_int32 len)
{
    return OAL_SUCC;
}

oal_int32 oal_pcie_firmware_cmd_read_jump(oal_pcie_res *pst_pcie_res, oal_uint8* buff, oal_int32 len, oal_uint32 timeout)
{
    return OAL_SUCC;
}

FIRMWARECMD_STRU *oal_pcie_firmware_search_cmd(oal_uint8* buff, oal_int32 len)
{
    oal_uint32 ulLoop   = 0;
    oal_uint32 ulCmdLen = 0;

    if (FILES_BIN_SEND == g_ul_files_flag)
    {
        PCI_PRINT_LOG(PCI_LOG_DBG, "pcie firmware:send file bin, no need search cmd");
        return g_pstCurrentFirmwareCmd;
    }

    for (ulLoop = 0; ulLoop < g_ulFirmwareCmdNum; ulLoop++)
    {
        ulCmdLen = strlen(g_astFirmwareCmdTable[ulLoop].acCmdName);
        if (0 == strncmp(g_astFirmwareCmdTable[ulLoop].acCmdName, (oal_int8 *)buff, ulCmdLen))
        {
            return ((FIRMWARECMD_STRU *)&g_astFirmwareCmdTable[ulLoop]);
        }
    }

    return NULL;
}

oal_int32 oal_pcie_firmware_get_param(oal_uint8* buff, oal_int32 len)
{
    oal_uint8 *pucDataBuff = buff;
    oal_uint8 *pucTmp;
    oal_uint32 ulLen = 0;
    oal_uint32 ulParam;
    //oal_int32  ret;
    oal_uint32 ulParamIndex = 0;

    if (NULL == g_pstCurrentFirmwareCmd)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "g_pstCurrentFirmwareCmd is null");
        return -OAL_EINVAL;
    }

    if (0 == g_pstCurrentFirmwareCmd->ucArgs)
    {
        return OAL_SUCC;
    }

    if (FILES_BIN_SEND == g_ul_files_flag)
    {
        return OAL_SUCC;
    }

    memset(g_aulParamBuf, 0, sizeof(g_aulParamBuf));

    ulLen = strlen(g_pstCurrentFirmwareCmd->acCmdName);
    pucDataBuff += ulLen;

    while ((NULL != pucDataBuff) && (ulParamIndex < FIRMWARE_MAXPARAMNUM))
    {
        if (COMPART_CHAR != *pucDataBuff)
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "param buff is error,[%s]", pucDataBuff);
            return -OAL_EINVAL;
        }

        pucDataBuff++;

        if (pucDataBuff - buff >= len)
        {
            break;
        }

        //ret = kstrtouint(pucDataBuff, 0, *ulParam); //0自动识别进制
        pucTmp  = pucDataBuff;
        ulParam = simple_strtoul(pucTmp, (char **)&pucDataBuff, 0); //0自动识别进制

        g_aulParamBuf[ulParamIndex] = ulParam;
        ulParamIndex++;

        //pucDataBuff = strchr(pucDataBuff, COMPART_CHAR);
    }

    if (ulParamIndex >= g_pstCurrentFirmwareCmd->ucArgs)
    {
        return OAL_SUCC;
    }
    else
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "param number is error");
        return -OAL_EINVAL;
    }
}

oal_int32 oal_pcie_firmware_read(oal_pcie_linux_res *pst_pcie_lres, oal_uint8* buff, oal_int32 len, oal_uint32 timeout)
{
    oal_pcie_res *pst_pcie_res = pst_pcie_lres->pst_pci_res;

    if (NULL == pst_pcie_res)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pcie_res is null");
        return -OAL_ENODEV;
    }

    if (NULL == buff)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "buff is null");
        return -OAL_EFAUL;
    }

    if (NULL != g_pstCurrentFirmwareCmd)
    {
        return g_pstCurrentFirmwareCmd->pCmdRead(pst_pcie_res, buff, len, timeout);
    }
    else
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "current firmware cmd struct is null");
        return -OAL_EFAUL;
    }
}

oal_int32 oal_pcie_firmware_write(oal_pcie_linux_res *pst_pcie_lres, oal_uint8* buff, oal_int32 len)
{
    oal_int32     lState = -OAL_EFAUL;
    oal_pcie_res *pst_pcie_res = pst_pcie_lres->pst_pci_res;

    if (NULL == pst_pcie_res)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "pst_pcie_res is null");
        return -OAL_ENODEV;
    }

    if (NULL == buff)
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "buff is null");
        return -OAL_EFAUL;
    }

    if(PCI_DBG_CONDTION())
        oal_print_hex_dump(buff, len > 128 ? 128:len, 32, "pcie patch write: ");

    g_pstCurrentFirmwareCmd = oal_pcie_firmware_search_cmd(buff, len);
    if (NULL != g_pstCurrentFirmwareCmd)
    {
        lState = oal_pcie_firmware_get_param(buff, len);
        if (OAL_SUCC == lState)
        {
            return g_pstCurrentFirmwareCmd->pCmdSend(pst_pcie_res, buff, len);
        }
        else
        {
            PCI_PRINT_LOG(PCI_LOG_ERR, "pcie firmware get param error");
            return -OAL_EFAIL;
        }
    }
    else
    {
        PCI_PRINT_LOG(PCI_LOG_ERR, "not support pcie firmware cmd");
        oal_print_hex_dump((oal_uint8*)buff, len > 1024 ? 1024:len, 32, "firmware cmd: ");
        return -OAL_EFAIL;
    }
}

#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

