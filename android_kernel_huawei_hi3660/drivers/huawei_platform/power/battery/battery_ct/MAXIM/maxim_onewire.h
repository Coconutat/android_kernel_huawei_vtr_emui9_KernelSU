#ifndef MAXIM_ONEWIRE_H
#define MAXIM_ONEWIRE_H

#include "onewire_common.h"
#include "onewire_phy_common.h"

/* onewire common interface */
typedef struct {
    onewire_time_request ow_trq;
    unsigned int program_ms;
    unsigned int secret_program_ms;
    unsigned int compute_mac_ms;
} maxim_time_request;


#define MAX_MAXIM_PAGE_NUM                          4
#define MAXIM_PAGE_SEGMENT_NUM                      8

typedef struct {
    unsigned char validity_rom_id;
    unsigned char validity_personality;
    unsigned char validity_sram;
    unsigned char validity_status;
    unsigned char validity_page[MAX_MAXIM_PAGE_NUM];
    unsigned char validity_segment[MAX_MAXIM_PAGE_NUM][MAXIM_PAGE_SEGMENT_NUM];
    unsigned char validity_mac;
    unsigned char *block_status;
    unsigned char *rom_id;
    unsigned char *personality;
    unsigned char *sram;
    unsigned char *user_memory;
    unsigned char *mac;
    unsigned int  ic_type;
    unsigned int  rom_id_length;
    unsigned int  block_number;
    unsigned int  block_size;
    unsigned int  personality_length;
    unsigned int  sram_length;
    unsigned int  page_number;
    unsigned int  page_size;
    unsigned int  mac_length;
    unsigned int  sn_length_bits;
    unsigned int  sn_page;
    unsigned int  sn_offset_bits;
    struct mutex  lock;
} maxim_onewire_mem;

struct __maxim_onewire_ic_des;
typedef struct __maxim_onewire_ic_des maxim_onewire_ic_des;

typedef struct __maxim_mem_ops {
    int (*get_rom_id)(maxim_onewire_ic_des *);
    int (*get_personality)(maxim_onewire_ic_des *);
    int (*get_user_memory)(maxim_onewire_ic_des *, unsigned char, unsigned char, unsigned char);
    int (*set_user_memory)(maxim_onewire_ic_des *, unsigned char *, unsigned char, unsigned char);
    int (*get_sram)(maxim_onewire_ic_des *);
    int (*set_sram)(maxim_onewire_ic_des *, unsigned char *);
    int (*get_mac)(maxim_onewire_ic_des *, unsigned char, unsigned char);
    int (*get_status)(maxim_onewire_ic_des *);
    int (*set_status)(maxim_onewire_ic_des *, unsigned char ,unsigned char);
    int (*valid_mem_ops)(maxim_onewire_ic_des *, struct platform_device *);
} maxim_mem_ops;

/* maxim_onewire_ic_des--maxim onewire ic memory and commution operations */
struct __maxim_onewire_ic_des {
    maxim_time_request trq;
    maxim_onewire_mem memory;
    maxim_mem_ops mem_ops;
    onewire_phy_ops phy_ops;
};

#define DOUBLE(x)                                           ((x)<<1)

/* Maxim ROM */
#define MAXIM_ROM_SIZE                                      8
#define MAXIM_SEGMENT_SIZE                                  4
#define MAXIM_SECRET_SIZE                                   32
#define MAXIM_MAC256_SIZE                                   32
#define MAXIM_PAGE_SIZE                                     32

/* Slave presence signal is low */
#define NO_SLAVE_RESPONSE(x)                                ((x)!=0)

/* MAXIM 1-wire memory and SHA function command */
#define WRITE_MEMORY                                        0x55
#define READ_MEMORY                                         0xF0
#define WRITE_BLOCK_PROTECTION                              0xC3
#define READ_STAUS                                          0xAA
#define READ_WRITE_SCRATCHPAD                               0x0F
#define LOAD_AND_LOCK_SECRET                                0x33
#define COMPUTE_AND_LOCK_SECRET                             0x3C
#define COMPUTE_AND_READ_PAGE_MAC                           0xA5
#define AUTHENTICATED_WRITE_MEMORY                          0x5A
#define AUTHENTICATED_WRITE_BLOCK_PROTECTION                0xCC

/* MAXIM 1-wire rom function command */
#define READ_ROM                                            0x33
#define MATCH_ROM                                           0x55
#define SEARCH_ROM                                          0xF0
#define SKIP_ROM                                            0xCC
#define RESUME_COMMAND                                      0xA5

/* Command Parameters */
#define SEGMENT_OFFSET                                      0x05
#define MAXIM_PAGE_MASK                                     0x01
#define SEGMENT_MASK                                        0x07
#define ANONYMOUS_MODE                                      0xE0
#define READ_MODE                                           0x0F
#define WRITE_MODE                                          0x00
#define BLOCK_STATUS_CONFIG                                 0x00
#define PERSONALITY_CONFIG                                  0xE0
#define ONEWIRE_RELEASE                                     0xAA
#define BLOCK_MASK                                          0x03
#define PROTECTION_MASK                                     0xF0
#define MAXIM_PAGE0                                         0
#define MAXIM_PAGE1                                         1
#define MAXIM_BLOCK0                                        0
#define MAXIM_BLOCK1                                        1
#define MAXIM_BLOCK2                                        2
#define MAXIM_BLOCK3                                        3
#define MAXIM_SEGMENT0                                      0
#define MAXIM_SEGMENT1                                      1
#define MAXIM_SEGMENT2                                      2
#define MAXIM_SEGMENT3                                      3
#define MAXIM_SEGMENT4                                      4
#define MAXIM_SEGMENT5                                      5
#define MAXIM_SEGMENT6                                      6
#define MAXIM_SEGMENT7                                      7
#define MAXIM_MAN_ID_SIZE                                   2
#define MAXIM_MAN_ID_OFFSET                                 2
#define MAXIM_SEGMENTS2BYTES(x)                             ((x) << 2 )

/* Command success response */
#define MAXIM_ONEWIRE_COMMAND_SUCCESS                       0xAA

/* 1-wire function operation return signals */
#define MAXIM_ONEWIRE_SUCCESS                               ONEWIRE_SUCCESS
#define MAXIM_ERROR_BIT                                     0x80
#define MAXIM_ONEWIRE_DTS_ERROR                             (MAXIM_ERROR_BIT|ONEWIRE_DTS_FAIL)
#define MAXIM_ONEWIRE_COM_ERROR                             (MAXIM_ERROR_BIT|ONEWIRE_COM_FAIL)
#define MAXIN_ONEWIRE_CRC8_ERROR                            (MAXIM_ERROR_BIT|ONEWIRE_CRC8_FAIL)
#define MAXIN_ONEWIRE_CRC16_ERROR                           (MAXIM_ERROR_BIT|ONEWIRE_CRC16_FAIL)
#define MAXIM_ONEWIRE_NO_SLAVE                              (MAXIM_ERROR_BIT|ONEWIRE_NO_SLAVE)
#define MAXIM_ONEWIRE_ILLEGAL_PARAM                         (MAXIM_ERROR_BIT|ONEWIRE_ILLEGAL_PARAM)

/* SHA ERROR */
#define SHA_SUCCESS                                         0x00
#define SHA_PARA_ERR                                        0x01
#define SHA_MALLOC_ERR                                      0x02
#define SHA_LENGTH_ERR                                      0x03


/* MAC computation mode */
#define MAXIM_SEGMENT_4BYTES                                4
#define MAXIM_64BYTES                                       64
#define MAXIM_128BYTES                                      128

/* CRC result */
#define MAXIM_CRC16_RESULT                                  0xB001
#define MAXIM_CRC8_RESULT                                   0


/*
 *For SHA-1, SHA-224  and SHA-256, each message block has 512 bits, which are
 *represented as a sequence of sixteen 32-bit words.
 */
#define SHA256_BLOCKSIZE_512BITS                            512
#define SHA256_WORDSIZE_32BITS                              32
#define SHA256_WORDSIZE_4BYTES                              4

/* MAC input data structure */
#define AUTH_MAC_ROM_ID_OFFSET                              96
#define AUTH_MAC_PAGE_OFFSET                                0
#define AUTH_MAC_SRAM_OFFSET                                32
#define AUTH_MAC_KEY_OFFSET                                 64
#define AUTH_MAC_PAGE_NUM_OFFSET                            106
#define AUTH_MAC_MAN_ID_OFFSET                              104
#define MAX_MAC_SOURCE_SIZE                                 128

/* detect onewire slaves and notify all slaves */
#define ONEWIRE_NOTIFY_ALL_SLAVES(x)                                                            \
    if(NO_SLAVE_RESPONSE(x->reset())){                                                          \
        hwlog_err("1-wire maxim: no slave device response, found in %s\n", __func__);           \
        return MAXIM_ONEWIRE_NO_SLAVE;                                                          \
    }                                                                                           \
    x->write_byte(SKIP_ROM)

/* Maxim CRC16 check */
#define DO_CRC16(a,b)                                                                           \
    do{                                                                                         \
        if (check_crc16(a,b) != MAXIM_CRC16_RESULT){                                            \
            hwlog_err("maxim: CRC16 failed in %s\n", __func__);                                 \
            return MAXIN_ONEWIRE_CRC16_ERROR;                                                   \
        } else {                                                                                \
            hwlog_info("maxim: CRC16 success in %s\n", __func__);                               \
        }                                                                                       \
    }while(0)

/* Maxim CRC8 check */
#define CRC8_ERROR_PROCESS(x)                                                                   \
    do{                                                                                         \
        if(x != MAXIM_CRC8_RESULT){                                                             \
            hwlog_err("maxim: CRC8 failed in %s\n", __func__);                                  \
            return MAXIN_ONEWIRE_CRC8_ERROR;                                                    \
        }                                                                                       \
    }while(0)

/* Command response process */
#define MAXIM_COMMAND_FAIL_PROCESS(x)                                                           \
    do{                                                                                         \
        if((x) != MAXIM_ONEWIRE_COMMAND_SUCCESS){                                               \
            hwlog_err("maxim: Error command indicator %x in %s\n", (x), __func__);              \
            return MAXIM_ONEWIRE_COM_ERROR;                                                     \
        }                                                                                       \
    }while(0)

/* dts read property error process*/
#define MAXIM_ONEWIRE_DTS_READ_ERROR_RETURN(x,y)                                                \
    do{                                                                                         \
        if(x){                                                                                  \
            hwlog_err("Battery-driver: DTS do not have "y", needed in %s.\n",__func__);         \
            return MAXIM_ONEWIRE_DTS_ERROR;                                                     \
        }                                                                                       \
    }while(0)

int maxim_onewire_register(maxim_onewire_ic_des *, struct platform_device *);

#endif