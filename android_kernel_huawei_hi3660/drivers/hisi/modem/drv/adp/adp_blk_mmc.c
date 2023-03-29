
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/statfs.h>

#include <product_config.h>

#include <adrv.h>

/*lint --e{585}*/
/*****************************************************************************
* 函数  : bsp_blk_size
* 功能  : 获取一个分区的大小
* 输入  : part_name 分区名字
* 输出  : size  分区的大小值存放在size
* 返回  : 小于0失败，大于等于0成功
*****************************************************************************/
int bsp_blk_size(const char *part_name, u32 *size)
{/*lint --e{501}*/

	mm_segment_t fs;
	long ret_close, ret;
	unsigned int fd;
	loff_t isize = 0;

	char blk_path[128] = "";

	/* check param */
	if(!part_name || !size)
	{
		printk(KERN_ERR "%s:invalid param.\n", __func__);
		return -1;
	}
	ret = (long)flash_find_ptn(part_name, blk_path);
	if (ret < 0) {
		printk(KERN_ERR "%s not found from partition table!\n", part_name);
		return -1;
	}

	fs = get_fs();
	set_fs((mm_segment_t)KERNEL_DS);

	ret = sys_open(blk_path, O_RDONLY, 0600);
	if (ret < 0) {
		printk(KERN_ERR"fail to open file %s, ret %ld!\n", blk_path, ret);
		goto open_err;
	}

	fd = (unsigned long)ret;

	ret = sys_ioctl(fd, BLKGETSIZE64, (unsigned long)&isize);
	if (ret < 0) {
		printk(KERN_ERR "get %s size is failed, ret %ld!\n",
				blk_path, ret);
		goto ioctl_err;
	}
	*size = (u32)isize;
	ret = 0;
ioctl_err:
	ret_close = sys_close(fd);
	if(ret_close) {
		ret = -1;
        printk(KERN_ERR "%s close failed??, ret %ld\n", blk_path, ret_close);
	}

open_err:
	set_fs(fs);

	return ret;

}

/*****************************************************************************
* 函数  : bsp_blk_read
* 功能  : 读一个分区指定偏移的数据
* 输入  : part_name 分区名字
* 输入  : part_offset 分区偏移
* 输入  : data_len  要读的大小
* 输出  : data_buf  存放读出的数据
* 返回  : 小于0失败，大于等于0成功
*****************************************************************************/
int bsp_blk_read(const char *part_name, loff_t part_offset, void *data_buf, size_t data_len)
{/*lint --e{838,501}*/
	mm_segment_t fs;
	long ret_close, ret, len;
	unsigned int fd = 0;
	loff_t size = 0;

	char blk_path[128] = "";

	/* check param */
	if(!part_name || !data_buf)
	{
		printk(KERN_ERR "%s:invalid param.\n", __func__);
		return -1;
	}

	ret = flash_find_ptn(part_name, blk_path);
	if (ret < 0) {
		printk(KERN_ERR "%s not found from partition table!\n", part_name);
		return -1;
    }
	fs = get_fs();
	set_fs((mm_segment_t)KERNEL_DS);

	ret = sys_open(blk_path, O_RDONLY, 0600);
	if (ret < 0) {
		printk(KERN_ERR"fail to open file %s, ret %ld!\n", blk_path, ret);
		goto open_err;
	}

	fd = (unsigned long)ret;

	ret = sys_ioctl(fd, BLKGETSIZE64, (unsigned long)&size);
	if (ret < 0) {
		printk(KERN_ERR "get %s size is failed, ret %ld!\n",
				blk_path, ret);
		goto ioctl_err;
	}

	if (part_offset > size || (part_offset + (loff_t)data_len > size)) {
		ret = -1;
		printk(KERN_ERR "%s invalid offset %lld data_len %zu size %lld!\n",
				blk_path, part_offset, data_len, size);
		goto ioctl_err;
	}

	ret = sys_lseek(fd, part_offset, SEEK_SET);
	if (ret < 0) {
		printk(KERN_ERR "%s lseek %lld failed, ret %ld!\n",
				blk_path, part_offset, ret);
		goto ioctl_err;
	}

	len = sys_read(fd, data_buf, data_len);
	if (len != data_len)
	{
		ret = -1;
		printk(KERN_ERR "%s read error, data_len %zu read_len %ld!\n",
				blk_path, data_len, len);
		goto ioctl_err;
	}
	ret = 0;

ioctl_err:
	ret_close = sys_close(fd);
	if(ret_close) {
		ret = -1;
        printk(KERN_ERR "%s close failed??, ret %ld\n", blk_path, ret_close);
	}

open_err:
	set_fs(fs);

	return ret;
}

/*****************************************************************************
* 函数  : bsp_blk_write
* 功能  : 对一个分区指定偏移写数据
* 输入  : part_name 分区名字
* 输入  : part_offset 分区偏移
* 输入  : data_len  要写的大小
* 输出  : data_buf  存放要写的数据
* 返回  : 小于0失败，大于等于0成功
*****************************************************************************/
int bsp_blk_write(const char *part_name, loff_t part_offset, void *data_buf, size_t data_len)
{/*lint --e{838, 501}*/
	mm_segment_t fs;
	long ret_close, ret, len;
	unsigned int fd;
	loff_t size = 0;

	char blk_path[128] = "";

	/* check param */
	if(!part_name || !data_buf)
	{
		printk(KERN_ERR "%s:invalid param.\n", __func__);
		return -1;
	}

	ret = flash_find_ptn(part_name, blk_path);
	if (ret < 0) {
		printk(KERN_ERR "%s not found from partition table!\n", part_name);
		return -1;
	}

	fs = get_fs();
	set_fs((mm_segment_t)KERNEL_DS);

	ret = sys_open(blk_path, O_WRONLY | O_DSYNC, 0600);
	if (ret < 0) {
		printk(KERN_ERR"fail to open file %s, ret %ld!\n", blk_path, ret);
		goto open_err;
	}

	fd = (unsigned long)ret;

	ret = sys_ioctl(fd, BLKGETSIZE64, (unsigned long)&size);
	if (ret < 0) {
		printk(KERN_ERR "get %s size is failed, ret %ld!\n",
				blk_path, ret);
		goto ioctl_err;
	}

	if (part_offset > size || (part_offset + (loff_t)data_len > size)) {
		ret = -1;
		printk(KERN_ERR "%s invalid offset %lld data_len %zu size %lld!\n",
				blk_path, part_offset, data_len, size);
		goto ioctl_err;
	}

	ret = sys_lseek(fd, part_offset, SEEK_SET);
	if (ret < 0) {
		printk(KERN_ERR "%s lseek %lld failed, ret %ld!\n",
				blk_path, part_offset, ret);
		goto ioctl_err;
	}

	len = sys_write(fd, data_buf, data_len);
	if (len != data_len)
	{
		ret = -1;
		printk(KERN_ERR "%s read error, data_len %zu read_len %ld!\n",
				blk_path, data_len, len);
		goto ioctl_err;
	}

	ret = sys_fsync(fd);
	if (ret < 0) {
		printk(KERN_ERR "%s fsync failed, ret %ld!\n",
				blk_path, ret);
		goto ioctl_err;
	}
	ret = 0;

ioctl_err:
	ret_close = sys_close(fd);
	if(ret_close) {
		ret = -1;
        printk(KERN_ERR "%s close failed??, ret %ld\n", blk_path, ret_close);
	}

open_err:
	set_fs(fs);

	return ret;

}

int bsp_blk_isbad(const char *partition_name, loff_t partition_offset)
{
	return 0;
}

int bsp_blk_erase(const char *partition_name)
{
	return 0;
}

EXPORT_SYMBOL(bsp_blk_size);
EXPORT_SYMBOL(bsp_blk_read);
EXPORT_SYMBOL(bsp_blk_write);
EXPORT_SYMBOL(bsp_blk_isbad);
EXPORT_SYMBOL(bsp_blk_erase);

