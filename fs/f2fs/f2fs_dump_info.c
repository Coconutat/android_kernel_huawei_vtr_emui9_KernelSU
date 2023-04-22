

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/statfs.h>
#include <linux/buffer_head.h>
#include <linux/backing-dev.h>
#include <linux/kthread.h>
#include <linux/parser.h>
#include <linux/mount.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/random.h>
#include <linux/exportfs.h>
#include <linux/blkdev.h>
#include <linux/f2fs_fs.h>
#include <linux/sysfs.h>

#include "f2fs.h"
#include "segment.h"

/* Display on console */
#define DISP(fmt, ptr, member)				\
	do {						\
		printk("F2FS-fs:%-30s\t\t" fmt, #member, ((ptr)->member));	\
	} while (0)

#define DISP_u32(ptr, member)						\
	do {								\
		printk("F2FS-fs:%-30s" "\t\t[0x%8x : %u]\n",		\
			#member, ((ptr)->member), ((ptr)->member));	\
	} while (0)

#define DISP_u64(ptr, member)						\
	do {								\
		printk("F2FS-fs:%-30s" "\t\t[0x%8llx : %llu]\n",		\
			#member, ((ptr)->member), ((ptr)->member));	\
	} while (0)


/* print the f2fs superblock infomation to the kernel message,
 * it will be saved by DMD or panic log
 * simplified info of f2fs tool: fsck.f2fs
 * f2fs superblock struct locate in kernel/include/linux/f2fs_fs.h
 */
void f2fs_print_raw_sb_info(struct f2fs_sb_info *sbi)
{
	struct f2fs_super_block *sb = NULL;

	if (sbi == NULL) {
		return;
	}

	sb = F2FS_RAW_SUPER(sbi);

	if (sb == NULL) {
		return;
	}

	printk("\n");
	printk("+--------------------------------------------------------+\n");
	printk("| Super block                                            |\n");
	printk("+--------------------------------------------------------+\n");

	DISP_u32(sb, magic);
	DISP_u32(sb, major_ver);
	DISP_u32(sb, minor_ver);
	DISP_u32(sb, log_sectorsize);
	DISP_u32(sb, log_sectors_per_block);

	DISP_u32(sb, log_blocksize);
	DISP_u32(sb, log_blocks_per_seg);
	DISP_u32(sb, segs_per_sec);
	DISP_u32(sb, secs_per_zone);
	DISP_u32(sb, checksum_offset);
	DISP_u64(sb, block_count);

	DISP_u32(sb, section_count);
	DISP_u32(sb, segment_count);
	DISP_u32(sb, segment_count_ckpt);
	DISP_u32(sb, segment_count_sit);
	DISP_u32(sb, segment_count_nat);

	DISP_u32(sb, segment_count_ssa);
	DISP_u32(sb, segment_count_main);
	DISP_u32(sb, segment0_blkaddr);

	DISP_u32(sb, cp_blkaddr);
	DISP_u32(sb, sit_blkaddr);
	DISP_u32(sb, nat_blkaddr);
	DISP_u32(sb, ssa_blkaddr);
	DISP_u32(sb, main_blkaddr);

	DISP_u32(sb, root_ino);
	DISP_u32(sb, node_ino);
	DISP_u32(sb, meta_ino);
	DISP_u32(sb, cp_payload);
	DISP_u32(sb, feature);
	DISP("%s", sb, version);
	printk("\n\n");
}

void f2fs_print_ckpt_info(struct f2fs_sb_info *sbi)
{
	struct f2fs_checkpoint *cp = NULL;

	if (sbi == NULL) {
		return;
	}

	cp = F2FS_CKPT(sbi);

	if (cp == NULL) {
		return;
	}

	printk("\n");
	printk("+--------------------------------------------------------+\n");
	printk("| Checkpoint                                             |\n");
	printk("+--------------------------------------------------------+\n");

	DISP_u64(cp, checkpoint_ver);
	DISP_u64(cp, user_block_count);
	DISP_u64(cp, valid_block_count);
	DISP_u32(cp, rsvd_segment_count);
	DISP_u32(cp, overprov_segment_count);
	DISP_u32(cp, free_segment_count);

	DISP_u32(cp, alloc_type[CURSEG_HOT_NODE]);
	DISP_u32(cp, alloc_type[CURSEG_WARM_NODE]);
	DISP_u32(cp, alloc_type[CURSEG_COLD_NODE]);
	DISP_u32(cp, cur_node_segno[0]);
	DISP_u32(cp, cur_node_segno[1]);
	DISP_u32(cp, cur_node_segno[2]);

	DISP_u32(cp, cur_node_blkoff[0]);
	DISP_u32(cp, cur_node_blkoff[1]);
	DISP_u32(cp, cur_node_blkoff[2]);


	DISP_u32(cp, alloc_type[CURSEG_HOT_DATA]);
	DISP_u32(cp, alloc_type[CURSEG_WARM_DATA]);
	DISP_u32(cp, alloc_type[CURSEG_COLD_DATA]);
	DISP_u32(cp, cur_data_segno[0]);
	DISP_u32(cp, cur_data_segno[1]);
	DISP_u32(cp, cur_data_segno[2]);

	DISP_u32(cp, cur_data_blkoff[0]);
	DISP_u32(cp, cur_data_blkoff[1]);
	DISP_u32(cp, cur_data_blkoff[2]);

	DISP_u32(cp, ckpt_flags);
	DISP_u32(cp, cp_pack_total_block_count);
	DISP_u32(cp, cp_pack_start_sum);
	DISP_u32(cp, valid_node_count);
	DISP_u32(cp, valid_inode_count);
	DISP_u32(cp, next_free_nid);
	DISP_u32(cp, sit_ver_bitmap_bytesize);
	DISP_u32(cp, nat_ver_bitmap_bytesize);
	DISP_u32(cp, checksum_offset);
	DISP_u64(cp, elapsed_time);

	DISP_u32(cp, sit_nat_version_bitmap[0]);
	printk("\n\n");
}

void f2fs_print_sbi_info(struct f2fs_sb_info *sbi)
{
	if (sbi == NULL || !sbi->print_sbi_safe)
		return;

	f2fs_msg(sbi->sb, KERN_ALERT, "\n");
	f2fs_msg(sbi->sb, KERN_ALERT, "+--------------------------------------------------------+\n");
	f2fs_msg(sbi->sb, KERN_ALERT, "|       SBI(Real time dirty nodes/segments info)         |\n");
	f2fs_msg(sbi->sb, KERN_ALERT, "+--------------------------------------------------------+\n");

	f2fs_msg(sbi->sb, KERN_ALERT, "ndirty_node: %lld\n", get_pages(sbi, F2FS_DIRTY_NODES));
	f2fs_msg(sbi->sb, KERN_ALERT, "ndirty_dent: %lld\n", get_pages(sbi, F2FS_DIRTY_DENTS));
	f2fs_msg(sbi->sb, KERN_ALERT, "ndirty_meta: %lld\n", get_pages(sbi, F2FS_DIRTY_META));
	f2fs_msg(sbi->sb, KERN_ALERT, "ndirty_data: %lld\n", get_pages(sbi, F2FS_DIRTY_DATA));
	f2fs_msg(sbi->sb, KERN_ALERT, "ndirty_files: %d\n", sbi->ndirty_inode[DIR_INODE]);
	f2fs_msg(sbi->sb, KERN_ALERT, "inmem_pages: %lld\n", get_pages(sbi, F2FS_INMEM_PAGES));
	if (likely(SM_I(sbi))) {
		f2fs_msg(sbi->sb, KERN_ALERT, "rsvd_segs: %d\n", reserved_segments(sbi));
		f2fs_msg(sbi->sb, KERN_ALERT, "overp_segs: %d\n", overprovision_segments(sbi));
		if (likely(FREE_I(sbi))) {
			f2fs_msg(sbi->sb, KERN_ALERT, "free_segs: %d\n", free_segments(sbi));
			f2fs_msg(sbi->sb, KERN_ALERT, "free_secs: %d\n", free_sections(sbi));
		}
		if (likely(DIRTY_I(sbi))) {
			f2fs_msg(sbi->sb, KERN_ALERT, "prefree_count: %d\n", prefree_segments(sbi));
			f2fs_msg(sbi->sb, KERN_ALERT, "dirty_count: %d\n", dirty_segments(sbi));
		}
		if (likely(SIT_I(sbi)))
			f2fs_msg(sbi->sb, KERN_ALERT, "dirty_nats: %d\n", SIT_I(sbi)->dirty_sentries);
	}
	f2fs_msg(sbi->sb, KERN_ALERT, "total_count: %d\n", ((unsigned int)sbi->user_block_count)/((unsigned int)sbi->blocks_per_seg));
	f2fs_msg(sbi->sb, KERN_ALERT, "valid_count: %d\n", valid_user_blocks(sbi));
	/*lint -save -e529 -e438*/
	f2fs_msg(sbi->sb, KERN_ALERT, "inline_attr: %d\n", atomic_read(&sbi->inline_xattr));
	f2fs_msg(sbi->sb, KERN_ALERT, "inline_inode: %d\n", atomic_read(&sbi->inline_inode));
	f2fs_msg(sbi->sb, KERN_ALERT, "inline_dir: %d\n", atomic_read(&sbi->inline_dir));
	/*lint -restore*/
	if (likely(sbi->user_block_count))
		f2fs_msg(sbi->sb, KERN_ALERT, "utilization: %d\n", utilization(sbi));
	if (likely(sbi->node_inode && sbi->node_inode->i_mapping))
		f2fs_msg(sbi->sb, KERN_ALERT, "node_pages: %d\n",(int)NODE_MAPPING(sbi)->nrpages);
	if (likely(sbi->meta_inode && sbi->meta_inode->i_mapping))
		f2fs_msg(sbi->sb, KERN_ALERT, "meta_pages: %d\n",(int)META_MAPPING(sbi)->nrpages);
	if (likely(NM_I(sbi)))
		f2fs_msg(sbi->sb, KERN_ALERT, "nats: %d\n", NM_I(sbi)->nat_cnt);

	f2fs_msg(sbi->sb, KERN_ALERT, "segment_count[LFS]: %d\n", sbi->segment_count[0]);
	f2fs_msg(sbi->sb, KERN_ALERT, "segment_count[SSR]: %d\n", sbi->segment_count[1]);

	f2fs_msg(sbi->sb, KERN_ALERT, "block_count[LFS]: %d\n", sbi->block_count[0]);
	f2fs_msg(sbi->sb, KERN_ALERT, "block_count[SSR]: %d\n", sbi->block_count[1]);

	f2fs_msg(sbi->sb, KERN_ALERT, "\n\n");
}

/* Display on console for little endian disk data*/
#define DISP_LE_u16(ptr, member)						\
	do {								\
		printk("F2FS-fs:%-30s" "\t\t[0x%8x : %u]\n",		\
			#member, le16_to_cpu(((ptr)->member)), le16_to_cpu(((ptr)->member)));	\
	} while (0)

#define DISP_LE_u32(ptr, member)						\
	do {								\
		printk("F2FS-fs:%-30s" "\t\t[0x%8x : %u]\n",		\
			#member, le32_to_cpu(((ptr)->member)), le32_to_cpu(((ptr)->member)));	\
	} while (0)

#define DISP_LE_u64(ptr, member)						\
	do {								\
		printk("F2FS-fs:%-30s" "\t\t[0x%8llx : %llu]\n",		\
			#member, le64_to_cpu(((ptr)->member)), le64_to_cpu(((ptr)->member)));	\
	} while (0)

/* print f2fs_inode on disk*/
void f2fs_print_inode(struct f2fs_inode *ri)
{
	if (ri == NULL) {
		return;
	}

	printk("\n");
	printk("+--------------------------------------------------------+\n");
	printk("| F2FS inode dump                                        |\n");
	printk("+--------------------------------------------------------+\n");

	DISP_LE_u16(ri, i_mode);
	DISP_LE_u32(ri, i_uid);
	DISP_LE_u32(ri, i_gid);
	DISP_LE_u32(ri, i_links);
	DISP_LE_u32(ri, i_size);
	DISP_LE_u64(ri, i_blocks);
	DISP_LE_u64(ri, i_atime);
	DISP_LE_u64(ri, i_ctime);
	DISP_LE_u64(ri, i_mtime);
	DISP_LE_u32(ri, i_atime_nsec);
	DISP_LE_u32(ri, i_ctime_nsec);
	DISP_LE_u32(ri, i_mtime_nsec);
	DISP_LE_u32(ri, i_generation);
	DISP_LE_u32(ri, i_current_depth);
	DISP_LE_u32(ri, i_xattr_nid);
	DISP_LE_u32(ri, i_flags);
	DISP_LE_u32(ri, i_pino);
	DISP_LE_u32(ri, i_namelen);
	printk("\n\n");
}
