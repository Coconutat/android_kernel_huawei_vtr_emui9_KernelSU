


#include <linux/compiler.h>
#include <linux/gpio.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <media/huawei/camera.h>
#include <media/v4l2-subdev.h>
#include <linux/hisi/hw_cmdline_parse.h>

#include "hwcam_intf.h"
#include "hwsensor.h"
#include "sensor_commom.h"
#include "cam_log.h"

typedef struct _tag_hwsensor
{
    struct v4l2_subdev                          subdev;
    struct platform_device*                     pdev;

    hwsensor_intf_t*                            intf;
    int                                         cam_dev_num;
    struct mutex                                lock;
    hwcam_data_table_t*                         cfg;
    struct ion_handle*                          cfg_hdl;
} hwsensor_t;

#define SENSOR_POWER_DOWN     0
#define SENSOR_POWER_ON       1

#define dev_to_video_device(i) container_of((i), struct video_device, dev)
#define to_hwsensor_t(i) container_of((i), hwsensor_t, lock)
#define SD2Sensor(sd) container_of(sd, hwsensor_t, subdev)
#define I2S(i) container_of(i, sensor_t, intf)
extern int memset_s(void *dest, size_t destMax, int c, size_t count);
extern int memcpy_s(void *dest, size_t destMax, const void *src, size_t count);
extern int snprintf_s(char* strDest, size_t destMax, size_t count, const char* format, ...);
extern int strncpy_s(char *strDest, size_t destMax, const char *strSrc, size_t count);

//lint -save -e838 -e732 -e747 -e713 -e826 -e715 -e785 -e606 -e774 -esym(753,*)
//lint -save -e578 -e438 -e30 -e142 -e64 -e429 -esym(528,*)
static int hw_sensor_subdev_internal_close(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
    hwsensor_t *s = SD2Sensor(sd);
    int rc=0;
    struct sensor_cfg_data cdata = {0};
    cdata.cfgtype = SEN_CONFIG_POWER_OFF;

    if (s == NULL) {
        cam_err("%s get s_strl error", __func__);
        return -1;
    }
    if (s->intf == NULL || s->intf->vtbl == NULL
        || s->intf->vtbl->config == NULL || s->intf->vtbl->csi_disable == NULL)
        return rc;

    rc = s->intf->vtbl->csi_disable(s->intf);
    rc |= s->intf->vtbl->config(s->intf,(void *)(&cdata));

    cam_notice(" enter %s,return value %d", __func__,rc);
    return rc;
}

static int hwsensor_init(hwsensor_t* s_ctrl)
{

    //sensor_t *sensor = I2S(s_ctrl->intf);
    //to do
    return 0;
}


static int hwsensor_subdev_open(
        struct v4l2_subdev* sd,
        struct v4l2_subdev_fh* fh)
{
    hwsensor_t* s = NULL;
    s = SD2Sensor(sd);
    hwsensor_init(s);
    return 0;
}

static int
hwsensor_subdev_close(
        struct v4l2_subdev* sd,
        struct v4l2_subdev_fh* fh)
{
    hwsensor_t* s = NULL;
    struct ion_handle* hdl = NULL;
    hwcam_data_table_t* cfg = NULL;
    hw_sensor_subdev_internal_close(sd,fh);

    s = SD2Sensor(sd);
    swap(s->cfg_hdl, hdl);
    swap(s->cfg, cfg);
    if (hdl) {
        HWCAM_CFG_ERR("release sensor driver data table! \n");
        hwcam_cfgdev_release_data_table(hdl);
    }

    return 0;
}

static long
hwsensor_subdev_get_info(
        hwsensor_t* s,
        hwsensor_info_t* info)
{
    int index;
    int i=0;
    sensor_t *sensor = NULL;
    if (NULL == s || NULL == info){
        HWCAM_CFG_ERR("s or info is null");
        return -1;
    }

    sensor = I2S(s->intf);
    memset_s(info->name, DEVICE_NAME_SIZE, 0, DEVICE_NAME_SIZE);
    strncpy_s(info->name, DEVICE_NAME_SIZE - 1, hwsensor_intf_get_name(s->intf),
        strlen(hwsensor_intf_get_name(s->intf))+1);
    info->vcm_enable= sensor->board_info->vcm_enable;

    memset(info->extend_name, 0, DEVICE_NAME_SIZE);

    memset(info->vcm_name, 0, DEVICE_NAME_SIZE);
    if(info->vcm_enable) {
        strncpy_s(info->vcm_name, DEVICE_NAME_SIZE -1,sensor->board_info->vcm_name, strlen(sensor->board_info->vcm_name)+1);
    } else {
        memset_s(info->vcm_name, DEVICE_NAME_SIZE, 0, DEVICE_NAME_SIZE);
    }
    info->dev_id = s->cam_dev_num;
    index = sensor->board_info->sensor_index;
    info->mount_position = (hwsensor_position_kind_t)index;
    info->extisp_type = sensor->board_info->extisp_type;
    info->module_type = sensor->board_info->module_type;
    info->flash_pos_type = sensor->board_info->flash_pos_type;
    for (i=0; i<CSI_NUM; i++) {
        info->csi_id[i] = sensor->board_info->csi_id[i];
        info->i2c_id[i] = sensor->board_info->i2c_id[i];
    }
    info->phyinfo_count = 0;
    if (sensor->board_info->phyinfo_valid > 0) {
        info->phyinfo_count = sensor->board_info->phyinfo_valid;
        memcpy(&info->phyinfo, &sensor->board_info->phyinfo, sizeof(info->phyinfo));
    }
    /* for test */

#pragma GCC visibility push(default)
        HWCAM_CFG_ERR("%s, info_count = %d\n, for print, not err.\n"
			"is_master_sensor[0] = %d, is_master_sensor[1] = %d\n"
			"phy_id[0] = %d, phy_id[1] = %d\n"
			"phy_mode[0] = %d, phy_mode[1] = %d\n"
			"phy_freq_mode[0] = %d, phy_freq_mode[1] = %d\n"
			"phy_freq[0] = %d, phy_freq[1] = %d\n"
			"phy_work_mode[0] = %d, phy_work_mode[1] = %d",
			__func__, info->phyinfo_count,
			info->phyinfo.is_master_sensor[0], info->phyinfo.is_master_sensor[1],
			(int)info->phyinfo.phy_id[0], (int)info->phyinfo.phy_id[1],
			info->phyinfo.phy_mode[0], info->phyinfo.phy_mode[1],
			info->phyinfo.phy_freq_mode[0], info->phyinfo.phy_freq_mode[1],
			info->phyinfo.phy_freq[0], info->phyinfo.phy_freq[1],
			info->phyinfo.phy_work_mode[0], info->phyinfo.phy_work_mode[1]);
#pragma GCC visibility pop
    return 0;
}

static long
hwsensor_subdev_mount_buf(
        hwsensor_t* s,
        hwcam_buf_info_t* bi)
{
    long rc = -EINVAL;

    if (NULL == s || NULL == bi) {
        HWCAM_CFG_ERR("s or bi is null");
        return rc;
    }

    switch (bi->kind)
    {
    case HWCAM_BUF_KIND_PIPELINE_PARAM:
        if (!s->cfg) {
            s->cfg = hwcam_cfgdev_import_data_table(
                    "sensor_drv_cfg", bi, &s->cfg_hdl);
            if (s->cfg) { rc = 0; }
        }
        break;

    default:
        HWCAM_CFG_ERR("invalid buffer kind(%d)! \n", bi->kind);
        break;
    }
    return rc;
}

static long
hwsensor_subdev_unmount_buf(
        hwsensor_t* s,
        hwcam_buf_info_t* bi)
{
    long rc = -EINVAL;

    if (NULL == s || NULL == bi) {
        HWCAM_CFG_ERR("s or bi is null");
        return rc;
    }

    switch (bi->kind)
    {
    case HWCAM_BUF_KIND_PIPELINE_PARAM:
        hwcam_cfgdev_release_data_table(s->cfg_hdl);
        s->cfg_hdl = NULL;
        s->cfg = NULL;
        rc = 0;
        break;

    default:
        HWCAM_CFG_ERR("invalid buffer kind(%d)! \n", bi->kind);
        break;
    }
    return rc;
}

static long
hwsensor_subdev_ioctl(
        struct v4l2_subdev *sd,
        unsigned int cmd,
        void *arg)
{
    long rc = -EINVAL;
    hwsensor_t* s = NULL;

    if (NULL == sd) {
        HWCAM_CFG_ERR("sd is NULL \n");
        return -EINVAL;
    }

    s = SD2Sensor(sd);
    cam_debug("hwsensor cmd = %x",cmd);

    switch (cmd)
    {
    case HWSENSOR_IOCTL_GET_INFO:
        rc = hwsensor_subdev_get_info(s, arg);
        break;
    case HWCAM_V4L2_IOCTL_MOUNT_BUF:
        rc = hwsensor_subdev_mount_buf(s, arg);
        break;
    case HWCAM_V4L2_IOCTL_UNMOUNT_BUF:
        rc = hwsensor_subdev_unmount_buf(s, arg);
        break;
    case HWSENSOR_IOCTL_SENSOR_CFG:
        if(NULL != s->intf->vtbl->config)
        {
            rc = s->intf->vtbl->config(s->intf,arg);
        }
        break;
    case HWSENSOR_IOCTL_OTP_CFG:
        if(NULL != s->intf->vtbl->otp_config)
        {
            rc = s->intf->vtbl->otp_config(s->intf,arg);
        }
        break;
     case HWSENSOR_IOCTL_GET_THERMAL:
        if(NULL != s->intf->vtbl->get_thermal)
        {
            rc = s->intf->vtbl->get_thermal(s->intf,arg);
        }
        break;
    default:
        HWCAM_CFG_ERR("invalid IOCTL CMD(%d)! \n", cmd);
        break;
    }
    return rc;
}

static int
hwsensor_power(
        struct v4l2_subdev *sd,
        int on)
{
    return 0;
}

static struct v4l2_subdev_internal_ops
s_subdev_internal_ops_hwsensor =
{
    .open = hwsensor_subdev_open,
    .close = hwsensor_subdev_close,
};

static struct v4l2_subdev_core_ops
s_subdev_core_ops_hwsensor =
{
    .ioctl = hwsensor_subdev_ioctl,
    .s_power = hwsensor_power,
};

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 4, 0))
static int
hwsensor_v4l2_enum_fmt(
        struct v4l2_subdev* sd,
        unsigned int index,
        enum v4l2_mbus_pixelcode* code)
{
    return 0;
}
#endif

static struct v4l2_subdev_video_ops
s_subdev_video_ops_hwsensor =
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 4, 0))
    .enum_mbus_fmt = hwsensor_v4l2_enum_fmt,
#endif
};

static struct v4l2_subdev_ops
s_subdev_ops_hwsensor =
{
    .core = &s_subdev_core_ops_hwsensor,
    .video = &s_subdev_video_ops_hwsensor,
};

static ssize_t hw_sensor_powerctrl_show(struct device *dev,
    struct device_attribute *attr,char *buf)
{
    cam_info("enter %s", __func__);
    return 1;
}

static ssize_t hw_sensor_powerctrl_store(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    struct video_device *devnode = NULL;
    struct mutex *lock = NULL;
    hwsensor_t* sensor = NULL;
    hwsensor_intf_t * intf = NULL;
    int state = 0;
    if (NULL == dev || NULL == attr || NULL == buf) {
        cam_err("%s dev or attr or buf is NULL!", __func__);
        return -EINVAL;
    }
    state = simple_strtol(buf, NULL, 10);
    cam_info("enter %s, state %d", __func__, state);
    devnode = dev_to_video_device(dev);
    if (NULL == devnode) {
        cam_err("%s devnode is NULL!", __func__);
        return -EINVAL;
    }
    lock = devnode->lock;
    if (NULL == lock) {
        cam_err("%s lock is NULL!", __func__);
        return -EINVAL;
    }
    sensor = to_hwsensor_t(lock);
    if (NULL == sensor) {
        cam_err("%s sensor is NULL!", __func__);
        return -EINVAL;
    }
    intf = sensor->intf;
    if (NULL == intf) {
        cam_err("%s intf is NULL", __func__);
        return -EINVAL;
    }
    if (intf->vtbl) {
        int rc = 0;
        if (SENSOR_POWER_ON == state && intf->vtbl->power_up) {
            rc = intf->vtbl->power_up(intf);
            cam_info("%s sensor power up, rc = %d", __func__, rc);
        } else if (SENSOR_POWER_DOWN == state && intf->vtbl->power_down) {
            rc = intf->vtbl->power_down(intf);
            cam_info("%s sensor power down, rc = %d", __func__, rc);
        }
    }
    return count;
}

static struct device_attribute hw_sensor_powerctrl =
    __ATTR(sensor_power_ctrl, 0664, hw_sensor_powerctrl_show, hw_sensor_powerctrl_store);

int hw_camera_register_attribute(hwsensor_intf_t* intf, struct device* dev)
{
    int ret = 0;
    cam_info("enter %s", __func__);
    if (NULL == intf || NULL == dev)
    {
        cam_err("%s intf or dev is NULL.", __func__);
        return -EINVAL;
    }
    ret = device_create_file(dev, &hw_sensor_powerctrl);
    if (ret < 0) {
        cam_err("%s failed to create power ctrl attribute.", __func__);
        return ret;
    }
    return 0;
}

int
hwsensor_register(
        struct platform_device* pdev,
        hwsensor_intf_t* si)
{
    int rc = 0;
    struct v4l2_subdev* subdev = NULL;
    hwsensor_t* sensor = NULL;
    sensor_t* hisensor = NULL;

    if (NULL == pdev || NULL == si) {
        cam_err("%s pdev or si is NULL.", __func__);
        return -EINVAL;
    }

    hisensor = I2S(si);
    if (NULL == hisensor) {
        cam_err("%s hisensor is NULL.", __func__);
        return -EINVAL;
    }

    sensor = (hwsensor_t*)kzalloc(
            sizeof(hwsensor_t), GFP_KERNEL);
    if (sensor == NULL) {
        rc = -ENOMEM;
        goto register_fail;
    }

    subdev = &sensor->subdev;
    mutex_init(&sensor->lock);

    v4l2_subdev_init(subdev, &s_subdev_ops_hwsensor);
    subdev->internal_ops = &s_subdev_internal_ops_hwsensor;
    snprintf_s(subdev->name, sizeof(subdev->name),sizeof(subdev->name)-1,
            "%s", hwsensor_intf_get_name(si));
    subdev->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
    v4l2_set_subdevdata(subdev, pdev);
    platform_set_drvdata(pdev, hisensor);

    init_subdev_media_entity(subdev,HWCAM_SUBDEV_SENSOR);
    hwcam_cfgdev_register_subdev(subdev,HWCAM_SUBDEV_SENSOR);
    subdev->devnode->lock = &sensor->lock;

    hwcam_dev_create(&pdev->dev, &sensor->cam_dev_num);
    sensor->intf = si;
    sensor->pdev = pdev;
    sensor->cfg = NULL;
    sensor->cfg_hdl = NULL;

    if (runmode_is_factory()) //just for factory
    {
        rc = hw_camera_register_attribute(si, &subdev->devnode->dev);
        if (rc < 0) {
            cam_err("%s failed to register camera attribute node.", __func__);
            return rc;
        }
    }
    if (si->vtbl->sensor_register_attribute) {
        rc = si->vtbl->sensor_register_attribute(si, &subdev->devnode->dev);
    }

register_fail:
    return rc;
}

#define Intf2Hwsensor(si) container_of(si, hwsensor_t, intf)
void hwsensor_unregister(hwsensor_intf_t* si)
{
    struct v4l2_subdev* subdev = NULL;
    hwsensor_t* sensor = Intf2Hwsensor(si);

    subdev = &sensor->subdev;
    media_entity_cleanup(&subdev->entity);
    hwcam_cfgdev_unregister_subdev(subdev);

    kzfree(sensor);
}

//lint -restore
