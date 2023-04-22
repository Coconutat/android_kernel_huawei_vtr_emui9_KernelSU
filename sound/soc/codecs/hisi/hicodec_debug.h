#ifndef __HICODEC_DEBUG_H__
#define __HICODEC_DEBUG_H__

enum {
	HICODEC_DEBUG_FLAG_READ = 0,
	HICODEC_DEBUG_FLAG_WRITE = 1,
};

/*
 * seg_name: name to print for this entry
 *           NULL for condition that this entry has same seg_name with the previous one
 * start:    address of first reg to dump
 * end:      address of last reg to dump
 * reg_size: register value size in bytes, for example:
 *             soc register --- 4Bytes (32bit)
 *             pmu register --- 1Byte (8bit)
 */
struct hicodec_dump_reg_entry
{
	const char *seg_name;
	unsigned int start;
	unsigned int end;
	unsigned int reg_size;
};

struct hicodec_dump_reg_info
{
	struct hicodec_dump_reg_entry *entry;
	unsigned int count;
};

/*
 * hicodec_debug_init:
 *     init when codec driver is probing
 * @codec:
 * @info: codec-specific infos
 */
int hicodec_debug_init(struct snd_soc_codec *codec, const struct hicodec_dump_reg_info *info);

/*
 * hicodec_debug_uninit:
 *     uninit when codec driver is removing
 * @codec:
 */
void hicodec_debug_uninit(struct snd_soc_codec *codec);

/*
 * hicodec_debug_reg_rw_cache:
 *     record operations of codec registers
 * @reg: register address
 * @val: register value
 * @rw:  HICODEC_DEBUG_FLAG_READ or HICODEC_DEBUG_FLAG_WRITE
 */
void hicodec_debug_reg_rw_cache(unsigned int reg, unsigned int val, int rw);

#endif
