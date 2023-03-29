

#ifndef __CP_DUMP_AREA_H__
#define __CP_DUMP_AREA_H__

#include <product_config.h>
#include "mntn_interface.h"
#ifndef __ASSEMBLY__
#include "osl_types.h"
#include "osl_list.h"
#include "osl_spinlock.h"
#endif
#include "bsp_memmap.h"
#include "bsp_s_memory.h"
#include "bsp_dump_mem.h"


/*  dump content

    ---------------
   | head          |
    ---------------
   | area map [8]  |
    ---------------
   | area 1        |
    ---------------
   | area 2        |
    ---------------
   | area 3        |
    ---------------
   | area 4        |
    ---------------
   | area 5        |
    ---------------
   | area 6        |
    ---------------
   | area 7        |
    ---------------
   | area 8        |
    ---------------
*/


struct dump_global_area_ctrl_s{
    u32                             ulInitFlag;
    u32                             length;
    struct dump_global_struct_s*    virt_addr;
    unsigned long                   phy_addr;
};

s32 dump_get_area_info(DUMP_AREA_ID areaid,struct dump_area_mntn_addr_info_s* area_info);
s32 dump_area_init(void);
s32 dump_get_global_info(struct dump_global_area_ctrl_s * global_area);
void* dump_get_global_baseinfo(void);



#endif


