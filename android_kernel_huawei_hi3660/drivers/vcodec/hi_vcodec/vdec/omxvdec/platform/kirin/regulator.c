
#include "omxvdec.h"
#include "platform.h"
#include "regulator.h"

#include <linux/hisi/hisi-iommu.h>   //for struct iommu_domain_data
#include <linux/iommu.h>             //for struct iommu_domain
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/clk.h>

/*lint -e774*/

/********************************** MACRO *************************************/
#define REGULATOR_NAME          "ldo_vdec"
#define VCODEC_CLOCK_NAME       "clk_vdec"
#define VCODEC_CLK_RATE         "dec_clk_rate"
#ifdef PLATFORM_HI3660
#define VDEC_QOS_MODE           0xE893000C
#endif

/*********************************** VARS *************************************/
static HI_U32  g_VdecClkRate_l  = 200000000;    //200MHZ
static HI_U32  g_VdecClkRate_h  = 480000000;    //480MHZ
static HI_U32  g_CurClkRate     = 0;
static HI_U32  g_vdecQosMode    = 0x2;
static HI_BOOL g_VdecPowerOn    = HI_FALSE;
static struct  regulator    *g_VdecRegulator  = HI_NULL;
static struct  iommu_domain *g_VdecSmmuDomain = HI_NULL;
static VFMW_DTS_CONFIG_S     g_DtsConfig;
static struct clk *g_PvdecClk = HI_NULL;
struct mutex   g_RegulatorMut;


/******************************** LOCAL FUNC **********************************/
#ifdef HIVDEC_SMMU_SUPPORT

/*----------------------------------------
    func: iommu enable intf
 ----------------------------------------*/
static HI_S32 VDEC_Enable_Iommu(struct device *dev)
{
    g_VdecSmmuDomain = hisi_ion_enable_iommu(NULL);
    if (HI_NULL == g_VdecSmmuDomain)
    {
        OmxPrint(OMX_FATAL, "%s iommu_domain_alloc failed!\n", __func__);
        return HI_FAILURE;/*lint !e570 */
    }
    return HI_SUCCESS;
} /*lint !e715*/

/*----------------------------------------
    func: iommu disable intf
 ----------------------------------------*/
static HI_VOID VDEC_Disable_Iommu(struct device *dev)
{
    if((NULL != g_VdecSmmuDomain) && (NULL != dev))
        g_VdecSmmuDomain = NULL;
}

/*----------------------------------------
    func: get smmu page table base addr
 ----------------------------------------*/
static HI_U64 VDEC_GetSmmuBasePhy(struct device *dev)
{
    struct iommu_domain_data *domain_data = HI_NULL;

    if (HI_FAILURE == VDEC_Enable_Iommu(dev))
        return 0;

    domain_data = (struct iommu_domain_data *)(g_VdecSmmuDomain->priv);

    return (HI_U64)(domain_data->phy_pgd_base);
}

#endif


/*----------------------------------------
    func: initialize vcodec clock rate
 ----------------------------------------*/
static HI_S32 VDEC_Init_ClockRate(struct device *dev)
{
    HI_S32 ret;
    struct device_node *np = HI_NULL;
    struct clk *pvdec_clk = HI_NULL;

    if (HI_NULL == dev)
    {
        printk(KERN_CRIT "%s dev is NULL!\n", __func__);
        return HI_FAILURE;
    }

    np = dev->of_node;

    pvdec_clk = devm_clk_get(dev, VCODEC_CLOCK_NAME);
    if (IS_ERR_OR_NULL(pvdec_clk))
    {
        printk(KERN_CRIT "%s can not get clock!\n", __func__);
        return HI_FAILURE;
    }
    g_PvdecClk = pvdec_clk;

    ret = of_property_read_u32_index(np, VCODEC_CLK_RATE, 1, &g_VdecClkRate_l);
    if (ret)
    {
        printk(KERN_CRIT "%s can not get g_VdecClkRate_l, return %d\n", __func__, ret);
        g_VdecClkRate_l = 200000000;    //200MHZ
        return HI_FAILURE;
    }
    OmxPrint(OMX_ALWS, "%s get g_VdecClkRate_l = %u\n", __func__, g_VdecClkRate_l);

    ret = of_property_read_u32_index(np, VCODEC_CLK_RATE, 0, &g_VdecClkRate_h);
    if (ret)
    {
        printk(KERN_CRIT "%s can not get g_VdecClkRate_h, return %d\n", __func__, ret);
        g_VdecClkRate_h = 480000000;    //480MHZ
        return HI_FAILURE;
    }
    OmxPrint(OMX_ALWS, "%s get g_VdecClkRate_h = %u\n", __func__, g_VdecClkRate_h);

#ifdef PLATFORM_HI3660
    ret  = clk_set_rate(pvdec_clk, g_VdecClkRate_h);
    if (ret)
    {
        printk(KERN_CRIT "%s Failed to clk_set_rate, return %d\n", __func__, ret);
        return HI_FAILURE;
    }
    OmxPrint(OMX_ALWS, "%s set VdecClkRate = %u\n", __func__, g_VdecClkRate_h);
#endif

    g_CurClkRate = g_VdecClkRate_h;

    return HI_SUCCESS;
}


/*----------------------------------------
    func: get dts configure
          as reg base & irq num
 ----------------------------------------*/
static HI_S32 VDEC_GetDtsConfigInfo(struct device *dev, VFMW_DTS_CONFIG_S *pDtsConfig)
{
    HI_S32 ret;
    struct device_node *np_crg = HI_NULL;
    struct device_node *np = dev->of_node;
    struct resource res;

    if (HI_NULL == np)
    {
        printk(KERN_CRIT "%s the device node is null\n", __func__);
        return HI_FAILURE;
    }

    if (HI_NULL == pDtsConfig)
    {
        printk(KERN_CRIT "%s pDtsConfig is null\n", __func__);
        return HI_FAILURE;
    }

    //Get irq num, return 0 if failed
    pDtsConfig->MfdeIrqNum = irq_of_parse_and_map(np, 0);
    if (0 == pDtsConfig->MfdeIrqNum)
    {
        printk(KERN_CRIT "%s irq_of_parse_and_map MfdeIrqNum failed!\n", __func__);
        return HI_FAILURE;
    }

    pDtsConfig->ScdIrqNum = irq_of_parse_and_map(np, 1);
    if (0 == pDtsConfig->ScdIrqNum)
    {
        printk(KERN_CRIT "%s irq_of_parse_and_map ScdIrqNum failed!\n", __func__);
        return HI_FAILURE;
    }

    pDtsConfig->BpdIrqNum = irq_of_parse_and_map(np, 2);
    if (0 == pDtsConfig->BpdIrqNum)
    {
        printk(KERN_CRIT "%s irq_of_parse_and_map BpdIrqNum failed!\n", __func__);
        return HI_FAILURE;
    }

    pDtsConfig->SmmuIrqNum = irq_of_parse_and_map(np, 3);
    if (0 == pDtsConfig->SmmuIrqNum)
    {
        printk(KERN_CRIT "%s irq_of_parse_and_map SmmuIrqNum failed!\n", __func__);
        return HI_FAILURE;
    }

    pDtsConfig->MfdeSafeIrqNum = 326;//irq_of_parse_and_map(np, 4);
    if (0 == pDtsConfig->MfdeSafeIrqNum)
    {
        printk(KERN_CRIT "%s irq_of_parse_and_map MfdeSafeIrqNum failed!\n", __func__);
        return HI_FAILURE;
    }

    pDtsConfig->ScdSafeIrqNum = 327;//irq_of_parse_and_map(np, 5);
    if (0 == pDtsConfig->ScdSafeIrqNum)
    {
        printk(KERN_CRIT "%s irq_of_parse_and_map ScdSafeIrqNum failed!\n", __func__);
        return HI_FAILURE;
    }

    pDtsConfig->BpdSafeIrqNum = 328;//irq_of_parse_and_map(np, 6);
    if (0 == pDtsConfig->BpdSafeIrqNum)
    {
        printk(KERN_CRIT "%s irq_of_parse_and_map BpdSafeIrqNum failed!\n", __func__);
        return HI_FAILURE;
    }

    pDtsConfig->SmmuSafeIrqNum = 329;//irq_of_parse_and_map(np, 7);
    if (0 == pDtsConfig->SmmuSafeIrqNum)
    {
        printk(KERN_CRIT "%s irq_of_parse_and_map SmmuSafeIrqNum failed!\n", __func__);
        return HI_FAILURE;
    }

    //Get reg base addr & size, return 0 if success
    ret = of_address_to_resource(np, 0, &res);
    if (ret)
    {
        printk(KERN_CRIT "%s of_address_to_resource failed! return %d\n", __func__, ret);
        return HI_FAILURE;
    }
    pDtsConfig->VdhRegBaseAddr = res.start;
    pDtsConfig->VdhRegRange    = resource_size(&res);

#ifdef HIVDEC_SMMU_SUPPORT
    //Get reg base addr & size, return 0 if failed
    pDtsConfig->SmmuPageBaseAddr = VDEC_GetSmmuBasePhy(dev);
    if (0 == pDtsConfig->SmmuPageBaseAddr)
    {
        printk(KERN_CRIT "%s Regulator_GetSmmuBasePhy failed!\n", __func__);
        return HI_FAILURE;
    }
#endif

    np_crg = of_find_compatible_node(HI_NULL, HI_NULL, "hisilicon,crgctrl");
    ret = of_address_to_resource(np_crg, 0, &res);
    if (ret)
    {
        printk(KERN_CRIT "%s of_address_to_resource crg failed! return %d\n", __func__, ret);
        return HI_FAILURE;
    }
    pDtsConfig->PERICRG_RegBaseAddr = res.start;

    //Check if is FPGA platform
    ret = of_property_read_u32(np, "vdec_fpga", &pDtsConfig->IsFPGA);
    if (ret)
    {
        pDtsConfig->IsFPGA = 0;
        OmxPrint(OMX_ALWS, "Is not FPGA platform!\n");
    }

    /* get vdec qos mode */
    ret = of_property_read_u32(np, "vdec_qos_mode", &g_vdecQosMode);
    if(ret)
    {
        g_vdecQosMode = 0x2;
        OmxPrint(OMX_WARN, "get vdec qos mode failed set default!\n");
    }

    ret = VDEC_Init_ClockRate(dev);
    if (ret != HI_SUCCESS)
    {
        printk(KERN_ERR "%s VDEC_Init_ClockRate failed!\n", __func__);
        return HI_FAILURE;
        //fixme: continue or return?
    }

    return HI_SUCCESS;
}

#ifdef PLATFORM_HI3660
/*----------------------------------------
    func: get dts configure
          as reg base & irq num
 ----------------------------------------*/
static HI_S32 VDEC_Config_QOS(void)
{
    HI_S32 ret = HI_FAILURE;
    HI_U32 *qos_addr = HI_NULL;

    qos_addr = (HI_U32 *)ioremap(VDEC_QOS_MODE, 4);
    if(qos_addr)
    {
        writel(g_vdecQosMode, qos_addr);
        iounmap(qos_addr);

        ret = HI_SUCCESS;
    }
    else
    {
        OmxPrint(OMX_ERR, "ioremap VDEC_QOS_MODE reg failed!\n");
        ret = HI_FAILURE;
    }

    return ret;
}
#endif

/******************************** SHARE FUNC **********************************/

/*----------------------------------------
    func: regulator probe entry
 ----------------------------------------*/
HI_S32 VDEC_Regulator_Probe(struct device *dev)
{
    HI_S32 ret;

    g_VdecRegulator = HI_NULL;

    if (HI_NULL == dev)
    {
       printk(KERN_CRIT "%s, invalid params!", __func__);
       return HI_FAILURE;
    }

    g_VdecRegulator = devm_regulator_get(dev, REGULATOR_NAME);
    if (HI_NULL == g_VdecRegulator)
    {
        printk(KERN_CRIT "%s get regulator failed!\n", __func__);
        return HI_FAILURE;
    }
    else if (IS_ERR(g_VdecRegulator))
    {
        OmxPrint(OMX_FATAL, "%s get regulator failed, error no = %ld!\n", __func__, PTR_ERR(g_VdecRegulator));
        g_VdecRegulator = HI_NULL;
        return HI_FAILURE;
    }

    memset(&g_DtsConfig, 0, sizeof(VFMW_DTS_CONFIG_S)); /* unsafe_function_ignore: memset */
    ret = VDEC_GetDtsConfigInfo(dev, &g_DtsConfig);
    if (ret != HI_SUCCESS)
    {
        printk(KERN_CRIT "%s Regulator_GetDtsConfigInfo failed.\n", __func__);
        return HI_FAILURE;
    }

    ret = VFMW_SetDtsConfig(&g_DtsConfig);
    if (ret != HI_SUCCESS)
    {
        printk(KERN_CRIT "%s VFMW_SetDtsConfig failed.\n", __func__);
        return HI_FAILURE;
    }
    VDEC_INIT_MUTEX(&g_RegulatorMut);

    return HI_SUCCESS;
}

/*----------------------------------------
    func: regulator deinitialize
 ----------------------------------------*/
HI_S32 VDEC_Regulator_Remove(struct device *dev)
{
    VDEC_DOWN_INTERRUPTIBLE(&g_RegulatorMut);

    VDEC_Disable_Iommu(dev);
    g_VdecRegulator = HI_NULL;
    g_PvdecClk      = HI_NULL;

    VDEC_UP_INTERRUPTIBLE(&g_RegulatorMut);

    return HI_SUCCESS;
}

#ifdef PLATFORM_HI3660
static HI_S32 VDEC_Regulator_Enable_Kirin960(HI_VOID)
{
    HI_S32 ret;

    VDEC_DOWN_INTERRUPTIBLE(&g_RegulatorMut);

    if (HI_TRUE == g_VdecPowerOn)
    {
        VDEC_UP_INTERRUPTIBLE(&g_RegulatorMut);
        return HI_SUCCESS;
    }
    if (HI_NULL == g_PvdecClk)
    {
        printk(KERN_CRIT "%s g_PvdecClk is NULL!\n", __func__);
        goto error_exit;
    }

    if (IS_ERR_OR_NULL(g_VdecRegulator))
    {
       OmxPrint(OMX_WARN, "%s g_VdecRegulator = NULL!", __func__ );
        goto error_exit;
    }

    ret = clk_prepare_enable(g_PvdecClk);
    if (ret != 0)
    {
        printk(KERN_CRIT "%s clk_prepare_enable failed!\n", __func__);
        goto error_exit;
    }

    OmxPrint(OMX_ALWS, "%s, call regulator_enable \n", __func__);
    ret = regulator_enable(g_VdecRegulator);
    if (ret != 0)
    {
        printk(KERN_CRIT "%s enable regulator failed!\n", __func__);
        goto error_clk_unprepare;
    }

    ret = VDEC_Config_QOS();
    if(ret != 0)
    {
        printk(KERN_ERR "VDEC_Config_QOS failed!\n");
        goto error_regulator_disable;
    }

    g_VdecPowerOn = HI_TRUE;

    //OmxPrint(OMX_INFO, "%s enable regulator success!\n", __func__);

    VDEC_UP_INTERRUPTIBLE(&g_RegulatorMut);

    return HI_SUCCESS;

error_regulator_disable:
    regulator_disable(g_VdecRegulator);

error_clk_unprepare:
    clk_disable_unprepare(g_PvdecClk);

error_exit:
    VDEC_UP_INTERRUPTIBLE(&g_RegulatorMut);

    return HI_FAILURE;
}
#endif

#ifdef PLATFORM_KIRIN970
static HI_S32 VDEC_Regulator_Enable_Kirin970(HI_VOID)
{
    HI_S32 ret;

    VDEC_DOWN_INTERRUPTIBLE(&g_RegulatorMut);

    if (HI_TRUE == g_VdecPowerOn)
    {
        VDEC_UP_INTERRUPTIBLE(&g_RegulatorMut);
        return HI_SUCCESS;
    }
    if (HI_NULL == g_PvdecClk)
    {
        printk(KERN_CRIT "%s g_PvdecClk is NULL!\n", __func__);
        goto error_exit;
    }

    if (IS_ERR_OR_NULL(g_VdecRegulator))
    {
       OmxPrint(OMX_WARN, "%s g_VdecRegulator = NULL!", __func__ );
        goto error_exit;
    }

    OmxPrint(OMX_ALWS, "%s, call regulator_enable \n", __func__);
    ret = regulator_enable(g_VdecRegulator);
    if (ret != 0)
    {
        printk(KERN_CRIT "%s enable regulator failed!\n", __func__);
        goto error_exit;
    }

    ret  = clk_set_rate(g_PvdecClk, g_VdecClkRate_h);
    if (ret)
    {
        printk(KERN_CRIT "%s Failed to clk_set_rate, return %d\n", __func__, ret);
        goto error_regulator_disable;
    }
    OmxPrint(OMX_ALWS, "%s set VdecClkRate = %u\n", __func__, g_VdecClkRate_h);

    ret = clk_prepare_enable(g_PvdecClk);
    if (ret != 0)
    {
        printk(KERN_CRIT "%s clk_prepare_enable failed!\n", __func__);
        goto error_regulator_disable;
    }

/*
    ret = VDEC_Config_QOS();
    if(ret != 0)
    {
        printk(KERN_ERR "VDEC_Config_QOS failed!\n");
        goto error_clk_unprepare;
    }
*/

    g_VdecPowerOn = HI_TRUE;

    //OmxPrint(OMX_INFO, "%s enable regulator success!\n", __func__);

    VDEC_UP_INTERRUPTIBLE(&g_RegulatorMut);

    return HI_SUCCESS;

/*
error_clk_unprepare:
    clk_disable_unprepare(g_PvdecClk);
*/

error_regulator_disable:
    regulator_disable(g_VdecRegulator);

error_exit:
    VDEC_UP_INTERRUPTIBLE(&g_RegulatorMut);

    return HI_FAILURE;
}
#endif

/*----------------------------------------
    func: enable regulator
 ----------------------------------------*/
HI_S32 VDEC_Regulator_Enable(HI_VOID)
{
#ifdef PLATFORM_HI3660
    return VDEC_Regulator_Enable_Kirin960();
#endif

#ifdef PLATFORM_KIRIN970
    return VDEC_Regulator_Enable_Kirin970();
#endif
}

#ifdef PLATFORM_HI3660
static HI_S32 VDEC_Regulator_Disable_Kirin960(HI_VOID)
{
    HI_S32 ret;

    VDEC_DOWN_INTERRUPTIBLE(&g_RegulatorMut);

    if (HI_FALSE == g_VdecPowerOn)
    {
        VDEC_UP_INTERRUPTIBLE(&g_RegulatorMut);
        return HI_SUCCESS;
    }

    if (HI_NULL == g_PvdecClk)
    {
        printk(KERN_CRIT "%s g_PvdecClk is NULL!\n", __func__);
        goto error_exit;
    }

    if (IS_ERR_OR_NULL(g_VdecRegulator))
    {
        OmxPrint(OMX_WARN, "%s g_VdecRegulator = NULL!", __func__ );
        goto error_exit;
    }

    OmxPrint(OMX_ALWS, "%s, call regulator_disable\n", __func__);
    ret = regulator_disable(g_VdecRegulator);
    if (ret != 0)
    {
        printk(KERN_CRIT "%s disable regulator failed!\n", __func__);
        goto error_exit;
    }

    clk_disable_unprepare(g_PvdecClk);

    g_VdecPowerOn = HI_FALSE;

    //OmxPrint(OMX_INFO, "%s disable regulator success!\n", __func__);

    VDEC_UP_INTERRUPTIBLE(&g_RegulatorMut);

    return HI_SUCCESS;

error_exit:
    VDEC_UP_INTERRUPTIBLE(&g_RegulatorMut);

    return HI_FAILURE;
}
#endif

#ifdef PLATFORM_KIRIN970
static HI_S32 VDEC_Regulator_Disable_Kirin970(HI_VOID)
{
    HI_S32 ret;

    VDEC_DOWN_INTERRUPTIBLE(&g_RegulatorMut);

    if (HI_FALSE == g_VdecPowerOn)
    {
        VDEC_UP_INTERRUPTIBLE(&g_RegulatorMut);
        return HI_SUCCESS;
    }

    if (HI_NULL == g_PvdecClk)
    {
        printk(KERN_CRIT "%s g_PvdecClk is NULL!\n", __func__);
        goto error_exit;
    }

    if (IS_ERR_OR_NULL(g_VdecRegulator))
    {
        OmxPrint(OMX_WARN, "%s g_VdecRegulator = NULL!", __func__ );
        goto error_exit;
    }

    clk_disable_unprepare(g_PvdecClk);

    OmxPrint(OMX_ALWS, "%s, call regulator_disable\n", __func__);
    ret = regulator_disable(g_VdecRegulator);
    if (ret != 0)
    {
        printk(KERN_CRIT "%s disable regulator failed!\n", __func__);
        goto error_exit;
    }

    g_VdecPowerOn = HI_FALSE;

    //OmxPrint(OMX_INFO, "%s disable regulator success!\n", __func__);

    VDEC_UP_INTERRUPTIBLE(&g_RegulatorMut);

    return HI_SUCCESS;

error_exit:
    VDEC_UP_INTERRUPTIBLE(&g_RegulatorMut);

    return HI_FAILURE;
}
#endif

/*----------------------------------------
    func: disable regulator
 ----------------------------------------*/
HI_S32 VDEC_Regulator_Disable(HI_VOID)
{
#ifdef PLATFORM_HI3660
    return VDEC_Regulator_Disable_Kirin960();
#endif

#ifdef PLATFORM_KIRIN970
    return VDEC_Regulator_Disable_Kirin970();
#endif
}


/*----------------------------------------
    func: get decoder clock rate
 ----------------------------------------*/
HI_S32 VDEC_Regulator_GetClkRate(CLK_RATE_E *pClkRate)
{
    VDEC_DOWN_INTERRUPTIBLE(&g_RegulatorMut);

    if (g_CurClkRate == g_VdecClkRate_h)
    {
        *pClkRate = CLK_RATE_HIGH;
    }
    else if (g_CurClkRate == g_VdecClkRate_l)
    {
        *pClkRate = CLK_RATE_LOW;
    }
    else
    {
        OmxPrint(OMX_ERR, "%s: unkown clk rate %d, return CLK_RATE_HIGH\n", __func__, g_CurClkRate);
        *pClkRate = CLK_RATE_HIGH;
    }
    VDEC_UP_INTERRUPTIBLE(&g_RegulatorMut);

    return HI_SUCCESS;
}

/*----------------------------------------
    func: decoder clock rate select
 ----------------------------------------*/
HI_S32 VDEC_Regulator_SetClkRate(CLK_RATE_E eClkRate)
{
    HI_S32 ret  = 0;
    HI_U32 rate = 0;
    HI_U8  need_set_flag = 1;

    VDEC_DOWN_INTERRUPTIBLE(&g_RegulatorMut);

    if (g_DtsConfig.IsFPGA)
    {
        //OmxPrint(OMX_INFO, "Is FPGA platform, no need to change VCodec rate!\n");
        VDEC_UP_INTERRUPTIBLE(&g_RegulatorMut);
        return HI_SUCCESS;
    }

    if (IS_ERR_OR_NULL(g_PvdecClk))
    {
        printk(KERN_ERR "Couldn't get clk! [%s]\n", __func__);
        goto error_exit;
    }

    rate = (HI_U32)clk_get_rate(g_PvdecClk);
    switch (eClkRate)
    {
        case CLK_RATE_LOW:
            if (g_VdecClkRate_l == rate)
            {
                //OmxPrint(OMX_INFO, "VCodec already in %u HZ clock rate\n", rate);
                need_set_flag = 0;
            }
            else
            {
                rate = g_VdecClkRate_l;
                need_set_flag = 1;
            }
            break;

        case CLK_RATE_HIGH:
            if (g_VdecClkRate_h == rate)
            {
                //OmxPrint(OMX_INFO, "VCodec already in %u HZ clock rate\n", rate);
                need_set_flag = 0;
            }
            else
            {
                rate = g_VdecClkRate_h;
                need_set_flag = 1;
            }
            break;

        default:
            printk(KERN_ERR "[%s] unsupport clk rate enum %d\n", __func__, eClkRate);
            goto error_exit;
    }

    if (need_set_flag == 1)
    {
        //OmxPrint(OMX_INFO, "Prepare to change VCodec clock rate to %u HZ\n", rate);

        ret = clk_set_rate(g_PvdecClk, rate);
        if (ret != 0)
        {
            printk(KERN_ERR "Failed to clk_set_rate %u HZ[%s] ret : %d\n", rate, __func__, ret);
            goto error_exit;
        }
        g_CurClkRate = rate;
        //OmxPrint(OMX_INFO, "Success changed VCodec clock rate to %u HZ\n", rate);
    }

    VDEC_UP_INTERRUPTIBLE(&g_RegulatorMut);

    return HI_SUCCESS;

error_exit:
    VDEC_UP_INTERRUPTIBLE(&g_RegulatorMut);

    return HI_FAILURE;
}


/*----------------------------------------
    func: regulator read proc entry
 ----------------------------------------*/
HI_VOID VDEC_Regulator_Read_Proc(HI_VOID *pParam, HI_VOID *v)
{
    struct seq_file *p = (struct seq_file *)pParam;

    PROC_PRINT(p, "----------------------- Regulator Info -----------------------\n\n");

    PROC_PRINT(p, "IsFpga           :%-10d\n", g_DtsConfig.IsFPGA);
    PROC_PRINT(p, "PowerState       :%-10s\n", (HI_TRUE==g_VdecPowerOn)?"ON":"OFF");
    if (0 == g_DtsConfig.IsFPGA)
    {
    PROC_PRINT(p, "ClockRate        :%-10d (%d L/%d H)\n", g_CurClkRate, g_VdecClkRate_l, g_VdecClkRate_h);
    }

    PROC_PRINT(p, "MfdeIrqNum       :%-10d  | ScdIrqNum          :%d\n",      g_DtsConfig.MfdeIrqNum,          g_DtsConfig.ScdIrqNum);
    PROC_PRINT(p, "BpdIrqNum        :%-10d  | SmmuIrqNum         :%d\n",      g_DtsConfig.BpdIrqNum,           g_DtsConfig.SmmuIrqNum);
    PROC_PRINT(p, "MfdeSafeIrqNum   :%-10d  | ScdSafeIrqNum      :%d\n",      g_DtsConfig.MfdeSafeIrqNum,      g_DtsConfig.ScdSafeIrqNum);
    PROC_PRINT(p, "BpdSafeIrqNum    :%-10d  | SmmuSafeIrqNum     :%d\n",      g_DtsConfig.BpdSafeIrqNum,       g_DtsConfig.SmmuSafeIrqNum);
    PROC_PRINT(p, "VdhRegBaseAddr   :%-10x  | VdhRegRange        :%d\n",      g_DtsConfig.VdhRegBaseAddr,      g_DtsConfig.VdhRegRange);
    PROC_PRINT(p, "PERICRG_BaseAddr :%-10x  | SmmuPageBaseAddr   :%-12llx\n", g_DtsConfig.PERICRG_RegBaseAddr, g_DtsConfig.SmmuPageBaseAddr);

    PROC_PRINT(p, "--------------------------------------------------------------\n\n");

    return;
}

