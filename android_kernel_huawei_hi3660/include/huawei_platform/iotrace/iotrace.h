
#ifndef _IO_TRACE_H
#define _IO_TRACE_H

#include<linux/tracepoint.h>

DECLARE_TRACE(generic_perform_write_enter,
        TP_PROTO(struct file *file, size_t count, loff_t pos),
        TP_ARGS(file, count, pos));

DECLARE_TRACE(generic_perform_write_end,
        TP_PROTO(struct file *file, size_t count),
        TP_ARGS(file, count));

DECLARE_TRACE(generic_file_read_begin,
        TP_PROTO(struct file *filp, size_t count),
        TP_ARGS(filp, count));

DECLARE_TRACE(generic_file_read_end,
        TP_PROTO(struct file *filp, size_t count),
        TP_ARGS(filp, count));

/*
DECLARE_TRACE(ext4_da_write_begin_enter,
        TP_PROTO(struct inode *inode, loff_t pos, unsigned int len,
              unsigned int flags),
        TP_ARGS(inode, pos, len, flags)
    );
    */
DECLARE_TRACE(ext4_da_write_begin_end,
        TP_PROTO(struct inode *inode, loff_t pos, unsigned int len,
              unsigned int flags),
        TP_ARGS(inode, pos, len, flags)
    );

DECLARE_TRACE(ext4_sync_write_wait_end,
        TP_PROTO(struct file *file, int datasync),
        TP_ARGS(file, datasync)
    );

/*DECLARE_TRACE(ext4_sync_file_begin,
        TP_PROTO(struct file *file, int datasync),
        TP_ARGS(file, datasync)
    );

*/
DECLARE_TRACE(ext4_sync_file_end,
        TP_PROTO(struct file *file, int ret),
        TP_ARGS(file, ret)
    );

DECLARE_TRACE(f2fs_detected_quasi,
        TP_PROTO(struct bio *bio),
        TP_ARGS(bio)
    );

DECLARE_TRACE(block_write_begin_enter,
        TP_PROTO(struct inode *inode, struct page *page, loff_t pos,
            unsigned int len),
        TP_ARGS(inode, page, pos, len)
    );

DECLARE_TRACE(block_write_begin_end,
        TP_PROTO(struct inode *inode, struct page *page, int err),
        TP_ARGS(inode, page, err)
    );

DECLARE_TRACE(mpage_da_map_and_submit,
        TP_PROTO(struct inode *inode, unsigned long long fblk, unsigned int len),
        TP_ARGS(inode, fblk, len)
    );

/*M*/
DECLARE_TRACE(block_crypt_dec_pending,
        TP_PROTO(struct bio *bio),
        TP_ARGS(bio)
    );

DECLARE_TRACE(block_kcryptd_crypt,
        TP_PROTO(struct bio *bio),
        TP_ARGS(bio)
    );

DECLARE_TRACE(block_dm_request,
        TP_PROTO(struct request_queue *q, struct bio *bio),
        TP_ARGS(q, bio)
    );

DECLARE_TRACE(block_crypt_map,
        TP_PROTO(struct bio *bio, sector_t sector),
        TP_ARGS(bio, sector)
    );

DECLARE_TRACE(block_throttle_weight,
        TP_PROTO(struct bio *bio, unsigned int weight, unsigned int nr_queued),
        TP_ARGS(bio, weight, nr_queued)
    );

DECLARE_TRACE(block_throttle_dispatch,
        TP_PROTO(struct bio *bio, unsigned int weight),
        TP_ARGS(bio, weight)
    );

DECLARE_TRACE(block_throttle_iocost,
        TP_PROTO(uint64_t bps, unsigned int iops, uint64_t bytes_disp, unsigned int io_disp),
        TP_ARGS(bps, iops, bytes_disp, io_disp)
    );

DECLARE_TRACE(block_throttle_limit_start,
        TP_PROTO(struct bio *bio, int max_inflights, atomic_t inflights),
        TP_ARGS(bio, max_inflights, inflights)
    );

DECLARE_TRACE(block_throttle_limit_end,
        TP_PROTO(struct bio *bio),
        TP_ARGS(bio)
    );

DECLARE_TRACE(block_throttle_bio_in,
        TP_PROTO(struct bio *bio),
        TP_ARGS(bio)
    );

DECLARE_TRACE(block_throttle_bio_out,
        TP_PROTO(struct bio *bio, long delay),
        TP_ARGS(bio, delay)
    );

DECLARE_TRACE(block_bio_wbt_done,
        TP_PROTO(struct bio *bio),
        TP_ARGS(bio)
    );

#endif
