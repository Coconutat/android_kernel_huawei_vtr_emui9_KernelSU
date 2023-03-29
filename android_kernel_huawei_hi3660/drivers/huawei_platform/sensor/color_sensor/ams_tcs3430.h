/*
*****************************************************************************
* Copyright by ams AG                                                       *
* All rights are reserved.                                                  *
*                                                                           *
* IMPORTANT - PLEASE READ CAREFULLY BEFORE COPYING, INSTALLING OR USING     *
* THE SOFTWARE.                                                             *
*                                                                           *
* THIS SOFTWARE IS PROVIDED FOR USE ONLY IN CONJUNCTION WITH AMS PRODUCTS.  *
* USE OF THE SOFTWARE IN CONJUNCTION WITH NON-AMS-PRODUCTS IS EXPLICITLY    *
* EXCLUDED.                                                                 *
*                                                                           *
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS       *
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT         *
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS         *
* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT  *
* OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,     *
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT          *
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     *
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY     *
* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT       *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE     *
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.      *
*****************************************************************************
*/

/*
 * AMS platform header for the OSAL
 */

/*
 * @@AMS_REVISION_Id:
 */

#ifndef __AMS_TCS3430_H__
#define	__AMS_TCS3430_H__
#include "color_sensor.h"

#ifndef AMS_PLATFORM_H
#define	AMS_PLATFORM_H

#ifdef	__cplusplus
extern "C" {
#endif

#define CONFIG_AMS_LITTLE_ENDIAN 1
#ifdef CONFIG_AMS_LITTLE_ENDIAN
#define AMS_ENDIAN_1    0
#define AMS_ENDIAN_2    8
#else
#define AMS_ENDIAN_2    0
#define AMS_ENDIAN_1    8
#endif

#define AMS_PORT_portHndl   struct i2c_client

#define AMS_TCS3430_LUX_AVERAGE_COUNT    8
#define AMS_TCS3430_CAL_AVERAGE			 1

#define AMS_TCS3430_HIGH    0xFF
#define AMS_TCS3430_LOW     0x00

#define AMS_TCS3430_INTEGRATION_TIME_PER_CYCLE 2778
#define AMS_TCS3430_INTEGRATION_FACTOR         1000
#define AMS_TCS3430_ATIME_TO_MS(x) ((((x + 1) * AMS_TCS3430_INTEGRATION_TIME_PER_CYCLE) / AMS_TCS3430_INTEGRATION_FACTOR))
#define AMS_TCS3430_MS_TO_ATIME(x) ((UINT8)((x * AMS_TCS3430_INTEGRATION_FACTOR) / AMS_TCS3430_INTEGRATION_TIME_PER_CYCLE))
#define AMS_TCS3430_ATIME_TO_CYCLES(x) (x + 1)

#define AMS_TCS3430_MATRIX_ROW_SIZE 3
#define AMS_TCS3430_MATRIX_COL_SIZE 4

#define AMS_TCS3430_DEVICE_OFF_TO_IDLE_MS       10

#define AMS_TCS3430_USEC_PER_TICK               (AMS_TCS3430_INTEGRATION_TIME_PER_CYCLE)
#define AMS_TCS3430_ACTUAL_USEC(x)                  (((x + AMS_TCS3430_USEC_PER_TICK / 2) / AMS_TCS3430_USEC_PER_TICK) * AMS_TCS3430_USEC_PER_TICK)
#define AMS_TCS3430_ALS_USEC_TO_REG(x)          (x / AMS_TCS3430_USEC_PER_TICK)
#define AMS_TCS3430_ADC_COUNT_HIGH 24000
#define AMS_TCS3430_ADC_COUNT_LOW 100

#ifndef AMS_TCS3430_UINT_MAX_VALUE
#define AMS_TCS3430_UINT_MAX_VALUE      (-1)
#endif

#define AMS_TCS3430_DEVICE_ID       0xDC
#define AMS_TCS3430_DEVICE_ID_MASK  0xFC
#define AMS_TCS3430_REV_ID          0x01
#define AMS_TCS3430_REV_ID_MASK     0x07

#define AMS_REPORT_DATA_LEN         	(4)

#define AMS_TCS3430_ADC_BYTES		(8)

#define RGBAP_CALI_DATA_NV_NUM       383
#define RGBAP_CALI_DATA_SIZE          96
#define AMS_TCS3430_FLOAT_TO_FIX	(10000)
#define FLOAT_TO_FIX_LOW	         100
#define AMS_TCS3430_CAL_THR           2
#define AP_COLOR_DMD_DELAY_TIME_MS   30000

#define AMS_TCS3430_LOW_LEVEL       100

#ifndef TRUE
#define TRUE    1
#define FALSE   0
#endif

#ifdef CONFIG_AMS_OPTICAL_FLOAT_SUPPORT
struct ams_tcs3430_matrix_data_s {
    float high_ir[AMS_TCS3430_MATRIX_ROW_SIZE][AMS_TCS3430_MATRIX_COL_SIZE];
    float low_ir[AMS_TCS3430_MATRIX_ROW_SIZE][AMS_TCS3430_MATRIX_COL_SIZE];
};
typedef struct ams_tcs3430_matrix_data_s ams_tcs3430_matrix_data_t;
#endif

struct coefficients {
#ifdef CONFIG_AMS_OPTICAL_FLOAT_SUPPORT
    ams_tcs3430_matrix_data_t * matrix;
    float kx;
    float ky;
    float kz;
    float kir1;
    float high_ir_thresh;
    float left_edge;
    float right_edge;
#endif
    UINT16 nominal_atime;
    UINT8 nominal_again;
};
typedef struct coefficients coefficients_t;

#ifdef CONFIG_AMS_OPTICAL_FLOAT_SUPPORT
struct cie_tristimulus {
    float x;
    float y;
    float z;
};
#endif
struct adc_data { /* do not change the order of elements */
    UINT16 z;
    UINT16 y;
    UINT16 ir1;
    UINT16 x;
    UINT16 ir2;
};
typedef struct adc_data ams_tcs3430_adc_data_t;

struct als_xyz_data {
    ams_tcs3430_adc_data_t adc;
#ifdef CONFIG_AMS_OPTICAL_FLOAT_SUPPORT
    struct cie_tristimulus tristimulus;
    float chromaticity_x;
    float chromaticity_y;
    float lux;
	float ir;
    UINT32 cct;
#endif
};
typedef struct als_xyz_data ams_tcs3430_als_xyz_data_t;

typedef enum _amsAlsStatus {
    AMS_3430_ALS_STATUS_IRQ  = (1 << 0),
    AMS_3430_ALS_STATUS_RDY  = (1 << 1),
    AMS_3430_ALS_STATUS_OVFL = (1 << 2)
}ams_tcs3430_AlsStatus_t;

typedef struct _amsAlsDataSet {
    ams_tcs3430_adc_data_t datasetArray;
    UINT32 gain;
    UINT16 atime_ms;
    UINT8 status;
} ams_tcs3430_AlsDataSet_t;

typedef struct _amsAlsContext {
    ams_tcs3430_AlsDataSet_t als_data;
    ams_tcs3430_als_xyz_data_t results;
    UINT16 saturation;
} ams_tcs3430_AlsContext_t;

typedef enum _deviceIdentifier_e {
    AMS_UNKNOWN_DEVICE,
    AMS_TCS3430_REV0,
    AMS_TCS3430_REV1,
    AMS_TCS3430_LAST_DEVICE
} ams_tcs3430_deviceIdentifier_e;

typedef enum _deviceRegisters {
    AMS_TCS3430_DEVREG_ENABLE,    /* 0x80 */
    AMS_TCS3430_DEVREG_ATIME,
    AMS_TCS3430_DEVREG_WTIME,
    AMS_TCS3430_DEVREG_AILTL,
    AMS_TCS3430_DEVREG_AILTH,
    AMS_TCS3430_DEVREG_AIHTL,
    AMS_TCS3430_DEVREG_AIHTH,
    AMS_TCS3430_DEVREG_PERS,
    AMS_TCS3430_DEVREG_CFG0,
    AMS_TCS3430_DEVREG_CFG1,
    AMS_TCS3430_DEVREG_REVID,
    AMS_TCS3430_DEVREG_ID,
    AMS_TCS3430_DEVREG_STATUS,    /* 0x93 */
    AMS_TCS3430_DEVREG_CH0DATAL,
    AMS_TCS3430_DEVREG_CH0DATAH,
    AMS_TCS3430_DEVREG_CH1DATAL,
    AMS_TCS3430_DEVREG_CH1DATAH,
    AMS_TCS3430_DEVREG_CH2DATAL,
    AMS_TCS3430_DEVREG_CH2DATAH,
    AMS_TCS3430_DEVREG_CH3DATAL,
    AMS_TCS3430_DEVREG_CH3DATAH,
    AMS_TCS3430_DEVREG_CFG2,
    AMS_TCS3430_DEVREG_CFG3,
    AMS_TCS3430_DEVREG_AZ_CONFIG,
    AMS_TCS3430_DEVREG_INTENAB,

    AMS_TCS3430_DEVREG_REG_MAX
}ams_tcs3430_deviceRegister_t;

typedef enum _3430_regOptions {

    PON                = 0x01,  /* register 0x80 */
    AEN                = 0x02,
    WEN                = 0x08,

    WLONG              = 0x04,  /* register 0x8d */

    AMUX               = 0x08,  /* register 0x90 */
    AGAIN_1            = 0x00,
    AGAIN_4            = 0x01,
    AGAIN_16           = 0x02,
    AGAIN_64           = 0x03,

    ASAT               = 0x80,  /* register 0x93 */
    AINT               = 0x10,
    CINT               = 0x08,

    HGAIN              = 0x10,  /* register 0x9F */

    INT_READ_CLEAR     = 0x80,  /* register 0xAB */
    SAI                = 0x10,

    AZ_MODE            = 0x80,  /* register 0xD6 */

    ASIEN              = 0x80,  /* register 0xDD */
    AIEN               = 0x10,

    LAST_IN_ENUM_LIST
}ams_tcs3430_regOptions_t;

typedef enum _3430_regMasks {
    MASK_PON              = 0x01, /* register 0x80 */
    MASK_AEN              = 0x02,
    MASK_WEN              = 0x08,

    MASK_ATIME            = 0xff, /* register 0x81 */

    MASK_WTIME            = 0xff, /* register 0x83 */

    MASK_AILTL            = 0xFF, /* register 0x84 */

    MASK_AILTH            = 0xFF, /* register 0x85 */

    MASK_AIHTL            = 0xFF, /* register 0x86 */

    MASK_AIHTH            = 0xFF, /* register 0x87 */

    MASK_APERS            = 0x0F, /* register 0x8c */

    MASK_WLONG            = 0x04, /* register 0x8d */

    MASK_AMUX             = 0x08, /* register 0x90 */
    MASK_AGAIN            = 0x03,

    MASK_REVID            = 0x07, /* register 0x91 */

    MASK_ID               = 0xFC, /* register 0x92 */

    MASK_ASAT             = 0x80, /* register 0x93 */
    MASK_AINT             = 0x10,
    MASK_CINT             = 0x08,

    MASK_CH0DATAL         = 0xFF, /* register 0x94 */

    MASK_CH0DATAH         = 0xFF, /* register 0x95 */

    MASK_CH1DATAL         = 0xFF, /* register 0x96 */

    MASK_CH1DATAH         = 0xFF, /* register 0x97 */

    MASK_CH2DATAL         = 0xFF, /* register 0x98 */

    MASK_CH2DATAH         = 0xFF, /* register 0x99 */

    MASK_CH3DATAL         = 0xFF, /* register 0x9A */

    MASK_CH3DATAH         = 0xFF, /* register 0x9B */

    MASK_HGAIN            = 0x10, /* register 0x9F */

    MASK_INT_RD_CLEAR     = 0x80, /* register 0xAB */
    MASK_SAI              = 0x10,

    MASK_AZ_MODE          = 0x80, /* register 0xD6 */
    MASK_AZ_NTH_ITERATION = 0x7F,

    MASK_ASIEN            = 0x80, /* register 0xDD */
    MASK_AIEN             = 0x10,

    MASK_LAST_IN_ENUMLIST
}ams_tcs3430_regMask_t;

typedef struct _deviceRegisterTable {
    UINT8 address;
    UINT8 resetValue;
}ams_tcs3430_deviceRegisterTable_t;

typedef struct _gainCaliThreshold{
    UINT32 low_thr;
    UINT32 high_thr;
}ams_tcs3430_gainCaliThreshold_t;

typedef struct _3430_config_data {
    UINT32 atime_ms;
    UINT32 gain;
}ams_tcs3430_deviceConfig_t;

typedef enum _3430_feature {
    AMS_TCS3430_FEATURE_ALS = (1 << 0),
    AMS_TCS3430_FEATURE_LAST
}ams_tcs3430_configureFeature_t;

typedef enum _3430_config_options {
    AMS_TCS3430_CONFIG_ENABLE,
    AMS_TCS3430_CONFIG_LAST
}ams_tcs3430_deviceConfigOptions_t;

typedef enum _3430_mode {
    AMS_TCS3430_MODE_OFF            = (0),
    AMS_TCS3430_MODE_ALS            = (1 << 0),
    AMS_TCS3430_MODE_UNKNOWN    /* must be in last position */
} ams_tcs3430_ams_mode_t;

typedef struct _calibrationData {
    UINT32 timeBase_us;
    coefficients_t calibbrationData;
} ams_tcs3430_calibrationData_t;

typedef struct _3430Context {
    ams_tcs3430_deviceIdentifier_e deviceId;
    AMS_PORT_portHndl * portHndl;
    ams_tcs3430_ams_mode_t mode;
    ams_tcs3430_calibrationData_t * systemCalibrationData;
    ams_tcs3430_calibrationData_t defaultCalibrationData;
    ams_tcs3430_AlsContext_t algCtx;
    UINT32 updateAvailable;
	bool first_inte;
}ams_tcs3430_deviceCtx_t;

typedef struct _deviceInfo {
    UINT32    memorySize;
    ams_tcs3430_calibrationData_t defaultCalibrationData;
}ams_tcs3430_deviceInfo_t;

#define AMSDRIVER_ALS_ENABLE 1
#define AMSDRIVER_ALS_DISABLE 0


#define LOG_ERR             1
#define LOG_WRN             1
#define LOG_INF             1
#define LOG_DBG             1

#define AMS_ERROR           0x00
#define AMS_WARNING         0x01
#define AMS_INFO            0x02
#define AMS_DEBUG           0x03

#ifdef AMS_MUTEX_DEBUG
#define AMS_MUTEX_LOCK(m) {						\
		printk(KERN_INFO "%s: Mutex Lock\n", __func__);         \
		mutex_lock(m);						\
	}
#define AMS_MUTEX_UNLOCK(m) {                                           \
		printk(KERN_INFO "%s: Mutex Unlock\n", __func__);       \
		mutex_unlock(m);					\
	}
#else
#define AMS_MUTEX_LOCK(m) {                           \
		mutex_lock(m);			      \
	}
#define AMS_MUTEX_UNLOCK(m) {                         \
		mutex_unlock(m);		      \
	}
#endif

#ifdef CONFIG_AMS_DEBUG_LOG
#define AMS_PORT_log(x)                 printk(KERN_ERR "AMS_Driver: " x)
#define AMS_PORT_log_1(x, a)            printk(KERN_ERR "AMS_Driver: " x, a)
#define AMS_PORT_log_2(x, a, b)         printk(KERN_ERR "AMS_Driver: " x, a, b)
#define AMS_PORT_log_3(x, a, b, c)      printk(KERN_ERR "AMS_Driver: " x, a, b, c)
#define AMS_PORT_log_4(x, a, b, c, d)   printk(KERN_ERR "AMS_Driver: " x, a, b, c, d)
#else
#define AMS_PORT_log(x)                 do{ } while (false)
#define AMS_PORT_log_1(x, a)            do{ } while (false)
#define AMS_PORT_log_2(x, a, b)         do{ } while (false)
#define AMS_PORT_log_3(x, a, b, c)      do{ } while (false)
#define AMS_PORT_log_4(x, a, b, c, d)   do{ } while (false)
#endif

#define AMS_PORT_log_Msg(t, x)                      { \
        switch(t) \
        { \
        case (AMS_ERROR): if(LOG_ERR == 1) AMS_PORT_log(x); break; \
        case (AMS_WARNING): if(LOG_WRN == 1) AMS_PORT_log(x); break; \
        case (AMS_INFO):  if(LOG_INF == 1) AMS_PORT_log(x); break; \
        case (AMS_DEBUG): if(LOG_DBG == 1) AMS_PORT_log(x); break; \
        } \
}

#define AMS_PORT_log_Msg_1(t, x, a)                 {  \
        switch(t) \
        { \
        case (AMS_ERROR): if(LOG_ERR == 1) AMS_PORT_log_1(x, a); break; \
        case (AMS_INFO):  if(LOG_INF == 1) AMS_PORT_log_1(x, a); break; \
        case (AMS_WARNING): if(LOG_WRN == 1) AMS_PORT_log_1(x, a); break; \
        case (AMS_DEBUG): if(LOG_DBG == 1) AMS_PORT_log_1(x, a); break; \
        } \
}

#define AMS_PORT_log_Msg_2(t, x, a, b)              {  \
        switch(t) \
        { \
        case (AMS_ERROR): if(LOG_ERR == 1) AMS_PORT_log_2(x, a, b); break; \
        case (AMS_INFO):  if(LOG_INF == 1) AMS_PORT_log_2(x, a, b); break; \
        case (AMS_WARNING): if(LOG_WRN == 1) AMS_PORT_log_2(x, a,b); break; \
        case (AMS_DEBUG): if(LOG_DBG == 1) AMS_PORT_log_2(x, a, b); break; \
        } \
}

#define AMS_PORT_log_Msg_3(t, x, a, b, c)           {  \
        switch(t) \
        { \
        case (AMS_ERROR): if(LOG_ERR == 1) AMS_PORT_log_3(x, a, b, c); break; \
        case (AMS_INFO):  if(LOG_INF == 1) AMS_PORT_log_3(x, a, b, c); break; \
        case (AMS_WARNING): if(LOG_WRN == 1) AMS_PORT_log_3(x, a,b, c); break; \
        case (AMS_DEBUG): if(LOG_DBG == 1) AMS_PORT_log_3(x, a, b, c); break; \
        } \
}

#define AMS_PORT_log_Msg_4(t, x, a, b, c, d)        {  \
        switch(t) \
        { \
        case (AMS_ERROR): if(LOG_ERR == 1) AMS_PORT_log_4(x, a, b, c, d); break; \
        case (AMS_INFO):  if(LOG_INF == 1) AMS_PORT_log_4(x, a, b, c, d); break; \
        case (AMS_WARNING): if(LOG_WRN == 1) AMS_PORT_log_4(x, a,b, c, d); break; \
        case (AMS_DEBUG): if(LOG_DBG == 1) AMS_PORT_log_4(x, a, b, c, d); break; \
        } \
}

#define AMS_PORT_get_timestamp_usec(x)

int amsdriver_remove(struct i2c_client *client);
int amsdriver_resume(struct device *dev);
int amsdriver_suspend(struct device *dev);
int amsdriver_probe(struct i2c_client *client,	const struct i2c_device_id *idp);
static bool ams_tcs3430_getDeviceInfo(ams_tcs3430_deviceInfo_t * info);
static bool ams_tcs3430_deviceInit(ams_tcs3430_deviceCtx_t * ctx, AMS_PORT_portHndl * portHndl);
static bool ams_tcs3430_deviceEventHandler(ams_tcs3430_deviceCtx_t * ctx, bool inCalMode);
static ams_tcs3430_deviceIdentifier_e ams_tcs3430_testForDevice(AMS_PORT_portHndl * portHndl);
static bool ams_tcs3430_getDeviceInfo(ams_tcs3430_deviceInfo_t * info);

#ifdef	__cplusplus
}
#endif

#endif
#endif /* __AMS_TCS3430_H__ */
