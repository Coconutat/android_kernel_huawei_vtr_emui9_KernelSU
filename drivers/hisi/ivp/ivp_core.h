#ifndef _IVP_CORE_H_
#define _IVP_CORE_H_

#include "ivp_platform.h"

struct file_header {
        char name[4];
        char time[20];
        unsigned int image_size;
        unsigned int sect_count;
};
struct image_section_header {
    unsigned short index;
    unsigned char type;
    unsigned char attribute;
    unsigned int offset;
    unsigned int vaddr;
    unsigned int size;
};

extern u32 ivp_reg_read(unsigned int off);
extern void ivp_reg_write(unsigned int off, u32 val);
extern u32 ivp_wdg_reg_read(unsigned int off);
extern void ivp_wdg_reg_write(unsigned int off, u32 val);
extern u32 ivp_smmu_reg_read(unsigned int off);
extern u32 ivp_pctrl_reg_read(unsigned int off);
extern int ivp_change_clk(struct ivp_device *ivp_devp, unsigned int level);
extern bool is_ivp_in_secmode(void);
extern int ivp_ion_phys(struct ion_client *client, struct ion_handle *handle,dma_addr_t *addr);

#endif /* _IVP_CORE_H_ */
