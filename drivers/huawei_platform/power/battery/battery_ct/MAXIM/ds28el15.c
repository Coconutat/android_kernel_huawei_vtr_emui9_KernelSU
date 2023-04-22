#include "ds28el15.h"

#define HWLOG_TAG HW_ONEWIRE_DS28EL15
HWLOG_REGIST();

enum debug_com_t {
    DEBUG_ROM_ID = 0,
    DEBUG_PERSONALITY,
    DEBUG_GET_EEPROM,
    DEBUG_SET_EEPROM,
    DEBUG_GET_SRAM,
    DEBUG_SET_SRAM,
    DEBUG_GET_MAC,
    DEBUG_EEPROM_STATUS,
    DEBUG_MAX_NUM,
};

/* */
static ds28el15_des ds28el15;

/* random 32 bytes for sram */
static unsigned char *random_bytes = NULL;

static unsigned char mac_datum[MAX_MAC_SOURCE_SIZE];

static ct_ops_reg_list ds28el15_ct_node;

/* number of all kinds of communication errors */
static unsigned int err_num[__MAX_COM_ERR_NUM] = {0};

/* lock for read & write battery safety information */
static DEFINE_MUTEX(batt_safe_info_lock);

/* battery constraints */
static battery_constraint batt_cons = {
    .id_mask            = NULL,
    .id_example         = NULL,
    .id_chk             = NULL,
};

static char sn_printable[PRINTABLE_SN_SIZE];

static unsigned int com_err[DEBUG_MAX_NUM] = {0};

static unsigned int total_com[DEBUG_MAX_NUM] = {0};

static void set_sched_affinity_to_current(void)
{
    long retval;
    int current_cpu;

    preempt_disable();
    current_cpu = smp_processor_id();
    retval = sched_setaffinity(CURRENT_DS28EL15_TASK, cpumask_of(current_cpu));
    preempt_enable();
    if(retval) {
        hwlog_info("Setting cpu affinity to current cpu failed(%ld) in %s.\n", retval, __func__);
    } else {
        hwlog_info("Setting cpu affinity to current cpu(%d) in %s.\n", current_cpu, __func__);
    }
}

static void set_sched_affinity_to_all(void)
{
    long retval;
    cpumask_t dstp;

    cpumask_setall(&dstp);
    retval = sched_setaffinity(CURRENT_DS28EL15_TASK, &dstp);
    if(retval) {
        hwlog_info("Setting cpu affinity to all valid cpus failed(%ld) in %s.\n", retval, __func__);
    } else {
        hwlog_info("Setting cpu affinity to all valid cpus in %s.\n", __func__);
    }
}

static int ds28el15_get_id(const unsigned char **id, unsigned int *id_len)
{
    int ret;
    int i;

    if(id_len) {
        *id_len = ds28el15.ic_des.memory.rom_id_length;
    }

    if(id) {
        set_sched_affinity_to_current();
        for(i = 0; i < GET_ROM_ID_RETRY; i++) {
            if( !(ds28el15.ic_des.memory.validity_rom_id) ) {
                hwlog_info("ds28el15 read rom id communication start...\n");
                ret = ds28el15.ic_des.mem_ops.get_rom_id(&ds28el15.ic_des);
                DS28EL15_COMMUNICATION_INFO(ret, "get_rom_id");
            }
            if(ds28el15.ic_des.memory.validity_rom_id) {
                *id = ds28el15.ic_des.memory.rom_id;
                set_sched_affinity_to_all();
                return DS28EL15_SUCCESS;
            }else{
                err_num[GET_ROM_ID_INDEX]++;
            }
        }
        set_sched_affinity_to_all();
        return DS28EL15_FAIL;
    }

    return DS28EL15_SUCCESS;
}

static int ds28el15_check_ic_status(enum IC_CR *result)
{
    int ret;
    int i;
    maxim_onewire_mem *mom = &ds28el15.ic_des.memory;
    char submem_cr[DS28EL15_INFO_BLOCK_NUM] = {0};

    if(!result) {
        hwlog_err("NULL point(result) found in %s.\n", __func__);
        return DS28EL15_FAIL;
    }

    i = 0;
    while(!(mom->validity_rom_id)) {
        if(!i) {
            set_sched_affinity_to_current();
        }
        hwlog_info("ds28el15 read rom id communication start...\n");
        ret = ds28el15.ic_des.mem_ops.get_rom_id(&ds28el15.ic_des);
        DS28EL15_COMMUNICATION_INFO(ret, "get_rom_id");
        i++;
        if (ret) {
            err_num[GET_ROM_ID_INDEX]++;
        }
        if(i >= GET_ROM_ID_RETRY) {
            break;
        }
    }
    if(i) {
        set_sched_affinity_to_all();
    }
    if(!(mom->validity_rom_id)) {
        hwlog_err("get rom id failed.\n");
        *result = IC_FAIL_UNKOWN;
        return DS28EL15_SUCCESS;
    }

    for(i = 0; i < mom->rom_id_length; i++) {
        batt_cons.id_chk[i] = batt_cons.id_example[i] ^ mom->rom_id[i];
    }
    for(i = 0; i < mom->rom_id_length; i++) {
        batt_cons.id_chk[i] &=  batt_cons.id_mask[i];
    }
    for(i = 0; i < mom->rom_id_length; i++) {
        if(batt_cons.id_chk[i]) {
            *result = IC_FAIL_UNMATCH;
            hwlog_err("IC id was unmatched at %dth byte.\n", i);
            return DS28EL15_SUCCESS;
        }
    }

    for(i = 0; i < GET_BLOCK_STATUS_RETRY; ) {
        if(!(mom->validity_status)) {
            if(!i) {
                set_sched_affinity_to_current();
            }
            hwlog_info("ds28el15 read block status communication start...\n");
            i++;
            ret = ds28el15.ic_des.mem_ops.get_status(&ds28el15.ic_des);
            DS28EL15_COMMUNICATION_INFO(ret, "get_status");
        }
        if(mom->validity_status) {
            break;
        }else{
            err_num[GET_BLOCK_STATUS_INDEX]++;
        }
    }
    if(i) {
        set_sched_affinity_to_all();
    }
    if(!(mom->validity_status)) {
        *result = IC_FAIL_UNKOWN;
        return DS28EL15_SUCCESS;
    }

    mom->block_status[DS28EL15_INFO_BLOCK0] &= DS28EL15_PROTECTION_MASK;
    mom->block_status[DS28EL15_INFO_BLOCK1] &= DS28EL15_PROTECTION_MASK;
    mom->block_status[DS28EL15_BATTERY_VENDOR_BLOCK] &= DS28EL15_PROTECTION_MASK;
    mom->block_status[DS28EL15_PCB_VENDOR_BLOCK] &= DS28EL15_PROTECTION_MASK;

    *result = IC_FAIL_MEM_STATUS;
    if(mom->block_status[DS28EL15_INFO_BLOCK0] != DS28EL15_AUTHENTICATION_PROTECTION &&
       mom->block_status[DS28EL15_INFO_BLOCK0] != DS28EL15_EMPTY_PROTECTION) {
        hwlog_err("Information block0 status was wrong(%02X).\n",
                   ds28el15.ic_des.memory.block_status[DS28EL15_INFO_BLOCK0]);
        submem_cr[0] = DS28EL15_FAIL;
    } else {
        hwlog_info("Information block0 status was %02X.\n",
                   ds28el15.ic_des.memory.block_status[DS28EL15_INFO_BLOCK0]);
    }
    if(mom->block_status[DS28EL15_INFO_BLOCK1] != DS28EL15_AUTHENTICATION_PROTECTION &&
       mom->block_status[DS28EL15_INFO_BLOCK1] != DS28EL15_EMPTY_PROTECTION) {
        hwlog_err("Information block1 status was wrong(%02X).\n",
                   ds28el15.ic_des.memory.block_status[DS28EL15_INFO_BLOCK1]);
        submem_cr[1] = DS28EL15_FAIL;
    } else {
        hwlog_info("Information block1 status was %02X.\n",
               ds28el15.ic_des.memory.block_status[DS28EL15_INFO_BLOCK1]);

    }
    if(mom->block_status[DS28EL15_BATTERY_VENDOR_BLOCK] != DS28EL15_EMPTY_PROTECTION &&
       mom->block_status[DS28EL15_BATTERY_VENDOR_BLOCK] != DS28EL15_WRITE_PROTECTION) {
        hwlog_err("Battery vendor block status was wrong(%02X).\n",
                   ds28el15.ic_des.memory.block_status[DS28EL15_BATTERY_VENDOR_BLOCK]);
        submem_cr[2] = DS28EL15_FAIL;
    } else {
        hwlog_info("Battery vendor block status was %02X.\n",
                   ds28el15.ic_des.memory.block_status[DS28EL15_BATTERY_VENDOR_BLOCK]);
    }
    if(mom->block_status[DS28EL15_PCB_VENDOR_BLOCK] != DS28EL15_EMPTY_PROTECTION &&
       mom->block_status[DS28EL15_PCB_VENDOR_BLOCK] != DS28EL15_WRITE_PROTECTION) {
        hwlog_err("pcb vendor block status was %02X.\n",
                   ds28el15.ic_des.memory.block_status[DS28EL15_PCB_VENDOR_BLOCK]);
        submem_cr[3] = DS28EL15_FAIL;
    } else {
        hwlog_info("pcb vendor block status was %02X.\n",
                   ds28el15.ic_des.memory.block_status[DS28EL15_PCB_VENDOR_BLOCK]);

    }

    for(i = 0; i < DS28EL15_INFO_BLOCK_NUM; i++) {
        if(submem_cr[i] == DS28EL15_FAIL) {
            *result = IC_FAIL_MEM_STATUS;
            return DS28EL15_SUCCESS;
        }
    }
    *result = IC_PASS;
    return DS28EL15_SUCCESS;
}

static int ds28el15_certification(void *data, int data_len, enum KEY_CR *result, int type)
{
    int ret;
    int i;
    int j;
    char mac_prt[65] = {0};
    int page;

    if(!result) {
        hwlog_err("NULL point(result) found in %s.\n", __func__);
        return DS28EL15_FAIL;
    }

    if(!data) {
        hwlog_err("NULL point(data) found in %s.\n", __func__);
        return DS28EL15_FAIL;
    }

    if(data_len != ds28el15.ic_des.memory.mac_length) {
        hwlog_err("certification data length(%d) not correct.\n", data_len);
        return DS28EL15_FAIL;
    }

    switch (type) {
    case DS28EL15_CT_MAC_PAGE0:
        page = MAXIM_PAGE0;
        break;
    case DS28EL15_CT_MAC_PAGE1:
        page = MAXIM_PAGE1;
        break;
    default:
        page = MAXIM_PAGE1;
        hwlog_err("illegal certification type, use page1 default.\n");
        break;
    }

    set_sched_affinity_to_current();
    for(j = 0; j < GET_MAC_RETRY; j++) {
        if(!(ds28el15.ic_des.memory.validity_mac)) {
            for(i = 0; i < SET_SRAM_RETRY; i++) {
                if(!(ds28el15.ic_des.memory.validity_sram)) {
                    ret = ds28el15.ic_des.mem_ops.set_sram(&ds28el15.ic_des, random_bytes);
                    DS28EL15_COMMUNICATION_INFO(ret,"set_sram");
                }
                if(ds28el15.ic_des.memory.validity_sram) {
                    break;
                }else{
                    err_num[SET_SRAM_INDEX]++;
                }
            }

            if(!(ds28el15.ic_des.memory.validity_sram)) {
                hwlog_info("Set random bytes for mac failed(%d@%d) in %s.",
                           j, GET_MAC_RETRY, __func__);
			    msleep(200);
                continue;
            }

            ret = ds28el15.ic_des.mem_ops.get_mac(&ds28el15.ic_des, !ANONYMOUS_MODE, page);
            DS28EL15_COMMUNICATION_INFO(ret,"get_mac");
        }

        if(ds28el15.ic_des.memory.validity_mac) {
            for(i = 0; i < data_len; i++) {
                snprintf(mac_prt + 2*i, 3, "%02x", ds28el15.ic_des.memory.mac[i]);
            }
            hwlog_info("remote mac is %s.\n", mac_prt);
            for(i = 0; i < data_len; i++){
                snprintf(mac_prt + 2*i, 3, "%02x", ((char *)data)[i]);
            }
            hwlog_info("local mac is %s.\n", mac_prt);
            if(memcmp(ds28el15.ic_des.memory.mac, data, data_len)) {
                *result = KEY_FAIL_UNMATCH;
            } else {
                *result = KEY_PASS;
            }
            set_sched_affinity_to_all();
            return DS28EL15_SUCCESS;
        } else {
            err_num[GET_MAC_INDEX]++;
        }
    }
    set_sched_affinity_to_all();
    *result = KEY_FAIL_TIMEOUT;
    return DS28EL15_SUCCESS;
}

static int ds28el15_get_sn(const unsigned char **sn, unsigned int *sn_size,
                                  unsigned char *sn_version)
{
    int ret;
    int i;
    char hex_print;
    char * sn_to_print;

    if(sn || sn_version) {
        if(ds28el15.ic_des.memory.sn_page >= ds28el15.ic_des.memory.page_number) {
            return DS28EL15_FAIL;
        }
        set_sched_affinity_to_current();
        for(i = 0; i < GET_USER_MEMORY_RETRY; i++) {
            if( !ds28el15.ic_des.memory.validity_page[ds28el15.ic_des.memory.sn_page] ) {
                ret = ds28el15.ic_des.mem_ops.get_user_memory(&ds28el15.ic_des,
                                                              ds28el15.ic_des.memory.sn_page,
                                                              0, MAXIM_PAGE_SEGMENT_NUM);
                DS28EL15_COMMUNICATION_INFO(ret, "get_user_memory");
            }
            if(ds28el15.ic_des.memory.validity_page[ds28el15.ic_des.memory.sn_page]) {
                break;
            }else{
                err_num[GET_USER_MEMORY_INDEX]++;
            }
        }
        if( !ds28el15.ic_des.memory.validity_page[ds28el15.ic_des.memory.sn_page] ) {
            hwlog_err("Get mac stopped for getting user memory page0 failed in %s.", __func__);
            goto get_sn_err;
        }
        set_sched_affinity_to_all();
        if(sn_version) {
            *sn_version = ds28el15.ic_des.memory.user_memory[DS28EL15_SN_VERSION_INDEX];
        }
        if(sn) {
            sn_to_print = ds28el15.ic_des.memory.user_memory;
            sn_to_print += ds28el15.ic_des.memory.sn_offset_bits/BYTEBITS;
            sn_to_print += ds28el15.ic_des.memory.sn_page * MAXIM_PAGE_SIZE;
            for(i = 0; i < SN_CHAR_PRINT_SIZE; i++) {
                sn_printable[i] = sn_to_print[i];
            }
            sn_to_print += SN_CHAR_PRINT_SIZE;
            for(i = 0; i < SN_HEX_PRINT_SIZE; i++) {
                if(IS_ODD(i)) {
                    hex_print = (sn_to_print[i/2] & 0x0f);
                }else {
                    hex_print = ((sn_to_print[i/2] & 0xf0) >> 4) & 0x0f;
                }
                sprintf(sn_printable+i+SN_CHAR_PRINT_SIZE,"%X",hex_print);
            }
            *sn = sn_printable;
        }
    }
    if(sn_size) {
        *sn_size = PRINTABLE_SN_SIZE - 1;
    }
    return DS28EL15_SUCCESS;

get_sn_err:
    set_sched_affinity_to_all();
    return DS28EL15_FAIL;
}

static int ds28el15_get_ct_src(mac_resource *res, unsigned int type)
{
    int ret;
    int i;
    int page;
    int page_offset;

    if(!res) {
        hwlog_err("Mac resource should not be NULL found in %s.\n", __func__);
	return DS28EL15_FAIL;
    }

    switch (type) {
    case DS28EL15_CT_MAC_PAGE0:
    case DS28EL15_CT_MAC_PAGE1:
        page = (type == DS28EL15_CT_MAC_PAGE0) ? MAXIM_PAGE0 : MAXIM_PAGE1;
        page_offset = (type == DS28EL15_CT_MAC_PAGE0) ? 0 : ds28el15.ic_des.memory.page_size;
        set_sched_affinity_to_current();
        for(i = 0; i < GET_ROM_ID_RETRY; i++) {
            if( !(ds28el15.ic_des.memory.validity_rom_id) ) {
                ret = ds28el15.ic_des.mem_ops.get_rom_id(&ds28el15.ic_des);
                DS28EL15_COMMUNICATION_INFO(ret, "get_rom_id");
            }
            if(ds28el15.ic_des.memory.validity_rom_id) {
                break;
            }else{
                err_num[GET_ROM_ID_INDEX]++;
            }
        }
        if( !(ds28el15.ic_des.memory.validity_rom_id) ) {
            hwlog_err("Get mac stopped for getting rom id failed in %s.", __func__);
            goto get_ct_src_fatal_err;
        }

        for(i = 0; i < GET_PERSONALITY_RETRY; i++) {
            if( !(ds28el15.ic_des.memory.validity_personality) ) {
                ret = ds28el15.ic_des.mem_ops.get_personality(&ds28el15.ic_des);
                DS28EL15_COMMUNICATION_INFO(ret, "get_personality");
            }
            if(ds28el15.ic_des.memory.validity_personality) {
                break;
            }else{
                err_num[GET_PERSONALITY_INDEX]++;
            }
        }
        if( !ds28el15.ic_des.memory.validity_personality ) {
            hwlog_err("Get mac stopped for getting personality failed in %s.", __func__);
            goto get_ct_src_fatal_err;
        }

        for(i = 0; i < GET_USER_MEMORY_RETRY; i++) {
            if( !ds28el15.ic_des.memory.validity_page[page] ) {
                ret = ds28el15.ic_des.mem_ops.get_user_memory(&ds28el15.ic_des, page,
                                                              0, MAXIM_PAGE_SEGMENT_NUM);
                DS28EL15_COMMUNICATION_INFO(ret, "get_user_memory");
            }
            if(ds28el15.ic_des.memory.validity_page[page]) {
                break;
            }else{
                err_num[GET_USER_MEMORY_INDEX]++;
            }
        }
        if( !ds28el15.ic_des.memory.validity_page[page] ) {
            hwlog_err("Get mac stopped for getting user memory page0 failed in %s.", __func__);
            goto get_ct_src_fatal_err;
        }
        set_sched_affinity_to_all();

        memset(mac_datum, 0, MAX_MAC_SOURCE_SIZE);
        memcpy(mac_datum + AUTH_MAC_ROM_ID_OFFSET, ds28el15.ic_des.memory.rom_id,
               ds28el15.ic_des.memory.rom_id_length);
        memcpy(mac_datum + AUTH_MAC_PAGE_OFFSET, ds28el15.ic_des.memory.user_memory + page_offset,
               ds28el15.ic_des.memory.page_size);
        memcpy(mac_datum + AUTH_MAC_SRAM_OFFSET, random_bytes,
               ds28el15.ic_des.memory.sram_length);
        mac_datum[AUTH_MAC_PAGE_NUM_OFFSET] = page;
        memcpy(mac_datum + AUTH_MAC_MAN_ID_OFFSET,
               ds28el15.ic_des.memory.personality + MAXIM_MAN_ID_OFFSET, MAXIM_MAN_ID_SIZE);
        res->datum = mac_datum;
        res->len = DS28EL15_CT_MAC_SIZE;
        res->ic_type = DS28EL15_MAC_RES;
        return DS28EL15_SUCCESS;
    default:
        hwlog_err("Wrong mac resource type(%ud) requred in %s.", type, __func__);
        break;
    }

    return DS28EL15_FAIL;

get_ct_src_fatal_err:
    set_sched_affinity_to_all();
    return DS28EL15_FAIL;
}

static int ds28el15_set_batt_safe_info(batt_safe_info_type type, void *value)
{
    int i;
    maxim_onewire_mem *mom = &ds28el15.ic_des.memory;
    int ret_val = DS28EL15_FAIL;
    int ret;

    mutex_lock(&batt_safe_info_lock);
    wake_lock_timeout(&ds28el15.write_lock, 5*HZ);
    switch (type)
    {
    case BATT_MATCH_ABILITY:
        if(*(enum BATT_MATCH_TYPE *)value == BATTERY_REMATCHABLE) {
            break;
        }
        if((mom->validity_status) &&
           (mom->block_status[DS28EL15_BATTERY_VENDOR_BLOCK] & DS28EL15_WRITE_PROTECTION)) {
            ret_val = DS28EL15_SUCCESS;
            hwlog_info("ds28el15 was already write protection.\n");
            break;
        }
        hwlog_info("ds28el15 is going to set battery to write protection.\n");
        set_sched_affinity_to_current();
        for(i = 0; i < SET_BLOCK_STATUS_RETRY; i++) {
            ret = ds28el15.ic_des.mem_ops.set_status(&ds28el15.ic_des,
                                                     DS28EL15_BATTERY_VENDOR_BLOCK,
                                                     DS28EL15_WRITE_PROTECTION);
            DS28EL15_COMMUNICATION_INFO(ret, "set_status");
            if(mom->validity_status) {
                mom->block_status[DS28EL15_BATTERY_VENDOR_BLOCK] |= DS28EL15_WRITE_PROTECTION;
                ret_val = DS28EL15_SUCCESS;
                break;
            } else {
                err_num[SET_BLOCK_STATUS_INDEX]++;
            }
        }
        set_sched_affinity_to_all();
        break;
    default:
        hwlog_err("unsupported battery safety information type found in %s.\n", __func__);
        break;
    }
    wake_unlock(&ds28el15.write_lock);
    mutex_unlock(&batt_safe_info_lock);

    return ret_val;
}

static int ds28el15_get_batt_safe_info(batt_safe_info_type type, void *value)
{
    maxim_onewire_mem *mom = &ds28el15.ic_des.memory;
    int retval = DS28EL15_FAIL;
    int i, ret;

    mutex_lock(&batt_safe_info_lock);
    switch (type)
    {
    case BATT_MATCH_ABILITY:
        for(i = 0; i < GET_BLOCK_STATUS_RETRY; ) {
            if(!(mom->validity_status)) {
                if(!i) {
                    set_sched_affinity_to_current();
                }
                hwlog_info("ds28el15 read block status communication start...\n");
                i++;
                ret = ds28el15.ic_des.mem_ops.get_status(&ds28el15.ic_des);
                DS28EL15_COMMUNICATION_INFO(ret, "get_status");
            }
            if(mom->validity_status) {
                break;
            }else{
                err_num[GET_BLOCK_STATUS_INDEX]++;
            }
        }
        if(i) {
            set_sched_affinity_to_all();
        }
        if(mom->block_status[DS28EL15_BATTERY_VENDOR_BLOCK] & DS28EL15_WRITE_PROTECTION) {
            *(enum BATT_MATCH_TYPE *)value = BATTERY_UNREMATCHABLE;
        } else {
            *(enum BATT_MATCH_TYPE *)value = BATTERY_REMATCHABLE;
        }
        retval = DS28EL15_SUCCESS;
        break;
    default:
        hwlog_err("unsupported battery safety information type found in %s.\n", __func__);
        break;
    }
    mutex_unlock(&batt_safe_info_lock);

    return retval;
}

static batt_ic_type ds28el15_get_ic_type(void)
{
    return MAXIM_SHA256_TYPE;
}

#ifdef ONEWIRE_STABILITY_DEBUG

static ssize_t get_rom_id_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    if(ds28el15.ic_des.mem_ops.get_rom_id(&ds28el15.ic_des)) {
        com_err[DEBUG_ROM_ID]++;
    }
    total_com[DEBUG_ROM_ID]++;
    return snprintf(buf, PAGE_SIZE, "%d@%d", com_err[DEBUG_ROM_ID], total_com[DEBUG_ROM_ID]);
}

static ssize_t get_personality_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    if(ds28el15.ic_des.mem_ops.get_personality(&ds28el15.ic_des)) {
        com_err[DEBUG_PERSONALITY]++;
    }
    total_com[DEBUG_PERSONALITY]++;
    return snprintf(buf, PAGE_SIZE, "%d@%d",
                    com_err[DEBUG_PERSONALITY], total_com[DEBUG_PERSONALITY]);
}

static ssize_t get_eeprom_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    if(ds28el15.ic_des.mem_ops.get_user_memory(&ds28el15.ic_des, MAXIM_PAGE0,
                                               0, MAXIM_PAGE_SEGMENT_NUM)) {
        com_err[DEBUG_GET_EEPROM]++;
    }
    total_com[DEBUG_GET_EEPROM]++;
    return snprintf(buf, PAGE_SIZE, "%d@%d",
                    com_err[DEBUG_GET_EEPROM], total_com[DEBUG_GET_EEPROM]);
}

static ssize_t set_eeprom_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    if(ds28el15.ic_des.mem_ops.set_user_memory(&ds28el15.ic_des, "1234",
                                               MAXIM_PAGE0, MAXIM_SEGMENT0)) {
        com_err[DEBUG_SET_EEPROM]++;
    }
    total_com[DEBUG_SET_EEPROM]++;
    return snprintf(buf, PAGE_SIZE, "%d@%d",
                    com_err[DEBUG_SET_EEPROM], total_com[DEBUG_SET_EEPROM]);
}

static ssize_t get_sram_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    if(ds28el15.ic_des.mem_ops.get_sram(&ds28el15.ic_des)) {
        com_err[DEBUG_GET_SRAM]++;
    }
    total_com[DEBUG_GET_SRAM]++;
    return snprintf(buf, PAGE_SIZE, "%d@%d", com_err[DEBUG_GET_SRAM], total_com[DEBUG_GET_SRAM]);
}

static ssize_t set_sram_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    if(ds28el15.ic_des.mem_ops.set_sram(&ds28el15.ic_des, random_bytes)) {
        com_err[DEBUG_SET_SRAM]++;
    }
    total_com[DEBUG_SET_SRAM]++;
    return snprintf(buf, PAGE_SIZE, "%d@%d", com_err[DEBUG_SET_SRAM], total_com[DEBUG_SET_SRAM]);
}

static ssize_t get_mac_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    if(ds28el15.ic_des.mem_ops.get_mac(&ds28el15.ic_des, !ANONYMOUS_MODE, MAXIM_PAGE0)) {
        com_err[DEBUG_GET_MAC]++;
    }
    total_com[DEBUG_GET_MAC]++;
    return snprintf(buf, PAGE_SIZE, "%d@%d", com_err[DEBUG_GET_MAC], total_com[DEBUG_GET_MAC]);
}

static ssize_t eeprom_status_show(struct device *dev, struct device_attribute *attr,
                                          char *buf)
{
    if(ds28el15.ic_des.mem_ops.get_status(&ds28el15.ic_des)) {
        com_err[DEBUG_EEPROM_STATUS]++;
    }
    total_com[DEBUG_EEPROM_STATUS]++;
    return snprintf(buf, PAGE_SIZE, "%d@%d",
                    com_err[DEBUG_EEPROM_STATUS], total_com[DEBUG_EEPROM_STATUS]);
}

static DEVICE_ATTR_RO(get_rom_id);
static DEVICE_ATTR_RO(get_personality);
static DEVICE_ATTR_RO(get_eeprom);
static DEVICE_ATTR_RO(set_eeprom);
static DEVICE_ATTR_RO(get_sram);
static DEVICE_ATTR_RO(set_sram);
static DEVICE_ATTR_RO(get_mac);
static DEVICE_ATTR_RO(eeprom_status);

#endif

static const char *err_str[] = {
    [SET_SRAM_INDEX] = "SET_SRAM",
    [GET_USER_MEMORY_INDEX] = "GET_USER_MEMORY",
    [GET_BLOCK_STATUS_INDEX] = "GET_BLOCK_STATUS",
    [SET_BLOCK_STATUS_INDEX] = "SET_BLOCK_STATUS",
    [GET_PERSONALITY_INDEX] = "GET_PERSONALITY",
    [GET_ROM_ID_INDEX] = "GET_ROM_ID",
    [GET_MAC_INDEX] = "GET_MAC",
};

static ssize_t err_num_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int val = 0;
    int i;
    for( i = 0; i < __MAX_COM_ERR_NUM; i++) {
        val += snprintf(buf + val, PAGE_SIZE, "%s failed:%d times.\n", err_str[i], err_num[i]);
    }
    return val;
}

static DEVICE_ATTR_RO(err_num);

static const struct attribute *ds28el15_attrs[] = {
#ifdef ONEWIRE_STABILITY_DEBUG
    &dev_attr_get_rom_id.attr,
    &dev_attr_get_personality.attr,
    &dev_attr_get_eeprom.attr,
    &dev_attr_set_eeprom.attr,
    &dev_attr_get_sram.attr,
    &dev_attr_set_sram.attr,
    &dev_attr_get_mac.attr,
    &dev_attr_eeprom_status.attr,
#endif
    &dev_attr_err_num.attr,
    NULL, /* sysfs_create_files need last one be NULL */
};

int ds28el15_ct_ops_register(batt_ct_ops *bco)
{
    int ret = DS28EL15_FAIL;

    if(ds28el15.ic_des.mem_ops.valid_mem_ops) {
        ret = ds28el15.ic_des.mem_ops.valid_mem_ops(&ds28el15.ic_des, ds28el15.pdev);
    } else {
        hwlog_err("ds28el15 is not valid for certification!\n");
    }
    if(!ret) {
        get_random_bytes(random_bytes, ds28el15.ic_des.memory.sram_length);
        bco->get_ic_type        = ds28el15_get_ic_type;
        bco->get_ic_id          = ds28el15_get_id;
        bco->get_batt_sn        = ds28el15_get_sn;
        bco->check_ic_status    = ds28el15_check_ic_status;
        bco->certification      = ds28el15_certification;
        bco->get_ct_src         = ds28el15_get_ct_src;
        bco->set_batt_safe_info = ds28el15_set_batt_safe_info;
        bco->get_batt_safe_info = ds28el15_get_batt_safe_info;
    }

    return ret;
}

/* Battery constraints initialization */
static int batt_cons_init(struct platform_device *pdev)
{
    int ret;

    /* Allocate memory for battery constraint infomation */
    batt_cons.id_mask = devm_kzalloc(&pdev->dev, ds28el15.ic_des.memory.rom_id_length, GFP_KERNEL);
    DS28EL15_NULL_POINT_RETURN(batt_cons.id_mask);
    batt_cons.id_example = devm_kzalloc(&pdev->dev, ds28el15.ic_des.memory.rom_id_length, GFP_KERNEL);
    DS28EL15_NULL_POINT_RETURN(batt_cons.id_example);
    batt_cons.id_chk = devm_kzalloc(&pdev->dev, ds28el15.ic_des.memory.rom_id_length, GFP_KERNEL);
    DS28EL15_NULL_POINT_RETURN(batt_cons.id_chk);

    /* Get battery id mask & id example */
    ret = of_property_read_u8_array(pdev->dev.of_node, "id-mask", batt_cons.id_mask,
                                    ds28el15.ic_des.memory.rom_id_length);
    DS28EL15_DTS_READ_ERROR_RETURN(ret, "id-mask");
    ret = of_property_read_u8_array(pdev->dev.of_node, "id-example", batt_cons.id_example,
                                    ds28el15.ic_des.memory.rom_id_length);
    DS28EL15_DTS_READ_ERROR_RETURN(ret, "id-example");

    return DS28EL15_SUCCESS;
}

static int dev_sys_node_init(struct platform_device *pdev)
{
    int ret;

    ds28el15.attr_group = devm_kzalloc(&pdev->dev, sizeof(struct attribute_group), GFP_KERNEL);
    DS28EL15_NULL_POINT_RETURN(ds28el15.attr_group);
    ds28el15.attr_groups = devm_kzalloc(&pdev->dev, 2*sizeof(struct attribute_group *), GFP_KERNEL);
    DS28EL15_NULL_POINT_RETURN(ds28el15.attr_groups);

    //ret = of_property_read_string(pdev->dev.of_node, "name", &ds28el15.attr_group->name);
    //DS28EL15_DTS_READ_ERROR_RETURN(ret, "name");
    ds28el15.attr_group->attrs = ds28el15_attrs;

    ds28el15.attr_groups[0] = ds28el15.attr_group;
    ds28el15.attr_groups[1] = NULL;

    if(sysfs_create_groups(&pdev->dev.kobj, ds28el15.attr_groups)) {
        return -1;
    }
    return 0;
}

static int ds28el15_driver_probe(struct platform_device *pdev)
{
    int ret;
    struct device_node *batt_ic_np;
    maxim_onewire_mem *mom;


    hwlog_info("ds28el15: probing...\n");

    /* record ds28el15 platform_device */
    ds28el15.pdev = pdev;
    batt_ic_np = pdev->dev.of_node;
    wake_lock_init(&ds28el15.write_lock, WAKE_LOCK_SUSPEND, pdev->name);

    /* ds28el15--maxim onewire ic and use gpio as communication controller*/
    if(maxim_onewire_register(&ds28el15.ic_des, ds28el15.pdev)) {
        hwlog_err("ds28el15 registeration failed in %s.\n", __func__);
        goto ds28el15_fail_0;
    }


    if(batt_cons_init(pdev)) {
        hwlog_err("ds28el15 constraint init failed in %s.\n", __func__);
        goto ds28el15_fail_0;
    }

	if(dev_sys_node_init(pdev)){
		hwlog_err("dev_sys_node_init failed in %s.\n", __func__);
        goto ds28el15_fail_0;
	}


    /* get random 32 bytes */
    random_bytes = devm_kmalloc(&pdev->dev, ds28el15.ic_des.memory.sram_length, GFP_KERNEL);
    if(!random_bytes) {
        hwlog_err("Kalloc for random bytes failed in %s.\n", __func__);
        goto ds28el15_fail_0;
    }

    mom = &ds28el15.ic_des.memory;
    mom->sn_length_bits = SN_LENGTH_BITS;
    ret = of_property_read_u32(batt_ic_np, "sn-offset-bits", &(mom->sn_offset_bits));
    if(ret) {
        hwlog_err("Read battery SN offset in bits failed in %s.", __func__);
        goto ds28el15_fail_0;
    } else if(NOT_MUTI8(mom->sn_offset_bits)) {
        hwlog_err("Illegal SN offset(%u) found in %s.", mom->sn_offset_bits, __func__);
        goto ds28el15_fail_0;
    }
    if(mom->sn_length_bits > BYTES2BITS(mom->page_size) ||
       mom->sn_offset_bits > BYTES2BITS(mom->page_size) ||
       mom->sn_length_bits + mom->sn_offset_bits > BYTES2BITS(mom->page_size)) {
        hwlog_err("Battery SN length(%u) or SN offset(%u) is illegal combination found in %s.\n",
                  mom->sn_length_bits, mom->sn_offset_bits, __func__);
    }
    ret = of_property_read_u32(batt_ic_np, "sn-page", &(mom->sn_page));
    if(ret || mom->sn_page >= mom->page_number) {
        hwlog_err("Illegal battery SN page(%u) readed by %s.\n", mom->sn_page, __func__);
    }

    /* add ds28el15_ct_ops_register to ct_ops_reg_list */
    ds28el15_ct_node.ic_memory_release = NULL;
    ds28el15_ct_node.ct_ops_register = ds28el15_ct_ops_register;
    list_add_tail(&ds28el15_ct_node.node, &batt_ct_head);

    hwlog_info("ds28el15: probing success.\n");
    return DS28EL15_SUCCESS;

ds28el15_fail_0:
	wake_lock_destroy(&ds28el15.write_lock);
	return DS28EL15_FAIL;
}

static int  ds28el15_driver_remove(struct platform_device *pdev)
{
    ds28el15.pdev = NULL;
	wake_lock_destroy(&ds28el15.write_lock);
    return DS28EL15_SUCCESS;
}

static struct of_device_id ds28el15_match_table[] = {
    {
        .name = "ds28el15",
        .compatible = "maxim,onewire-sha",
    },
    { /*end*/},
};

static struct platform_driver ds28el15_driver = {
    .probe		= ds28el15_driver_probe,
    .remove		= ds28el15_driver_remove,
    .driver		= {
        .name = "ds28el15",
        .owner = THIS_MODULE,
        .of_match_table = ds28el15_match_table,
    },
};

int __init ds28el15_driver_init(void)
{
    hwlog_info("ds28el15 driver init...\n");
    return platform_driver_register(&ds28el15_driver);
}

void __exit ds28el15_driver_exit(void)
{
    hwlog_info("ds28el15 driver exit...\n");
    platform_driver_unregister(&ds28el15_driver);
}

subsys_initcall_sync(ds28el15_driver_init);
module_exit(ds28el15_driver_exit);

MODULE_LICENSE("GPL");
