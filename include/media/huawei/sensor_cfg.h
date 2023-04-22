


#ifndef __HW_ALAN_KERNEL_HWCAM_SENSOR_CFG_H__
#define __HW_ALAN_KERNEL_HWCAM_SENSOR_CFG_H__

#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <media/huawei/camera.h>


#define ISO_OTP_DATA_LENGTH            (18)
#define LSC_OTP_DATA_LENGTH            (1402)
#define AF_OTP_DATA_LENGTH             (34)
#define HDC_OTP_DATA_LENGTH            (480)
#define CSI_NUM                        (2)
#define I2C_NUM                        (2)

typedef struct {
    uint8_t   ucYear;
    uint8_t   ucMonth;
    uint8_t   ucDate;
    uint8_t   ucCameraCode;
    uint8_t   ucVenderAndVersion;
    uint8_t   ucCheckSum;
    uint8_t   aucISO_AWBCalib[ISO_OTP_DATA_LENGTH];
    uint8_t   aucLSC[LSC_OTP_DATA_LENGTH];
    uint8_t   aucAF[AF_OTP_DATA_LENGTH];
    uint8_t   aucHDC[HDC_OTP_DATA_LENGTH];
    uint8_t   ucTotalCheckSum;
    uint8_t   ucDataVaild;
    uint8_t   ucHeaderValid;
    uint8_t   ucISOLVaild;
    uint8_t   ucISORValid;
    uint8_t   ucLSCLVaild;
    uint8_t   ucLSCRValid;
    uint8_t   ucVCMLVaild;
    uint8_t   ucVCMRValid;
    uint8_t   ucHDCLValid;
    uint8_t   ucHDCRValid;
}hwsensor_otp_info_t;


enum
{
    DEVICE_NAME_SIZE                          =   32,
};

typedef enum _tag_hwsensor_position_kind
{
    HWSENSOR_POSITION_INVALID                      =    -1,
    HWSENSOR_POSITION_REAR                      =    0,
    HWSENSOR_POSITION_FORE                      =    1,
    HWSENSOR_POSITION_SUBREAR                   =    2,
    HWSENSOR_POSITION_SUBFORE                   =    3,//legacy
    HWSENSOR_POSITION_REAR_THIRD                =    4,
    HWSENSOR_POSITION_REAR_SECOND               =    5,
    HWSENSOR_POSITION_FORE_SECOND               =    7,
    HWSENSOR_POSITION_FORE_THIRD                =    8,
    HWSENSOR_POSITION_IR4STRUCTURELIGHT = 10,
    HWSENSOR_POSITION_IRIS                      =    11,
    HWSENSOR_POSITION_GAZE                      =    12,
} hwsensor_position_kind_t;

typedef enum _tag_hwsensor_flash_pos_type_kind
{//0-alone, 1-mix
    HWSENSOR_FLASH_POS_ALONE                    =    0,
    HWSENSOR_FLASH_POS_MIX                      =    1,
} hwsensor_flash_pos_type_kind_t;

typedef struct _tag_hwsensor_info
{
    uint32_t                                    dev_id;

    char                                        name[DEVICE_NAME_SIZE];
    char                                        vcm_name[DEVICE_NAME_SIZE];
    char                                        extend_name[DEVICE_NAME_SIZE];
    int                                         vcm_enable;
    hwsensor_position_kind_t                    mount_position;
    uint32_t                                    mount_angle;
    int                                         extisp_type;
    int                                         csi_id[CSI_NUM];
    int                                         i2c_id[I2C_NUM];
    int                                         module_type;
    int                                         valid;
    hwsensor_flash_pos_type_kind_t              flash_pos_type;
    int                                         irTopologyType;
    int                                         phyinfo_count;
    phy_info_t                                  phyinfo;
} hwsensor_info_t;

/********************* cfg data define ************************************/
enum sensor_config_type
{
	SEN_CONFIG_POWER_ON = 0,
	SEN_CONFIG_POWER_OFF,
	SEN_CONFIG_WRITE_REG,
	SEN_CONFIG_READ_REG,
	SEN_CONFIG_WRITE_REG_SETTINGS,
	SEN_CONFIG_READ_REG_SETTINGS,
	SEN_CONFIG_ENABLE_CSI,
	SEN_CONFIG_DISABLE_CSI,
	SEN_CONFIG_MATCH_ID,
	SEN_CONFIG_OIS_WPB_CTRL,
	SEN_CONFIG_RESET_HOLD,
	SEN_CONFIG_RESET_RELEASE,
	SEN_CONFIG_MAX_INDEX
};

struct sensor_i2c_reg {
	uint32_t subaddr;
	uint32_t value;
	uint8_t mask;
	uint16_t size;
};

/* add for 32+64 */
struct sensor_i2c_setting {
	union {
		struct sensor_i2c_reg *setting;
		int64_t _setting;
	};
	uint32_t size;
};

/*sensor ioctl arg*/
struct sensor_cfg_data {
    int cfgtype;
    int mode;
    int data;
    int factory;

    union {
        char name[DEVICE_NAME_SIZE];
        struct sensor_i2c_reg reg;
        struct sensor_i2c_setting setting;
        //struct hisi_sensor_af_otp af_otp;
    } cfg;
    union {
         char extend_name[DEVICE_NAME_SIZE];
    }info;
};

typedef enum _tag_sensor_otp_cfg_type_t
{
    SENSOR_CONFIG_OTP_GET = 0,
    SENSOR_CONFIG_OTP_UPDATE,
    SENSOR_CONFIG_OTP_MAX_INDEX,
} sensor_otp_cfg_type_t;

typedef enum {
    IRSENSOR_DEFAULT_OTP = 0,
    IRSENSOR_HOLDER_OTP,
    IRSENSOR_DEVICE_OTP,
    IRSENSOR_ENTIRETY_OTP,
    IRSENSOR_DOT_OTP,
    IRSENSOR_WARPXY_OTP,
} sensor_mode_otp_t;

typedef struct _tag_hwsensor_config_otp_t
{
    unsigned int           cfg_type;//get or update otp
    unsigned int           type;// otp type
    void*                  otp_buffer;
    unsigned int           otp_buffer_size;
} hwsensor_config_otp_t;

typedef struct _sensor_thermal_data{
    int data;  // thermal value
}sensor_thermal_data;

#define HWSENSOR_IOCTL_GET_INFO                         _IOR('S',  BASE_VIDIOC_PRIVATE + 1, hwsensor_info_t)
#define HWSENSOR_IOCTL_SENSOR_CFG 		        _IOWR('V', BASE_VIDIOC_PRIVATE + 2, struct sensor_cfg_data)
#define HWSENSOR_IOCTL_OTP_CFG                          _IOWR('O', BASE_VIDIOC_PRIVATE + 3, hwsensor_config_otp_t)
#define HWSENSOR_IOCTL_GET_THERMAL                      _IOR('S', BASE_VIDIOC_PRIVATE + 2, sensor_thermal_data)

#endif // __HW_ALAN_KERNEL_HWCAM_SENSOR_CFG_H__

