
#include "fusb3601_global.h"

struct fusb3601_chip* get_chip = NULL;  /* Our driver's relevant data */

struct fusb3601_chip* fusb3601_GetChip(void)
{
    /* return a pointer to our structs */
    return get_chip;
}

void fusb3601_SetChip(struct fusb3601_chip* newChip)
{
    /* assign the pointer to our struct */
    get_chip = newChip;
}
