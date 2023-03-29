#ifndef VERITY_HANDLE_H
#define VERITY_HANDLE_H
#define FIRST_TIME_HASH_VERIFY 0
#define NO_FIRST_TIME_HASH_VERIFY 1
int oem_verity_fec_decode(struct dm_verity *v, struct dm_verity_io *io,
		      sector_t block, u8* metadata, struct bvec_iter* iter, struct bvec_iter* start,  struct bvec_iter* start1,
			  u8* want_digest, enum verity_block_type type);
int oem_verity_handle_err(struct dm_verity *v);
void verity_dsm_init(void);
int verity_hash_sel_sha(struct dm_verity *v, struct shash_desc *desc,
		const u8 *data, size_t len, u8 *digest, u32 count);
#endif