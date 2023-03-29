/*
 * camera utile class driver
 *
 *  Author: 	Zhoujie (zhou.jie1981@163.com)
 *  Date:  	2013/01/16
 *  Version:	1.0
 *  History:	2013/01/16      Frist add driver for dual temperature Led,this is virtual device to manage dual temperature Led
 *
 * ----------------------------------------------------------------------------
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * ----------------------------------------------------------------------------
 *
 */
//lint -save -e846 -e514 -e84 -e866 -e715 -e778 -e713 -e665
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/fs.h>
#include "cam_log.h"
#include <linux/printk.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/rpmsg.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/hisi/hw_cmdline_parse.h>
// Sensor MIPI frequency gear for fore and rear, default: 0
static int fore_frequency_gear = 0;
static int rear_frequency_gear = 0;

typedef  struct  _camerafs_class {
	struct class *classptr;
	struct device *pDevice;
}camerafs_class;

typedef  struct  _camerafs_ois_class {
    struct class *classptr;
    struct device *pDevice;
}camerafs_ois_class;

static camerafs_ois_class camerafs_ois;

static camerafs_class camerafs;

//static int brightness_level = 0;
static dev_t devnum;
static dev_t osi_devnum;

#define CAMERAFS_NODE    "node"
#define CAMERAFS_OIS_NODE "ois"
#define CAMERAFS_ID_MAX 3
#define CAMERAFS_ID_MIN 0

//#define MAX_BRIGHTNESS_FORMMI   (9)

wait_queue_head_t ois_que;
static int ois_check = 0;
#define OIS_TEST_TIMEOUT        ((HZ) * 8)
#ifdef DEBUG_HISI_CAMERA
static int ois_done = 0;
static int cross_width = -1;
static int cross_height = -1;
static int ic_num = -1;
#endif
spinlock_t pix_lock = __SPIN_LOCK_UNLOCKED("camerafs");

#define LDO_NUM_MAX 6
#define LDO_RUN_COUNT 20
#define LDO_NAME_LEN 32
#define LDO_DROP_CURRENT_COUNT 2
enum
{
    REAR_POS = 0,
    FRONT_POS,
    CAM_POS_MAX,
};
typedef  struct  _cam_ldo {
    int ldo_num;
    int ldo_channel[LDO_NUM_MAX];
    int ldo_current[LDO_NUM_MAX];
    int ldo_threshold[LDO_NUM_MAX];
    char ldo_name[LDO_NUM_MAX][LDO_NAME_LEN];
}cam_ldo;
static cam_ldo camerafs_ldo[CAM_POS_MAX];
static int rt_ldo_detect_pos = REAR_POS;
static struct mutex  ldo_lock;
extern int strncpy_s(char *strDest, size_t destMax, const char *strSrc, size_t count);
extern int memset_s(void *dest, size_t destMax, int c, size_t count);
extern int memcpy_s(void *dest, size_t destMax, const void *src, size_t count);
static ssize_t rear_sensor_freq_gear_store(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    int rc = 0;
    rc = sscanf(buf, "%d", &rear_frequency_gear);
    if (rc != 1) {
        rear_frequency_gear = 0; // set default value(0)
        cam_err("%s store rear_frequency_gear error, set default value(0).", __func__);
        return -1;
    }

    if ((rear_frequency_gear < 0) || (rear_frequency_gear > 1)) {
        rear_frequency_gear = 0;
    }
    return count;
}

static ssize_t rear_sensor_freq_gear_show(struct device *dev,
                struct device_attribute *attr, char *buf)
{
    int rc = 0;
    cam_info("%s buf=%s", __func__, buf);
    rc = scnprintf(buf, PAGE_SIZE, "%d\n", rear_frequency_gear);
    return rc;
}

static ssize_t fore_sensor_freq_gear_store(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    int rc = 0;
    rc = sscanf(buf, "%d", &fore_frequency_gear);
    if (rc != 1) {
        fore_frequency_gear = 0; // set default value(0)
        cam_err("%s store fore_frequency_gear error, set default value(0).", __func__);
        return -1;
    }

    if ((fore_frequency_gear < 0) || (fore_frequency_gear > 1)) {
        fore_frequency_gear = 0;
    }
    return count;
}

static ssize_t fore_sensor_freq_gear_show(struct device *dev,
                struct device_attribute *attr, char *buf)
{
    int rc = 0;
    cam_info("%s buf=%s", __func__, buf);
    rc = scnprintf(buf, PAGE_SIZE, "%d\n", fore_frequency_gear);
    return rc;
}

static struct device_attribute fore_sensor_frequency_ctrl =
__ATTR(fore_frequency_node, 0660, fore_sensor_freq_gear_show, fore_sensor_freq_gear_store);
static struct device_attribute rear_sensor_frequency_ctrl =
__ATTR(rear_frequency_node, 0660, rear_sensor_freq_gear_show, rear_sensor_freq_gear_store);

static int thermal_meter[CAMERAFS_ID_MAX];
static ssize_t hw_sensor_thermal_meter_store(struct device *dev,
                                             struct device_attribute *attr, const char *buf, size_t count)
{
     int cam_id = -1;
     int offset = 0;

     offset = sscanf(buf, "%d", &cam_id);
     if(offset > 0 && cam_id < CAMERAFS_ID_MAX && cam_id >= CAMERAFS_ID_MIN) {
          offset = sscanf(buf+offset, "%d", &thermal_meter[cam_id]);
     }

     return count;
}
static ssize_t hw_sensor_thermal_meter_show0(struct device *dev,
                                            struct device_attribute *attr, char *buf)
{
     int ret;
     ret = scnprintf(buf, PAGE_SIZE, "%d", thermal_meter[0]);
     return ret;
}
static ssize_t hw_sensor_thermal_meter_show1(struct device *dev,
                                            struct device_attribute *attr, char *buf)
{
     int ret;
     ret = scnprintf(buf, PAGE_SIZE, "%d",  thermal_meter[1]);
     return ret;
}
static ssize_t hw_sensor_thermal_meter_show2(struct device *dev,
                                            struct device_attribute *attr, char *buf)
{
     int ret;
     ret = scnprintf(buf, PAGE_SIZE, "%d",  thermal_meter[2]);
     return ret;
}

static struct device_attribute sensor_thermal_meter0 =
     __ATTR(thermal_meter0, 0664, hw_sensor_thermal_meter_show0, hw_sensor_thermal_meter_store);
static struct device_attribute sensor_thermal_meter1 =
     __ATTR(thermal_meter1, 0664, hw_sensor_thermal_meter_show1, hw_sensor_thermal_meter_store);
static struct device_attribute sensor_thermal_meter2 =
     __ATTR(thermal_meter2, 0664, hw_sensor_thermal_meter_show2, hw_sensor_thermal_meter_store);
int register_camerafs_ois_attr(struct device_attribute *attr);
#ifdef DEBUG_HISI_CAMERA
static ssize_t hw_ois_aging_store(struct device *dev,
struct device_attribute *attr, const char *buf, size_t count)
{
    int done_flag;
    if(sscanf(buf, "%d", &done_flag) <=0)
    {
        cam_info("write data done_flag error");
        return -1;
    }
    cam_info("%s: done_flag = %d", __func__, done_flag);
    ois_done = done_flag;
    wake_up_interruptible(&ois_que);
    return count;
}
#endif
#ifdef DEBUG_HISI_CAMERA
static ssize_t hw_ois_aging_show(struct device *dev,
struct device_attribute *attr,char *buf)
{
    int ret = -1;

    cam_info("Enter: %s", __func__);
    ois_done = 0;
    //wait for start command
    msleep(50);
    ret = wait_event_interruptible_timeout(ois_que, ois_done != 0, OIS_TEST_TIMEOUT);
    if(ret <= 0) {
        cam_warn("%s: wait ois signal timeout", __func__);
    }
    ret = scnprintf(buf, PAGE_SIZE, "%d", ois_done);

    return ret;
}
#endif
#ifdef DEBUG_HISI_CAMERA
static struct device_attribute hw_ois_aging =
__ATTR(ois_aging, 0664, hw_ois_aging_show, hw_ois_aging_store);
#endif
static ssize_t hw_ois_check_store(struct device *dev,
struct device_attribute *attr, const char *buf, size_t count)
{
    int done_flag;
    if(sscanf(buf, "%d", &done_flag) <= 0)
    {
        cam_info("%s: write data done_flag error", __func__);
        return -1;
    }
    cam_info("%s: done_flag = %d", __func__, done_flag);
    ois_check = done_flag;
    wake_up_interruptible(&ois_que);
    return count;
}

static ssize_t hw_ois_check_show(struct device *dev,
struct device_attribute *attr,char *buf)
{
    int ret = -1;

    cam_info("Enter: %s", __func__);
    ois_check = 0;
    //wait for start command
    msleep(50);
    ret = wait_event_interruptible_timeout(ois_que, ois_check != 0, OIS_TEST_TIMEOUT);
    if(ret <= 0) {
        cam_warn("%s: wait ois signal timeout", __func__);
    }
    ret = scnprintf(buf, PAGE_SIZE, "%d", ois_check);

    return ret;
}

static struct device_attribute hw_ois_check =
__ATTR(ois_check, 0664, hw_ois_check_show, hw_ois_check_store);

// add for ois mmi test
#ifdef DEBUG_HISI_CAMERA
static ssize_t hw_ois_test_mmi_show(struct device *dev,
struct device_attribute *attr,char *buf)
{
    int ret = -1;

    cam_info("Enter: %s", __func__);
    spin_lock(&pix_lock);
    ret = scnprintf(buf, PAGE_SIZE, "%d,%d\n",
            cross_width, cross_height);
    cross_width = -1;
    cross_height = -1;
    spin_unlock(&pix_lock);

    return ret;
}
#endif
#ifdef DEBUG_HISI_CAMERA
static ssize_t hw_ois_test_mmi_store(struct device *dev,
struct device_attribute *attr, const char *buf, size_t count)
{
    int width, height;

    spin_lock(&pix_lock);

    if(sscanf(buf, "%d%d", &width, &height) <= 0)
    {
        cam_info("%s: write data width height error", __func__);
        spin_unlock(&pix_lock);
        return -1;
    }

    cross_width = width;
    cross_height = height;
    spin_unlock(&pix_lock);
    cam_info("Enter: %s (%d, %d).", __func__, cross_width, cross_height);

    return count;
}
#endif
#ifdef DEBUG_HISI_CAMERA
static struct device_attribute hw_ois_pixel =
__ATTR(ois_pixel, 0664, hw_ois_test_mmi_show, hw_ois_test_mmi_store);
#endif
#ifdef DEBUG_HISI_CAMERA
static ssize_t hw_ois_ic_num_show(struct device *dev,
struct device_attribute *attr,char *buf)
{
    int ret = -1;

    cam_info("Enter: %s", __func__);
    spin_lock(&pix_lock);
    ret = scnprintf(buf, PAGE_SIZE, "%d\n", ic_num);
    spin_unlock(&pix_lock);

    return ret;
}
#endif
#ifdef DEBUG_HISI_CAMERA
static ssize_t hw_ois_ic_num_store(struct device *dev,
struct device_attribute *attr, const char *buf, size_t count)
{
    int icnum;

    spin_lock(&pix_lock);

    if(sscanf(buf, "%d", &icnum) <= 0)
    {
        cam_info("%s: write data icnum error", __func__);
        spin_unlock(&pix_lock);
        return -1;
    }

    ic_num = icnum;
    spin_unlock(&pix_lock);
    cam_info("Enter: %s (%d).", __func__, ic_num);

    return count;
}
#endif
#ifdef DEBUG_HISI_CAMERA
static struct device_attribute hw_ois_icnum =
__ATTR(ois_icnum, 0664, hw_ois_ic_num_show, hw_ois_ic_num_store);
#endif

extern int hisi_adc_get_current(int adc_channel);

static ssize_t hw_cam_ldo_detect_show(struct device *dev,
struct device_attribute *attr,char *buf)
{
    cam_ldo *p_ldo = NULL;
    int buflen = 0;
    cam_info("Enter : %s", __func__);
    mutex_lock(&ldo_lock);
    if(buf == NULL){
        cam_err("%s, buf is NULL\n", __func__);
        mutex_unlock(&ldo_lock);
        return -1;
    }
    buflen = (int)sizeof(cam_ldo);
    if(rt_ldo_detect_pos == REAR_POS){
        p_ldo = &(camerafs_ldo[REAR_POS]);
    }else{
        p_ldo = &(camerafs_ldo[FRONT_POS]);
    }
    memcpy_s((cam_ldo *)buf,buflen, p_ldo, buflen);
    mutex_unlock(&ldo_lock);
    cam_info("Exit : %s\n", __func__);
    return buflen;
}

static ssize_t hw_cam_ldo_detect_store(struct device *dev,
struct device_attribute *attr, const char *buf, size_t count)
{
    int i, j= 0;
    int sum_cur = 0;
    int cur_val = 0;
    int drop_count = LDO_DROP_CURRENT_COUNT;
    int min_cur = 0;
    int max_cur = 0;
    cam_ldo *p_ldo = NULL;
    mutex_lock(&ldo_lock);
    if(buf == NULL){
        cam_err("%s, buf is NULL\n", __func__);
        mutex_unlock(&ldo_lock);
        return -1;
    }
    cam_info("Enter: %s\n", __func__);
    if(buf[0] == '0'){
        rt_ldo_detect_pos = REAR_POS;
        p_ldo = &(camerafs_ldo[REAR_POS]);
    }else if(buf[0] == '1'){
        rt_ldo_detect_pos = FRONT_POS;
        p_ldo = &(camerafs_ldo[FRONT_POS]);
    }else{
       cam_err("%s, buf param is error\n", __func__);
        mutex_unlock(&ldo_lock);
       return -1;
    }
    for(i=0; i< p_ldo->ldo_num; i++){
        sum_cur = 0;
        for(j=0; j<LDO_RUN_COUNT; j++){
            cur_val = hisi_adc_get_current(p_ldo->ldo_channel[i]);
            if(cur_val < 0){
                cam_err("%s ldo read data error cur_val = %d\n", __func__, cur_val);
                sum_cur = -1;
                break;
            }else{
                if(j==0) {
                    min_cur = cur_val;
                    max_cur = cur_val;
                }else {
                    if(min_cur > cur_val)
                        min_cur = cur_val;
                    if(max_cur < cur_val)
                        max_cur = cur_val;
                }
                sum_cur = sum_cur + cur_val;
            }
            msleep(5);
        }
        if(sum_cur == -1){
            p_ldo->ldo_current[i] = sum_cur;
        }else{
            p_ldo->ldo_current[i] = (sum_cur-min_cur-max_cur)/(LDO_RUN_COUNT-drop_count);
        }
    }
    mutex_unlock(&ldo_lock);
    cam_info("Exit: %s\n", __func__);    
    return count;
}

static struct device_attribute hw_cam_ldo_detect =
__ATTR(cam_ldo, 0660, hw_cam_ldo_detect_show, hw_cam_ldo_detect_store);

int register_camerafs_attr(struct device_attribute *attr);

static int hw_rt_get_ldo_data(void)
{
    cam_ldo *p_ldo = NULL;
    const char* pldoname = NULL;
    int ret = 0;
    mutex_init(&ldo_lock);
    memset_s(camerafs_ldo, sizeof(cam_ldo)*CAM_POS_MAX, 0, sizeof(cam_ldo)*CAM_POS_MAX);
    if(runmode_is_factory()){
        struct device_node *of_node = NULL;
        int i = 0;
        of_node = of_find_node_by_path("/huawei,camera_ldo");/*lint !e838 */
        if(of_node == NULL){
            return -1;
        }

        p_ldo = &(camerafs_ldo[REAR_POS]);
        p_ldo->ldo_num = of_property_count_elems_of_size(of_node, "rear-ldo-channel", sizeof(u32));
        if(p_ldo->ldo_num > 0){
             ret = of_property_read_u32_array(of_node, "rear-ldo-channel",
                    (u32*)&p_ldo->ldo_channel, p_ldo->ldo_num);
            if (ret < 0) {
                cam_err("%s failed %d\n", __func__, __LINE__);
                return ret;
            }
             ret = of_property_read_u32_array(of_node, "rear-ldo-threshold",
                    (u32*)&p_ldo->ldo_threshold, p_ldo->ldo_num);
            if (ret < 0) {
                cam_err("%s failed %d\n", __func__, __LINE__);
                return ret;
            }
            for (i = 0; i < p_ldo->ldo_num; i++) {
                ret = of_property_read_string_index(of_node, "rear-ldo", i, &pldoname);
                if (ret < 0) {
                    cam_err("%s failed %d\n", __func__, __LINE__);
                    return ret;
                }
                strncpy_s(p_ldo->ldo_name[i], LDO_NAME_LEN-1, pldoname, strlen(pldoname));
            }
        }

        p_ldo = &(camerafs_ldo[FRONT_POS]);
        p_ldo->ldo_num = of_property_count_elems_of_size(of_node, "front-ldo-channel", sizeof(u32));
        if(p_ldo->ldo_num > 0){
             ret = of_property_read_u32_array(of_node, "front-ldo-channel",
                    (u32*)&p_ldo->ldo_channel, p_ldo->ldo_num);
            if (ret < 0) {
                cam_err("%s failed %d\n", __func__, __LINE__);
                return ret;
            }
             ret = of_property_read_u32_array(of_node, "front-ldo-threshold",
                    (u32*)&p_ldo->ldo_threshold, p_ldo->ldo_num);
            if (ret < 0) {
                cam_err("%s failed %d\n", __func__, __LINE__);
                return ret;
            }
            for (i = 0; i < p_ldo->ldo_num; i++) {
                ret = of_property_read_string_index(of_node, "front-ldo", i, &pldoname);
                if (ret < 0) {
                    cam_err("%s failed %d\n", __func__, __LINE__);
                    return ret;
                }
                strncpy_s(p_ldo->ldo_name[i], LDO_NAME_LEN-1, pldoname, strlen(pldoname));
            }
        }
        rt_ldo_detect_pos = REAR_POS;//default detect rear camera
        ret = register_camerafs_attr(&hw_cam_ldo_detect);
    }
    return ret;
}

static int __init camerafs_module_init(void)
{
	int ret;

        camerafs.classptr = NULL;
        camerafs.pDevice = NULL;
        spin_lock_init(&pix_lock);

       ret = alloc_chrdev_region(&devnum, 0, 1, CAMERAFS_NODE);
       ret = alloc_chrdev_region(&osi_devnum, 0, 1, CAMERAFS_OIS_NODE);
       if(ret)
       {
           printk("error %s fail to alloc a dev_t!!!\n",__func__);
           return -1;
       }

	camerafs.classptr= class_create(THIS_MODULE, "camerafs");
    camerafs_ois.classptr = camerafs.classptr;
	if (IS_ERR(camerafs.classptr)) {
		cam_err("class_create failed %d\n", ret);
		ret = PTR_ERR(camerafs.classptr);
		return -1;
	}

	camerafs.pDevice  = device_create(camerafs.classptr, NULL, devnum,NULL,"%s",CAMERAFS_NODE);
    camerafs_ois.pDevice = device_create(camerafs_ois.classptr, NULL, osi_devnum, NULL, "%s", CAMERAFS_OIS_NODE);

	if (IS_ERR(camerafs.pDevice)) {
		cam_err("class_device_create failed %s \n", CAMERAFS_NODE);
		ret = PTR_ERR(camerafs.pDevice);
		return -1;
	}

#ifdef DEBUG_HISI_CAMERA
        register_camerafs_ois_attr(&hw_ois_aging);
        register_camerafs_ois_attr(&hw_ois_pixel);
        register_camerafs_ois_attr(&hw_ois_icnum);
#endif
        register_camerafs_attr(&sensor_thermal_meter0);
        register_camerafs_attr(&sensor_thermal_meter1);
        register_camerafs_attr(&sensor_thermal_meter2);
        register_camerafs_ois_attr(&hw_ois_check);

    // for camera mipi adaption
    register_camerafs_attr(&rear_sensor_frequency_ctrl);
    register_camerafs_attr(&fore_sensor_frequency_ctrl);

    init_waitqueue_head(&ois_que);
    hw_rt_get_ldo_data();
    cam_info("%s end",__func__);
    return 0;
}

int register_camerafs_attr(struct device_attribute *attr)
{
	int ret = 0;

	ret = device_create_file(camerafs.pDevice,attr);
	if (ret<0)
	{
              cam_err("camera fs creat dev attr[%s] fail", attr->attr.name);
		return -1;
	}
       cam_info("camera fs creat dev attr[%s] OK", attr->attr.name);
	return 0;
}

int register_camerafs_ois_attr(struct device_attribute *attr)
{
    int ret = 0;

    ret = device_create_file(camerafs_ois.pDevice,attr);
    if (ret<0)
    {
        cam_err("camera oiscreat dev attr[%s] fail", attr->attr.name);
        return -1;
	}
    cam_info("camera ois creat dev attr[%s] OK", attr->attr.name);
    return 0;
}

EXPORT_SYMBOL(register_camerafs_attr);

static void __exit camerafs_module_deinit(void)
{
	device_destroy(camerafs.classptr, devnum);
    device_destroy(camerafs_ois.classptr, osi_devnum);
	class_destroy(camerafs.classptr);
	unregister_chrdev_region(devnum, 1);
    unregister_chrdev_region(osi_devnum, 1);
}
//lint -restore
module_init(camerafs_module_init);
module_exit(camerafs_module_deinit);
MODULE_AUTHOR("Jiezhou");
MODULE_DESCRIPTION("Camera fs virtul device");
MODULE_LICENSE("GPL");
