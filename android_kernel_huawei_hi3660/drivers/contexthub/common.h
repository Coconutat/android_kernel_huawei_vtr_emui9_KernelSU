#ifndef __LINUX_CONTEXTHUB_COMMON_H__
#define __LINUX_CONTEXTHUB_COMMON_H__

#define HM_EN(n)		(0x10001 << (n))
#define HM_DIS(n)		(0x10000 << (n))
#define writel_mask(mask, data, addr)	/*lint -save -e717 */do {writel((((u32)readl(addr)) & (~((u32)(mask)))) | ((data) & (mask)), (addr));} while (0)/*lint -restore */

static inline unsigned int is_bits_clr(unsigned int  mask, volatile void __iomem *addr)
{
    return (((*(volatile unsigned int *) (addr)) & (mask)) == 0);
}

static inline unsigned int is_bits_set(unsigned int  mask, volatile void __iomem *addr)
{
	return (((*(volatile unsigned int*) (addr))&(mask)) == (mask));
}

static inline void set_bits(unsigned int  mask, volatile void __iomem *addr)
{
    (*(volatile unsigned int *) (addr)) |= mask;
}

static inline void clr_bits(unsigned int mask, volatile void __iomem *addr)
{
    (*(volatile unsigned int *) (addr)) &= ~(mask);
}

int get_contexthub_dts_status(void);

#endif

