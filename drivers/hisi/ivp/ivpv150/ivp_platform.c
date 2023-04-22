#include <linux/of.h>
#include "ivp_log.h"
#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include "ivp_platform.h"
#include "ivp_reg.h"
#include "ivp_atf_subsys.h"

#define REMAP_ADD                        (0xe8d00000)
#define DEAD_FLAG                        (0xdeadbeef)
#define SIZE_16K                         (16 * 1024)
extern void ivp_reg_write(unsigned int off, u32 val);
extern u32 ivp_reg_read(unsigned int off);
static void *iram = NULL;

static int ivp_get_memory_section(struct platform_device *pdev,
                                              struct ivp_device *ivp_devp)
{
    int i = 0;
    int ret = -1;
    unsigned int size = 0;
    dma_addr_t dma_addr = 0;

    if (pdev == NULL || ivp_devp == NULL) {
        ivp_err("pointer is NULL.");
        return -EINVAL;
    }

    ret = of_property_read_u32(pdev->dev.of_node, OF_IVP_DYNAMIC_MEM, &size);
    if ((0 != ret) || (0 == size)) {
        ivp_err("get failed/not use dynamic mem, ret:%d", ret);
        return ret;
    }
    ivp_devp->dynamic_mem_size = size;

    ivp_devp->ivp_meminddr_len = size;

    ret = of_property_read_u32(pdev->dev.of_node, OF_IVP_DYNAMIC_MEM_SEC_SIZE, &size);
    if ((0 != ret) || (0 == size)) {
        ivp_err("get failed/not use dynamic mem, ret:%d", ret);
        return ret;
    }
    ivp_devp->dynamic_mem_section_size = size;

    if ((ivp_devp->dynamic_mem_section_size * (ivp_devp->sect_count - 3)) != ivp_devp->dynamic_mem_size) {
        ivp_err("dynamic_mem should be sect_count-3 times dynamic_mem_section");
        return -EINVAL;
    }

    /*lint -save -e598 -e648*/
    dma_set_mask_and_coherent(&ivp_devp->ivp_pdev->dev, DMA_BIT_MASK(64));
    /*lint -restore */
    ivp_devp->vaddr_memory = dma_alloc_coherent(&ivp_devp->ivp_pdev->dev,ivp_devp->dynamic_mem_size,&dma_addr,GFP_KERNEL);
    if (ivp_devp->vaddr_memory == NULL) {
        ivp_err("[%s] ivp_get_vaddr.0x%pK\n", __func__, ivp_devp->vaddr_memory);
        return -EINVAL;
    }

    for(i = 3; i < ivp_devp->sect_count; i++) {
        if(i == 3) {
            ivp_devp->sects[i].acpu_addr = dma_addr >> 4;
        }
        else {
            ivp_devp->sects[i].acpu_addr = ((ivp_devp->sects[i-1].acpu_addr << 4)
                     + ivp_devp->sects[i-1].len) >> 4;
            ivp_devp->sects[i].ivp_addr = ivp_devp->sects[i-1].ivp_addr
                     + ivp_devp->sects[i-1].len ;
        }
        ivp_devp->sects[i].len = ivp_devp->dynamic_mem_section_size;
        ivp_dbg("________ivp sections 0x%pK\n", ivp_devp->sects[i].acpu_addr);
    }

    return 0;
}

static void ivp_free_memory_section(struct ivp_device *ivp_devp)
{
    dma_addr_t dma_addr = 0;
    dma_addr = ivp_devp->sects[3].acpu_addr << 4;

    if (ivp_devp->vaddr_memory != NULL) {
        dma_free_coherent(&ivp_devp->ivp_pdev->dev, ivp_devp->dynamic_mem_size,ivp_devp->vaddr_memory, dma_addr);
        ivp_devp->vaddr_memory = NULL;
    }
}

static inline void ivp_hw_remap_ivp2ddr(unsigned int ivp_addr,
                                                   unsigned int len,
                                                   unsigned int ddr_addr)
{
    ivp_reg_write(ADDR_IVP_CFG_SEC_REG_START_REMAP_ADDR, ivp_addr / SIZE_1MB);
    ivp_reg_write(ADDR_IVP_CFG_SEC_REG_REMAP_LENGTH, len);
    ivp_reg_write(ADDR_IVP_CFG_SEC_REG_DDR_REMAP_ADDR, ddr_addr / SIZE_1MB);
}

static inline int ivp_remap_addr_ivp2ddr(unsigned int ivp_addr,
                                                     int len,
                                                     unsigned int ddr_addr)
{
    ivp_dbg("ivp_addr:%#x, len:%#x, ddr_addr:%#x", ivp_addr, len, ddr_addr);
    if ((ivp_addr & MASK_1MB) != 0 ||
        (ddr_addr & MASK_1MB) != 0 ||
        len >= 128 * SIZE_1MB) {
        ivp_err("not aligned");
        return -EINVAL;
    }
    len = (len + SIZE_1MB - 1) / SIZE_1MB - 1;
    ivp_hw_remap_ivp2ddr(ivp_addr, (u32)len, (u32)ddr_addr);
    return 0;
}

int ivp_poweron_pri(struct ivp_device *ivp_devp)
{
    int ret = 0;

    //1.Set Clock rate
    ret = clk_set_rate(ivp_devp->clk, (unsigned long)ivp_devp->clk_rate);
    if (ret != 0) {
        ivp_err("set rate %#x fail, ret:%d", ivp_devp->clk_rate, ret);
        ret = -EFAULT;
        return ret;
    }

    //2.Enable the clock
    ret = clk_prepare_enable(ivp_devp->clk);
    if (ret ) {
        ivp_err("i2c2_clk :clk prepare enable failed,ret=%d ",ret);
        return ret;
    }

    ivp_info("set core success to: %ld", clk_get_rate(ivp_devp->clk));

    //3.Enable the power
    ret = regulator_enable(ivp_devp->regulator);
    if (ret) {
        ivp_err("regularot enable failed [%d]!", ret);
        return ret;
    }

    ivpatf_change_slv_secmod(IVP_NONSEC);
    ivpatf_change_mst_secmod(IVP_NONSEC);

    return ret;
}

int ivp_poweron_remap(struct ivp_device *ivp_devp)
{
    int ret = 0;

    ret = ivp_remap_addr_ivp2ddr(ivp_devp->sects[3].ivp_addr,
                                 ivp_devp->ivp_meminddr_len,
                                 ivp_devp->sects[3].acpu_addr << IVP_MMAP_SHIFT);
    if (ret) {
        ivp_err("remap addr failed [%d]!", ret);
        return ret;
    }

    return ret;
}

int ivp_poweroff_pri(struct ivp_device *ivp_devp)
{
    int ret = 0;

    ivp_hw_enable_reset(ivp_devp);
    clk_disable_unprepare(ivp_devp->clk);

    ret = regulator_disable(ivp_devp->regulator);
    if (ret) {
        ivp_err("Power off failed [%d]!", ret);
    }

    return ret;
}

static int ivp_setup_regulator(struct platform_device *pdev,
                                          struct ivp_device *ivp_devp)
{
    int ret = 0;
    struct regulator *regulator = NULL;

    regulator = devm_regulator_get(&pdev->dev, IVP_REGULATOR);
    if (IS_ERR(regulator)) {
        ret = -ENODEV;
        ivp_err("Get ivp regulator failed, ret:%d", ret);
        return ret;

    } else {
        ivp_devp->regulator = regulator;
    }

    return ret;
}

static int ivp_setup_clk(struct platform_device *pdev,
                                 struct ivp_device *ivp_devp)
{
    int ret = 0;
    u32 clk_rate = 0;

    ivp_devp->clk = devm_clk_get(&pdev->dev, OF_IVP_CLK_NAME);
    if (IS_ERR(ivp_devp->clk)) {
        ivp_err("get clk failed");
        return -ENODEV;
    }

    ret = of_property_read_u32(pdev->dev.of_node, OF_IVP_CLK_RATE_NAME, &clk_rate);
    if (ret) {
        ivp_err("get rate failed, ret:%d", ret);
        return -ENOMEM;
    }
    ivp_devp->clk_rate = clk_rate;
    ivp_info("get clk rate: %u", clk_rate);

    return ret;
}

int ivp_change_clk(struct ivp_device *ivp_devp, unsigned int level)
{
    int ret = 0;

    return ret;
}

int ivp_init_pri(struct platform_device *pdev, struct ivp_device *ivp_devp)
{
    int ret = 0;

    ret = ivp_setup_regulator(pdev, ivp_devp);
    if (ret) {
        ivp_err("setup regulator failed, ret:%d", ret);
        return ret;
    }

    ret = ivp_setup_clk(pdev, ivp_devp);
    if (ret) {
        ivp_err("setup clk failed, ret:%d", ret);
        return ret;
    }

    ret = ivp_get_memory_section(pdev, ivp_devp);
    if (ret) {
        ivp_err("get memory section failed, ret:%d", ret);
        return ret;
    }

    return ret;
}

void ivp_deinit_pri(struct ivp_device *ivp_devp)
{
    ivp_free_memory_section(ivp_devp);
}

int ivp_init_resethandler(struct ivp_device *pdev)
{
    /* init code to remap address */
    iram = ioremap(REMAP_ADD, SIZE_16K);
    if (!iram) {
        ivp_err("Can't map ivp base address");
        return -1;
    }

    iowrite32(DEAD_FLAG, iram);

    return 0;
}

int ivp_check_resethandler(struct ivp_device *pdev)
{
    /* check init code in remap address */
    int inited = 0;
    uint32_t flag = ioread32(iram);
    if (flag != DEAD_FLAG)
        inited = 1;

    return inited;
}

void ivp_deinit_resethandler(struct ivp_device *pdev)
{
    /* deinit remap address */
    if(iram) {
        iounmap(iram);
    }
}

int ivp_sec_loadimage(struct ivp_device *pdev)
{
    ivp_err("not support sec ivp!");
    return -EINVAL;
}

void ivp_dev_hwa_enable(void)
{
    /*enable apb gate clock , watdog ,timer*/
    ivp_info("ivp will enable hwa.");
    ivp_reg_write(IVP_REG_OFF_APB_GATE_CLOCK, 0x00003FFF);
    ivp_reg_write(IVP_REG_OFF_TIMER_WDG_RST_DIS, 0x0000007F);

    return;
}

void ivp_hw_enable_reset(struct ivp_device *devp)
{
    ivp_reg_write(IVP_REG_OFF_DSP_CORE_RESET_EN, 0x02);
    ivp_reg_write(IVP_REG_OFF_DSP_CORE_RESET_EN, 0x01);
    ivp_reg_write(IVP_REG_OFF_DSP_CORE_RESET_EN, 0x04);
}


