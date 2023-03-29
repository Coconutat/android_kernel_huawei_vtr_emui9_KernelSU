#include "maxim_onewire.h"

#define HWLOG_TAG HW_ONEWIRE_MAXIM
HWLOG_REGIST();

/* Nerver change dscrc_table for CRC8, which is maxim special. */
static unsigned char dscrc_table[] = {
    0, 94, 188, 226, 97, 63, 221, 131, 194, 156, 126, 32, 163, 253, 31, 65,
    157, 195, 33, 127, 252, 162, 64, 30, 95,  1, 227, 189, 62, 96, 130, 220,
    35, 125, 159, 193, 66, 28, 254, 160, 225, 191, 93,  3, 128, 222, 60, 98,
    190, 224,  2, 92, 223, 129, 99, 61, 124, 34, 192, 158, 29, 67, 161, 255,
    70, 24, 250, 164, 39, 121, 155, 197, 132, 218, 56, 102, 229, 187, 89,  7,
    219, 133, 103, 57, 186, 228,  6, 88, 25, 71, 165, 251, 120, 38, 196, 154,
    101, 59, 217, 135,  4, 90, 184, 230, 167, 249, 27, 69, 198, 152, 122, 36,
    248, 166, 68, 26, 153, 199, 37, 123, 58, 100, 134, 216, 91,  5, 231, 185,
    140, 210, 48, 110, 237, 179, 81, 15, 78, 16, 242, 172, 47, 113, 147, 205,
    17, 79, 173, 243, 112, 46, 204, 146, 211, 141, 111, 49, 178, 236, 14, 80,
    175, 241, 19, 77, 206, 144, 114, 44, 109, 51, 209, 143, 12, 82, 176, 238,
    50, 108, 142, 208, 83, 13, 239, 177, 240, 174, 76, 18, 145, 207, 45, 115,
    202, 148, 118, 40, 171, 245, 23, 73,  8, 86, 180, 234, 105, 55, 213, 139,
    87,  9, 235, 181, 54, 104, 138, 212, 149, 203, 41, 119, 244, 170, 72, 22,
    233, 183, 85, 11, 136, 214, 52, 106, 43, 117, 151, 201, 74, 20, 246, 168,
    116, 42, 200, 150, 21, 75, 169, 247, 182, 232, 10, 84, 215, 137, 107, 53
};

/* Nerver change oddparity for CRC16, which is maxim special. */
short oddparity[16] = { 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0 };

/*
 *CRC8 Check
 *@data: data to check
 *@length: data length
 */
static unsigned char check_crc8(unsigned char *data, int length)
{
    unsigned char crc8 = 0;
    int i;
    for(i = 0; i < length; i++) {
        crc8 = dscrc_table[crc8 ^ data[i]];
    }
    return crc8;
}

/*
 *CRC16 Check
 *@check_data: data to check
 *@length: data length
 *!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *!!!!Nerver change digits inside this function!!!!
 *!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */
static unsigned short check_crc16(unsigned char *check_data, int length)
{
    unsigned short crc16 = 0;
    unsigned short datatemp;
    int i;
    for (i = 0; i < length; i++) {
        datatemp = check_data[i];
        datatemp = (datatemp ^ (crc16 & 0xff)) & 0xff;
        crc16 >>= 8;
        if (oddparity[datatemp & 0xf] ^ oddparity[datatemp >> 4]) {
            crc16 ^= 0xc001;
        }
        datatemp <<= 6;
        crc16 ^= datatemp;
        datatemp <<= 1;
        crc16 ^= datatemp;
    }
    return crc16;
}

/*
 *Read the 64 bits ROM of DS28E15
 *@rom: 64Bits Rom Receiving buffer. User should make sure rom has 8 bytes space
 */
static unsigned char onewire_read_rom(unsigned char *rom, onewire_phy_ops *phy_ops)
{
    unsigned char i;
    unsigned char crc8;

    /* Reset */
    if(NO_SLAVE_RESPONSE(phy_ops->reset())) {
        hwlog_err("1-wire maxim: no slave device response in %s", __func__);
        return MAXIM_ONEWIRE_NO_SLAVE;
    }

    /* Read ROM */
    phy_ops->write_byte(READ_ROM);

    /*
     *Read 8 bytes ROM = 8-bit family code, unique 48-bit serial number, and
     *8-bit CRC
     */
    for(i = 0; i < MAXIM_ROM_SIZE; i++) {
        rom[i] = phy_ops->read_byte();
    }

    /* CRC8 Check */
    crc8 = check_crc8(rom, MAXIM_ROM_SIZE);
    CRC8_ERROR_PROCESS(crc8);

    /* Reset */
    phy_ops->reset();

    return MAXIM_ONEWIRE_SUCCESS;
}

/*
 *Write DS28E15 EEPROM Data to specified segment in specified page.
 *@segment: specify the location within the selected memory page at which the
 *writing begins.
 *@page: specifies the memory page at which the writing begins.Valid memory page
 *numbers are 0 (page 0) and 1 (page 1).
 *@sendbuff: send data buffer, user should make sure that it has 4 bytes space.
 */
static unsigned char onewire_write_memory(unsigned char segment,
                                                   unsigned char page,
                                                   unsigned char *sendbuff,
                                                   onewire_phy_ops *phy_ops,
                                                   maxim_time_request *mtrq)
{
    unsigned char buf[64]; //only 0-33 used
    unsigned char bufcnt = 0;
    unsigned char i;
    unsigned char cr;

    ONEWIRE_NOTIFY_ALL_SLAVES(phy_ops);

    /* write memory command */
    buf[bufcnt++] = WRITE_MEMORY;

    /* write memory parameter */
    buf[bufcnt++] = ((segment & SEGMENT_MASK) << SEGMENT_OFFSET) | page;

    /* send write memory command & parameter */
    for (i = 0; i < bufcnt; i++) {
        phy_ops->write_byte(buf[i]);
    }

    /* Receive CRC16 */
    buf[bufcnt++] = phy_ops->read_byte();
    buf[bufcnt++] = phy_ops->read_byte();
    DO_CRC16(buf, bufcnt);

    /*
     *DATA START:
     *Send EEPROM Data & only support 1 segment now
     */
    for (bufcnt = 0; bufcnt < MAXIM_SEGMENT_SIZE ; bufcnt++) {
        phy_ops->write_byte(sendbuff[bufcnt]);
        buf[bufcnt] = sendbuff[bufcnt];
    }

    /* Receive CRC */
    buf[bufcnt++] = phy_ops->read_byte();
    buf[bufcnt++] = phy_ops->read_byte();
    DO_CRC16(buf, bufcnt);

    /* Send Release Byte */
    phy_ops->write_byte(ONEWIRE_RELEASE);
    /* Wait for eeprom programming done */
    phy_ops->wait_for_ic(mtrq->program_ms);
    /* Get command success indicator*/
    cr = phy_ops->read_byte();
    MAXIM_COMMAND_FAIL_PROCESS(cr);
    /*
     *DATA END:
     *1 segment end here & loop from DATA START to write multi segment
     */

    /* Reset */
    phy_ops->reset();

    return MAXIM_ONEWIRE_SUCCESS;
}


/*
 *Reading  DS28E15 EEPROM Data [seg_start, seg_end)
 *in Specified Page.
 *@segment: specify the location within the selected memory page at which the
 *reading begins.
 *@page: specifies the memory page at which the reading begins.Valid memory page
 *numbers are 0 (page 0) and 1 (page 1).
 *@receiver: read data buffer, user should make sure that it has 32 bytes space.
 */
static unsigned char onewire_read_memory(unsigned char seg_start, unsigned char seg_end,
                unsigned char page, unsigned char *receiver, onewire_phy_ops *phy_ops)
{
    unsigned char buf[64]; //only 0-33 used
    unsigned char bufcnt = 0;
    unsigned char i;

    ONEWIRE_NOTIFY_ALL_SLAVES(phy_ops);

    /* read memory command */
    buf[bufcnt++] = READ_MEMORY;

    /* read memory parameter */
    buf[bufcnt++] = ((seg_start & SEGMENT_MASK) << SEGMENT_OFFSET) | page;

    /* send read memory command & parameter */
    for (i = 0; i < bufcnt; i++) {
        phy_ops->write_byte(buf[i]);
    }

    /* Receive CRC16 */
    buf[bufcnt++] = phy_ops->read_byte();
    buf[bufcnt++] = phy_ops->read_byte();
    DO_CRC16(buf, bufcnt);

    /* Receive EEPROM Data */
    for(bufcnt = 0; bufcnt < MAXIM_SEGMENTS2BYTES(seg_end - seg_start); bufcnt++) {
        receiver[bufcnt] = phy_ops->read_byte();
        buf[bufcnt] = receiver[bufcnt];
    }

    /* Receive CRC */
    buf[bufcnt++] = phy_ops->read_byte();
    buf[bufcnt++] = phy_ops->read_byte();
    DO_CRC16(buf, bufcnt);

    phy_ops->reset();

    return MAXIM_ONEWIRE_SUCCESS;
}

/*
 *write block status
 *@block: block number to set
 *@protect_mode: protect mode
 */
static unsigned char onewire_write_block_status(unsigned char block,
                                                            unsigned char protect_mode,
                                                            onewire_phy_ops *phy_ops,
                                                            maxim_time_request *mtrq)
{
    unsigned char buf[64];
    unsigned char bufcnt = 0;
    unsigned char i;
    unsigned char cr;

    ONEWIRE_NOTIFY_ALL_SLAVES(phy_ops);

    /* construct a packet to send */
    buf[bufcnt++] = WRITE_BLOCK_PROTECTION;
    buf[bufcnt++] = (protect_mode & PROTECTION_MASK) | (block & BLOCK_MASK);

    /* send write block protection command & parameter */
    for (i = 0; i < bufcnt; i++) {
        phy_ops->write_byte(buf[i]);
    }

    /* Receive CRC16 */
    buf[bufcnt++] = phy_ops->read_byte();
    buf[bufcnt++] = phy_ops->read_byte();
    DO_CRC16(buf, bufcnt);

    /* Send Release Byte */
    phy_ops->write_byte(ONEWIRE_RELEASE);
    /* Wait for eeprom writing done */
    phy_ops->wait_for_ic(mtrq->program_ms);

    /* Get command success indicator */
    cr = phy_ops->read_byte();
    MAXIM_COMMAND_FAIL_PROCESS(cr);

    /* reset */
    phy_ops->reset();

    return MAXIM_ONEWIRE_SUCCESS;
}


/*
 *Read personality
 *@personality: device's personality data.User should make sure personality has 4 bytes space.
 */
static unsigned char onewire_read_personality(unsigned char *personality,
                                                         onewire_phy_ops *phy_ops)
{
    unsigned char buf[64];
    unsigned char bufcnt = 0;
    unsigned char i;

    ONEWIRE_NOTIFY_ALL_SLAVES(phy_ops);

    /* read status command */
    buf[bufcnt++] = READ_STAUS;
    /* read personality parameter */
    buf[bufcnt++] = PERSONALITY_CONFIG;
    /* Send command & parameter */
    for (i = 0; i < bufcnt; i++)  phy_ops->write_byte(buf[i]);

    /* Receive CRC */
    buf[bufcnt++] = phy_ops->read_byte();
    buf[bufcnt++] = phy_ops->read_byte();
    DO_CRC16(buf, bufcnt);

    /* get personality data */
    for (bufcnt = 0, i = 0; i < 4 ; i++, bufcnt++) {
        personality[bufcnt] = phy_ops->read_byte();
        buf[bufcnt] = personality[bufcnt];
    }
    /* Receive CRC */
    buf[bufcnt++] = phy_ops->read_byte();
    buf[bufcnt++] = phy_ops->read_byte();
    DO_CRC16(buf, bufcnt);

    phy_ops->reset();

    return MAXIM_ONEWIRE_SUCCESS;
}

/*
 *Read block status
 *@block_status: device's block status.User should make sure personality has 4 bytes space.
 */
static unsigned char onewire_read_block_status(unsigned char *block_status,
                                                          onewire_phy_ops *phy_ops)
{
    unsigned char buf[64];
    unsigned char bufcnt = 0;
    unsigned char i;

    ONEWIRE_NOTIFY_ALL_SLAVES(phy_ops);

    /* read status command */
    buf[bufcnt++] = READ_STAUS;
    /* read personality parameter */
    buf[bufcnt++] = BLOCK_STATUS_CONFIG;
    /* Send command & parameter */
    for (i = 0; i < bufcnt; i++)  phy_ops->write_byte(buf[i]);

    /* Receive CRC */
    buf[bufcnt++] = phy_ops->read_byte();
    buf[bufcnt++] = phy_ops->read_byte();
    DO_CRC16(buf, bufcnt);

    /* get personality data */
    for (bufcnt = 0, i = 0; i < 4 ; i++, bufcnt++) {
        block_status[bufcnt] = phy_ops->read_byte();
        buf[bufcnt] = block_status[bufcnt];
    }
    /* Receive CRC */
    buf[bufcnt++] = phy_ops->read_byte();
    buf[bufcnt++] = phy_ops->read_byte();
    DO_CRC16(buf, bufcnt);

    phy_ops->reset();

    return MAXIM_ONEWIRE_SUCCESS;
}


/*
 *To read or write 32 Bytes Scratchpad buffer.
 *@readmode : 0 means write, !0 means read
 *@buffer : Data buffer ; user should make sure buffer is 32 bytes.
 *inside the struct works fine.
 */
static unsigned char onewire_read_write_sram(unsigned char readmode,
                                                        unsigned char *buffer,
                                                        onewire_phy_ops *phy_ops)
{
    unsigned char buf[64];
    unsigned char bufcnt = 0;
    unsigned char i;

    ONEWIRE_NOTIFY_ALL_SLAVES(phy_ops);

    /* read write scratchpad command */
    buf[bufcnt++] = READ_WRITE_SCRATCHPAD;
    /* read write scratchpad parameter */
    buf[bufcnt++] = readmode ? READ_MODE : WRITE_MODE;

    /* Send read write scratchpad command & parameter */
    for (i = 0; i < bufcnt; i++) {
        phy_ops->write_byte(buf[i]);
    }
    /* Receive CRC */
    buf[bufcnt++] = phy_ops->read_byte();
    buf[bufcnt++] = phy_ops->read_byte();
    DO_CRC16(buf, bufcnt);

    /* read or write 32 bytes */
    if(readmode) {
        for (i = 0, bufcnt = 0; i < 32; bufcnt++, i++) {
            buffer[bufcnt] = phy_ops->read_byte();
            buf[i] = buffer[bufcnt];
        }
    } else {
        for (i = 0, bufcnt = 0; i < 32; bufcnt++, i++) {
            phy_ops->write_byte(buffer[i]);
            buf[bufcnt] = buffer[i];
        }
    }

    /* Receive CRC */
    buf[bufcnt++] = phy_ops->read_byte();
    buf[bufcnt++] = phy_ops->read_byte();
    DO_CRC16(buf, bufcnt);

    phy_ops->reset();

    return MAXIM_ONEWIRE_SUCCESS;
}

static unsigned char onewire_read_sram (unsigned char *readbuffer,
                                        onewire_phy_ops *phy_ops)
{
    return onewire_read_write_sram(READ_MODE, readbuffer, phy_ops);
}

static unsigned char onewire_write_sram(unsigned char *writebuffer,
                                                onewire_phy_ops *phy_ops)
{
    return onewire_read_write_sram(WRITE_MODE, writebuffer, phy_ops);
}

/*
 *The Compute and Read Page MAC command is used to get MAC.
 *@anonymous: anonymous mode use 8 bytes 0xFF instead of ROM ID.
 *@page: bit0 of page is used to determine which page is used for computing MAC.
 *0 means page0, 1 means page1.
 *@mac: the mac result. User should make sure mac is 32 bytes.
 *inside the struct works fine.
 */
static unsigned char onewire_compute_and_read_page_mac(unsigned char anonymous,
                                                                      unsigned char page,
                                                                      unsigned char *mac,
                                                                      onewire_phy_ops *phy_ops,
                                                                      maxim_time_request *mtrq)
{
    unsigned char buf[64];
    unsigned char bufcnt = 0;
    unsigned char i;
    unsigned char cr;

    ONEWIRE_NOTIFY_ALL_SLAVES(phy_ops);

    /* Compute and Read Page MAC command */
    buf[bufcnt++] = COMPUTE_AND_READ_PAGE_MAC;

    /* Compute and Read Page MAC parameter */
    buf[bufcnt++] = anonymous ? (ANONYMOUS_MODE | page) : page;

    /* Send Compute and Read Page MAC & parameter */
    for (i = 0; i < bufcnt; i++) {
        phy_ops->write_byte(buf[i]);
    }

    /* Receive CRC */
    buf[bufcnt++] = phy_ops->read_byte();
    buf[bufcnt++] = phy_ops->read_byte();
    DO_CRC16(buf, bufcnt);

    /* Wait for IC compute MAC finish */
    phy_ops->wait_for_ic(DOUBLE(mtrq->compute_mac_ms));

    /* read command response */
    cr = phy_ops->read_byte();
    MAXIM_COMMAND_FAIL_PROCESS(cr);

    /* read MAC */
    for (i = 0, bufcnt = 0 ; i < 32; i++, bufcnt++) {
        mac[i] = phy_ops->read_byte();
        buf[bufcnt] = mac[i];
    }
    /* Receive CRC */
    buf[bufcnt++] = phy_ops->read_byte();
    buf[bufcnt++] = phy_ops->read_byte();
    DO_CRC16(buf, bufcnt);

    return MAXIM_ONEWIRE_SUCCESS;
}

#if 0
/*
 *The Authenticated Write Memory command is used to program one or more
 *contiguous 4 byte segments of a memory block that requires master
 *authentication.
 *@segment:specify the location within the selected memory page where the
 *writing begins.
 *@page: bit0 of page is used to determine which page is going to be written.
 *0 means page0, 1 means page1.
 *@buffer: Data buffer ready for writing specified EEPROM. User should make sure
 *it's 4 bytes length.
 *@mac: mac is authentication. User should make sure it's 32 bytes length;
 *inside the struct works fine.
 */
static unsigned char onewire_auth_write_memory(unsigned char segment,
                                                          unsigned char page,
                                                          unsigned char *buffer,
                                                          unsigned char *mac,
                                                          onewire_phy_ops *phy_ops,
                                                          maxim_time_request *mtrq)
{
    unsigned char buf[64];
    unsigned char bufcnt = 0;
    unsigned char i;
    unsigned char cr;

    ONEWIRE_NOTIFY_ALL_SLAVES(phy_ops);

    /* Authenticated write memory command */
    buf[bufcnt++] = AUTHENTICATED_WRITE_MEMORY;

    /* Authenticated write memory data */
    buf[bufcnt++] = ((segment & SEGMENT_MASK) << SEGMENT_OFFSET) | page;

    /* Send authenticated write memory command & parameter */
    for (i = 0; i < bufcnt; i++) {
        phy_ops->write_byte(buf[i]);
    }
    /* Receive CRC */
    buf[bufcnt++] = phy_ops->read_byte();
    buf[bufcnt++] = phy_ops->read_byte();
    DO_CRC16(buf, bufcnt);


    /* Send all new segment data */
    for(i = 0, bufcnt = 0; i < MAXIM_SEGMENT_4BYTES ; i++, bufcnt++) {
        phy_ops->write_byte(buffer[i]);
        buf[bufcnt] = buffer[i];
    }
    /* Receive CRC */
    buf[bufcnt++] = phy_ops->read_byte();
    buf[bufcnt++] = phy_ops->read_byte();
    DO_CRC16(buf, bufcnt);
    /* Wait for MAC computation */
    phy_ops->wait_for_ic(mtrq->compute_mac_ms);

    /* Send MAC for authenticated writing*/
    for(i = 0, bufcnt = 0; i < SHA256_WORDSIZE_32BITS ; i++, bufcnt++) {
        phy_ops->write_byte(mac[i]);
        buf[bufcnt] = mac[i];
    }
    /* Receive CRC */
    buf[bufcnt++] = phy_ops->read_byte();
    buf[bufcnt++] = phy_ops->read_byte();
    DO_CRC16(buf, bufcnt);

    /* Get command response */
    cr = phy_ops->read_byte();
    MAXIM_COMMAND_FAIL_PROCESS(cr);

    /* Send Release Byte */
    phy_ops->write_byte(ONEWIRE_RELEASE);
    /* Wait for eeprom writing done */
    phy_ops->wait_for_ic(mtrq->program_ms);

    /* Get command response */
    cr = phy_ops->read_byte();
    MAXIM_COMMAND_FAIL_PROCESS(cr);

    /* Reset */
    phy_ops->reset();

    return MAXIM_ONEWIRE_SUCCESS;
}
#endif

static int maxim_get_rom_id (maxim_onewire_ic_des *ic_des)
{
    int ret;
    maxim_onewire_mem *mom = &ic_des->memory;

    mutex_lock(&mom->lock);
    mom->validity_rom_id = 0;
    ret = onewire_read_rom(mom->rom_id, &ic_des->phy_ops);
    if(!ret) {
        mom->validity_rom_id = 1;
    }
    mutex_unlock(&mom->lock);

    return ret;
}

static int maxim_get_personality (maxim_onewire_ic_des *ic_des)
{
    int ret;
    maxim_onewire_mem *mom = &ic_des->memory;

    mutex_lock(&mom->lock);
    mom->validity_personality = 0;
    ret = onewire_read_personality(mom->personality, &ic_des->phy_ops);
    if(!ret) {
        mom->validity_personality = 1;
    }
    mutex_unlock(&mom->lock);

    return ret;
}

/* get user eeprom [seg_start, seg_end) */
static int maxim_get_user_memory (maxim_onewire_ic_des *ic_des, unsigned char page,
                                            unsigned char seg_start, unsigned char seg_end)
{
    int ret;
    int i;
    unsigned char temp;
    unsigned char *mem_start;
    maxim_onewire_mem *mom = &ic_des->memory;

    if (seg_start >= seg_end) {
        return MAXIM_ONEWIRE_ILLEGAL_PARAM;
    }
    if (seg_end > MAXIM_PAGE_SEGMENT_NUM) {
        return MAXIM_ONEWIRE_ILLEGAL_PARAM;
    }
    if (page >= mom->page_number) {
        return MAXIM_ONEWIRE_ILLEGAL_PARAM;
    }
    mutex_lock(&mom->lock);
    for(i = seg_start; i < seg_end; i++) {
        mom->validity_segment[page][i] = 0;
    }
    mom->validity_page[page] = 0;
    mem_start = mom->user_memory + seg_start * MAXIM_SEGMENT_SIZE + page * mom->page_size;
    ret = onewire_read_memory(seg_start, seg_end, page, mem_start, &ic_des->phy_ops);
    if(!ret) {
        for(i = seg_start, temp = 1; i < seg_end; i++) {
            mom->validity_segment[page][i] = 1;
        }
        for(i = 0, mom->validity_page[page] = 1; i < MAXIM_PAGE_SEGMENT_NUM; i++) {
            if(mom->validity_segment[page][i] == 0) {
                mom->validity_page[page] = 0;
                break;
            }
        }
    }
    mutex_unlock(&mom->lock);

    return ret;
}

/* write user eeprom segment */
static int maxim_set_user_memory (maxim_onewire_ic_des *ic_des, unsigned char *newdata,
                                            unsigned char page, unsigned char segment)
{
    int ret;
    int i;
    maxim_onewire_mem *mom = &ic_des->memory;

    if (segment >= MAXIM_PAGE_SEGMENT_NUM) {
        return MAXIM_ONEWIRE_ILLEGAL_PARAM;
    }
    if (page >= mom->page_number) {
        return MAXIM_ONEWIRE_ILLEGAL_PARAM;
    }

    mutex_lock(&mom->lock);
    mom->validity_segment[page][segment] = 0;
    mom->validity_page[page] = 0;
    mom->validity_mac = 0;
    memcpy(mom->user_memory + segment * MAXIM_SEGMENT_SIZE + page * mom->page_size,
           newdata, MAXIM_SEGMENT_SIZE);
    ret = onewire_write_memory (segment, page, newdata, &ic_des->phy_ops, &ic_des->trq);
    if(!ret) {
        mom->validity_segment[page][segment] = 1;
        for(i = 0, mom->validity_page[page] = 1; i < MAXIM_PAGE_SEGMENT_NUM; i++) {
            if(mom->validity_segment[page][i] == 0) {
                mom->validity_page[page] = 0;
                break;
            }
        }
    }
    mutex_unlock(&mom->lock);

    return ret;
}

static int maxim_get_sram (maxim_onewire_ic_des *ic_des)
{
    int ret;
    maxim_onewire_mem *mom = &ic_des->memory;

    mutex_lock(&mom->lock);
    mom->validity_sram = 0;
    ret = onewire_read_sram(mom->sram, &ic_des->phy_ops);
    if(!ret) {
        mom->validity_sram = 1;
    }
    mutex_unlock(&mom->lock);

    return ret;
}

static int maxim_set_sram(maxim_onewire_ic_des *ic_des, unsigned char *newdata)
{
    int ret;
    maxim_onewire_mem *mom = &ic_des->memory;

    mutex_lock(&mom->lock);
    mom->validity_sram = 0;
    mom->validity_mac = 0;
    memcpy(mom->sram, newdata, mom->sram_length);
    ret = onewire_write_sram(mom->sram, &ic_des->phy_ops);
    if(!ret) {
        mom->validity_sram = 1;
    }
    mutex_unlock(&mom->lock);

    return ret;
}

static int maxim_get_mac(maxim_onewire_ic_des *ic_des, unsigned char anonymous,
                               unsigned char page)
{
    int ret;
    maxim_onewire_mem *mom = &ic_des->memory;

    mutex_lock(&mom->lock);
    mom->validity_mac = 0;
    mom->validity_sram = 0;
    ret = onewire_compute_and_read_page_mac(anonymous, page, mom->mac,
                                            &ic_des->phy_ops, &ic_des->trq);
    if(!ret) {
        mom->validity_mac = 1;
    }
    mutex_unlock(&mom->lock);

    return ret;
}

static int maxim_set_status(maxim_onewire_ic_des *ic_des, unsigned char block,
                                   unsigned char protect_mode)
{
    int ret;
    maxim_onewire_mem *mom = &ic_des->memory;

    mutex_lock(&mom->lock);
    mom->validity_status = 0;
    ret = onewire_write_block_status(block, protect_mode, &ic_des->phy_ops, &ic_des->trq);
    if(!ret) {
        mom->validity_status = 1;
    }
    mutex_unlock(&mom->lock);

    return ret;

}

static int maxim_get_status(maxim_onewire_ic_des *ic_des)
{
    int ret;
    maxim_onewire_mem *mom = &ic_des->memory;

    mutex_lock(&mom->lock);
    mom->validity_status = 0;
    ret = onewire_read_block_status(mom->block_status, &ic_des->phy_ops);
    if(!ret) {
        mom->validity_status = 1;
    }
    mutex_unlock(&mom->lock);

    return ret;

}

/* Battery information initialization */
static int maxim_valid_mem_ops(maxim_onewire_ic_des *ic_des, struct platform_device *pdev)
{
    /* get battery ic parameters */
    int ret;
    int i;
    unsigned int myid;
    onewire_phy_ops *opo;
    ow_phy_reg_list *pos;

    ONEWIRE_NULL_POINT(ic_des);
    ONEWIRE_NULL_POINT(pdev);

    opo = &ic_des->phy_ops;

    ret = of_property_read_u32(pdev->dev.of_node, "phy-ctrl", &myid);
    if(ret) {
        hwlog_err("No physical controller specified in device tree for %s.", pdev->name);
        return ONEWIRE_REGIST_FAIL;
    } else {
        /* find the physical controller by matching phanlde number */
        list_for_each_entry(pos, &ow_phy_reg_head, node) {
            if(pos->onewire_phy_register != NULL) {
                ret = pos->onewire_phy_register(opo, myid);
            } else {
                ret = ONEWIRE_REGIST_FAIL;
            }
            if(!ret) {
                hwlog_info("Found phy-ctrl for %s.", pdev->name);
                break;
            }
        }
        if(ret) {
            hwlog_err("Can't find corresponding physical controller for %s.", pdev->name);
            return ONEWIRE_REGIST_FAIL;
        }
    }

    /* check all onewire physical operations are valid */
    ONEWIRE_NULL_POINT(opo->reset);
    ONEWIRE_NULL_POINT(opo->read_byte);
    ONEWIRE_NULL_POINT(opo->write_byte);
    ONEWIRE_NULL_POINT(opo->set_time_request);
    ONEWIRE_NULL_POINT(opo->wait_for_ic);

    /* set time request for physic controller first */
    opo->set_time_request(&ic_des->trq.ow_trq);

    /* try to get batt_id */
    for( i = 0; i < FIND_IC_RETRY_NUM; i++) {
        ret = maxim_get_rom_id(ic_des);
        if(!ret) {
            break;
        }
    }
    if(ret) {
        hwlog_err("%s try to get rom id failed in %s.", pdev->name, __func__);
        return ret;
    }

    return MAXIM_ONEWIRE_SUCCESS;
}

static int maxim_onewire_memory_init(maxim_onewire_ic_des *ic_des,
                                                struct platform_device *pdev)
{
    int ret;
    struct device_node *batt_ic_np;
    maxim_onewire_mem *mom;

    batt_ic_np = pdev->dev.of_node;
    /* Get battery ic's memory description */
    mom = &ic_des->memory;
    memset(mom, 0, sizeof(maxim_onewire_mem));
    ret = of_property_read_u32(batt_ic_np, "rom-id-length", &(mom->rom_id_length));
    MAXIM_ONEWIRE_DTS_READ_ERROR_RETURN(ret, "rom-id-length");
    ret = of_property_read_u32(batt_ic_np, "block-number", &(mom->block_number));
    MAXIM_ONEWIRE_DTS_READ_ERROR_RETURN(ret, "block-number");
    ret = of_property_read_u32(batt_ic_np, "block-size", &(mom->block_size));
    MAXIM_ONEWIRE_DTS_READ_ERROR_RETURN(ret, "block-size");
    ret = of_property_read_u32(batt_ic_np, "personality-length", &(mom->personality_length));
    MAXIM_ONEWIRE_DTS_READ_ERROR_RETURN(ret, "personality-length");
    ret = of_property_read_u32(batt_ic_np, "sram-length", &(mom->sram_length));
    MAXIM_ONEWIRE_DTS_READ_ERROR_RETURN(ret, "sram-length");
    ret = of_property_read_u32(batt_ic_np, "page-number", &(mom->page_number));
    MAXIM_ONEWIRE_DTS_READ_ERROR_RETURN(ret, "page-number");
    ret = of_property_read_u32(batt_ic_np, "page-size", &(mom->page_size));
    MAXIM_ONEWIRE_DTS_READ_ERROR_RETURN(ret, "page-size");
    ret = of_property_read_u32(batt_ic_np, "mac-length", &(mom->mac_length));
    MAXIM_ONEWIRE_DTS_READ_ERROR_RETURN(ret, "mac-length");

    /* Allocate memory for battery memory */
    mom->rom_id = (unsigned char *)devm_kzalloc(&pdev->dev, mom->rom_id_length, GFP_KERNEL);
    ONEWIRE_KALLOC_FAIL(mom->rom_id);
    mom->block_status = (unsigned char *)devm_kzalloc(&pdev->dev, mom->block_number, GFP_KERNEL);
    ONEWIRE_KALLOC_FAIL(mom->block_status);
    mom->personality = (unsigned char *)devm_kzalloc(&pdev->dev, mom->personality_length,
                                                     GFP_KERNEL);
    ONEWIRE_KALLOC_FAIL(mom->personality);
    mom->sram = (unsigned char *)devm_kzalloc(&pdev->dev, mom->sram_length, GFP_KERNEL);
    ONEWIRE_KALLOC_FAIL(mom->sram);
    mom->user_memory = (unsigned char *)devm_kzalloc(&pdev->dev, mom->page_number * mom->page_size,
                                                     GFP_KERNEL);
    ONEWIRE_KALLOC_FAIL(mom->user_memory);
    mom->mac = (unsigned char *)devm_kzalloc(&pdev->dev, mom->mac_length, GFP_KERNEL);
    ONEWIRE_KALLOC_FAIL(mom->mac);

    /* init maxim_onewire_mem lock */
    mutex_init(&mom->lock);

    return MAXIM_ONEWIRE_SUCCESS;
}

static int maxim_onewire_time_rq_init(maxim_onewire_ic_des *ic_des,
                                      struct platform_device *pdev)
{
    int ret;
    struct device_node *batt_ic_np;
    onewire_time_request *owtrq;

    batt_ic_np = pdev->dev.of_node;

    /* get 1-wire ic time requests */
    owtrq = &ic_des->trq.ow_trq;
    ret = of_property_read_u32_index(batt_ic_np, "reset-time-request",
                                     FIRST_TIME_PROPERTY,
                                     &(owtrq->reset_init_low_ns));
    MAXIM_ONEWIRE_DTS_READ_ERROR_RETURN(ret, "reset_init_low_ns");
    ret = of_property_read_u32_index(batt_ic_np, "reset-time-request",
                                     SECOND_TIME_PROPERTY,
                                     &(owtrq->reset_slave_response_delay_ns));
    MAXIM_ONEWIRE_DTS_READ_ERROR_RETURN(ret, "reset_slave_response_delay_ns");
    ret = of_property_read_u32_index(batt_ic_np, "reset-time-request",
                                     THIRD_TIME_PROPERTY,
                                     &(owtrq->reset_hold_high_ns));
    MAXIM_ONEWIRE_DTS_READ_ERROR_RETURN(ret, "reset_hold_high_ns");
    ret = of_property_read_u32_index(batt_ic_np, "write-time-request",
                                     FIRST_TIME_PROPERTY,
                                     &(owtrq->write_init_low_ns));
    MAXIM_ONEWIRE_DTS_READ_ERROR_RETURN(ret, "write_init_low_ns");
    ret = of_property_read_u32_index(batt_ic_np, "write-time-request",
                                     SECOND_TIME_PROPERTY,
                                     &(owtrq->write_hold_ns));
    MAXIM_ONEWIRE_DTS_READ_ERROR_RETURN(ret, "write_hold_ns");
    ret = of_property_read_u32_index(batt_ic_np, "write-time-request",
                                     THIRD_TIME_PROPERTY,
                                     &(owtrq->write_residual_ns));
    MAXIM_ONEWIRE_DTS_READ_ERROR_RETURN(ret, "write_residual_ns");
    ret = of_property_read_u32_index(batt_ic_np, "read-time-request",
                                     FIRST_TIME_PROPERTY,
                                     &(owtrq->read_init_low_ns));
    MAXIM_ONEWIRE_DTS_READ_ERROR_RETURN(ret, "read_init_low_ns");
    ret = of_property_read_u32_index(batt_ic_np, "read-time-request",
                                     SECOND_TIME_PROPERTY,
                                     &(owtrq->read_residual_ns));
    MAXIM_ONEWIRE_DTS_READ_ERROR_RETURN(ret, "read_residual_ns");
    ret = of_property_read_u32(batt_ic_np, "program-time", &(ic_des->trq.program_ms));
    MAXIM_ONEWIRE_DTS_READ_ERROR_RETURN(ret, "program_ms");
    ret = of_property_read_u32(batt_ic_np, "secret-program-time",
                               &(ic_des->trq.secret_program_ms));
    MAXIM_ONEWIRE_DTS_READ_ERROR_RETURN(ret, "secret_program_ms");
    ret = of_property_read_u32(batt_ic_np, "compute-mac-time",
                               &(ic_des->trq.compute_mac_ms));
    MAXIM_ONEWIRE_DTS_READ_ERROR_RETURN(ret, "compute_mac_ms");

    owtrq->reset_hold_high_cycles = ns2cycles(owtrq->reset_hold_high_ns);
    owtrq->reset_init_low_cycles = ns2cycles(owtrq->reset_init_low_ns);
    owtrq->reset_slave_response_delay_cycles = ns2cycles(owtrq->reset_slave_response_delay_ns);
    owtrq->write_init_low_cycles = ns2cycles(owtrq->write_init_low_ns);
    owtrq->write_hold_cycles = ns2cycles(owtrq->write_hold_ns);
    owtrq->write_residual_cycles = ns2cycles(owtrq->write_residual_ns);
    owtrq->read_init_low_cycles = ns2cycles(owtrq->read_init_low_ns);
    owtrq->read_residual_cycles = ns2cycles(owtrq->read_residual_ns);
    hwlog_info("Timer Frequence is %u, Hz is %u, Loops per jiffy is %lu\n",
               arch_timer_get_cntfrq(), HZ, loops_per_jiffy);
    hwlog_info("reset_init_low_cycles = %ucycles(%uns),"
               "reset_slave_response_delay_cycles = %ucycles(%uns),"
               "reset_hold_high_cycles = %ucycles(%uns)\n",
               owtrq->reset_init_low_cycles, owtrq->reset_init_low_ns,
               owtrq->reset_slave_response_delay_cycles, owtrq->reset_slave_response_delay_ns,
               owtrq->reset_hold_high_cycles, owtrq->reset_hold_high_ns);
    hwlog_info("write_init_low_cycles = %ucycles(%uns),"
               "write_hold_cycles = %ucycles(%uns),"
               "write_residual_cycles = %ucycles(%uns)\n",
               owtrq->write_init_low_cycles, owtrq->write_init_low_ns,
               owtrq->write_hold_cycles, owtrq->write_hold_ns,
               owtrq->write_residual_cycles, owtrq->write_residual_ns);
    hwlog_info("read_init_low_cycles = %ucycles(%uns),"
               "read_residual_cycles = %ucycles(%uns)\n",
               owtrq->read_init_low_cycles, owtrq->read_init_low_ns,
               owtrq->read_residual_cycles, owtrq->read_residual_ns);
    return MAXIM_ONEWIRE_SUCCESS;
}

/*
 *this function make maxim onewire core struct fine.
 *1.onewire_phy_ops--onewire physical signal and basic operations
 *2.maxim_onewire_ic_des--maxim onewire ic memory and commution operations
 */
int maxim_onewire_register(maxim_onewire_ic_des *ic_des,
                           struct platform_device *pdev)
{
    int ret;
    maxim_mem_ops *mmo;

    ONEWIRE_NULL_POINT(ic_des);
    ONEWIRE_NULL_POINT(pdev);

    /* init memory */
    ret = maxim_onewire_memory_init(ic_des, pdev);
    if(ret) {
        hwlog_err("Maxim onewire memory init failed(%d) in %s.\n", ret, __func__);
        return ret;
    }

    /* init time request */
    ret = maxim_onewire_time_rq_init(ic_des, pdev);
    if(ret) {
        hwlog_err("Maxim onewire time request init failed(%d) in %s.\n", ret, __func__);
        return ret;
    }

    /* register memory operations */
    mmo = &ic_des->mem_ops;
    mmo->get_rom_id      =      maxim_get_rom_id;
    mmo->get_personality =      maxim_get_personality;
    mmo->get_user_memory =      maxim_get_user_memory;
    mmo->set_user_memory =      maxim_set_user_memory;
    mmo->set_sram        =      maxim_set_sram;
    mmo->get_sram        =      maxim_get_sram;
    mmo->get_mac         =      maxim_get_mac;
    mmo->set_status      =      maxim_set_status;
    mmo->get_status      =      maxim_get_status;
    mmo->valid_mem_ops   =      maxim_valid_mem_ops;

    return MAXIM_ONEWIRE_SUCCESS;
}
