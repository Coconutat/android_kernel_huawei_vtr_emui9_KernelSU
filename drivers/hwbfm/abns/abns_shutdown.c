

#include <chipset_common/bfmr/abns/abns_shutdown.h>

struct abns_word abns_type_map[] = {
    {"ABNS_NORMAL",ABNS_NORMAL},
    {"ABNS_PRESS10S",ABNS_PRESS10S},
    {"ABNS_LTPOWEROFF",ABNS_LTPOWEROFF},
    {"ABNS_PMU_EXP",ABNS_PMU_EXP},
    {"ABNS_PMU_SMPL",ABNS_PMU_SMPL},
    {"ABNS_UNKOWN",ABNS_UNKOWN},
    {"ABNS_BAT_LOW",ABNS_BAT_LOW},
    {"ABNS_BAT_TEMP_HIGH",ABNS_BAT_TEMP_HIGH},
    {"ABNS_BAT_TEMP_LOW",ABNS_BAT_TEMP_LOW},
    {"ABNS_CHIPSETS_TEMP_HIGH",ABNS_CHIPSETS_TEMP_HIGH},
};

static int bfmr_write_abns_flag(unsigned int abns_flag)
{
    int ret = -1;
    bfmr_rrecord_misc_msg_param_t *pparam = NULL;

    pparam = (bfmr_rrecord_misc_msg_param_t *)bfmr_malloc(sizeof(bfmr_rrecord_misc_msg_param_t));
    if (NULL == pparam)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        return -1;
    }
    memset((void *)pparam, 0, sizeof(bfmr_rrecord_misc_msg_param_t));

    ret = bfmr_read_rrecord_misc_msg(pparam);
    if (0 != ret)
    {
        BFMR_PRINT_ERR("bfmr_read_rrecord_misc_msg failed!\n");
        goto __out;
    }

    pparam->abns_flag = abns_flag;
    ret = bfmr_write_rrecord_misc_msg(pparam);
    if (0 != ret)
    {
        BFMR_PRINT_ERR("bfmr_write_rrecord_misc_msg failed!\n");
        goto __out;
    }

__out:
    if (NULL != pparam)
    {
        bfmr_free(pparam);
    }

    return ret;
}


int bfmr_set_abns_flag(unsigned int abns_flag)
{
    return bfmr_write_abns_flag(abns_flag);
}


int bfmr_clean_abns_flag(unsigned int abns_flag)
{
    return bfmr_write_abns_flag(abns_flag);
}
