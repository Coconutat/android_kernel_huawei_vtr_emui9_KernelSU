/*
*****************************************************************************
* Copyright by rohm AG                                                       *
* All rights are reserved.                                                  *
*                                                                           *
* IMPORTANT - PLEASE READ CAREFULLY BEFORE COPYING, INSTALLING OR USING     *
* THE SOFTWARE.                                                             *
*                                                                           *
* THIS SOFTWARE IS PROVIDED FOR USE ONLY IN CONJUNCTION WITH ROHM PRODUCTS.  *
* USE OF THE SOFTWARE IN CONJUNCTION WITH NON-ROHM-PRODUCTS IS EXPLICITLY    *
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
 * ROHM platform header for the OSAL
 */

/*
 * @@ROHM_REVISION_Id:
 */

#ifndef __ROHM_BH1749_H__
#define	__ROHM_BH1749_H__

#include "color_sensor.h"
#ifndef ROHM_PLATFORM_H
#define	ROHM_PLATFORM_H


#ifdef	__cplusplus
extern "C" {
#endif


/* BH1749 REGESTER */
#define ROHM_BH1749_SYSTEMCONTROL      (0x40)
#define ROHM_BH1749_MODECONTROL1       (0x41)
#define ROHM_BH1749_MODECONTROL2       (0x42)
#define ROHM_BH1749_RED_DATA           (0x50)
#define ROHM_BH1749_GREEN_DATA         (0x52)
#define ROHM_BH1749_BLUE_DATA          (0x54)
#define ROHM_BH1749_IR_DATA            (0x58)
#define ROHM_BH1749_GREEN2_DATA        (0x5A)
#define ROHM_BH1749_INTERRUPT          (0x60)
#define ROHM_BH1749_INT_PERSISTENCE    (0x61)
#define ROHM_BH1749_THRED_HIGH         (0x62)
#define ROHM_BH1749_THRED_LOW          (0x64)
#define ROHM_BH1749_MANUFACT_ID        (0x92)
#define ROHM_BH1749_ID                 (0xE0)

#define ROHM_BH1749_INT_SOURCE_GREEN          (1 << 2)  /* Green channel */
#define ROHM_BH1749_INT_ENABLE                (1)       /* INT pin enable */
#define ROHM_BH1749_INT_DISABLE               (0)       /* INT pin disable */
#define ROHM_BH1749_PERSISTENCE_VALUE         (0b10)    /* Interrupt status is updated if 4 consecutive threshold judgments are the same */
#define ROHM_BH1749_INT_STATUS_ACTIVE         (1 << 7)  /* active status */


/************ define parameter for register ************/
#define SW_RESET               (1 << 7)
#define INT_RESET              (1 << 6)

/* REG_MODECONTROL1(0x41) */
#define MEASURE_120MS          (0b010)  //0x2
#define MEASURE_240MS          (0b011)  //0x3
#define MEASURE_35MS           (0b101)  //0x5

#define TIME_120MS                 (120)
#define TIME_240MS                 (240)
#define TIME_35MS                   (30) //(35) 35 is not used for calculate lux, 30 will be used actually


#define RGB_GAIN_SHIFT_VALUE   (3)
#define RGB_GAIN_X1            (0b01 << RGB_GAIN_SHIFT_VALUE)   //0x8
#define RGB_GAIN_X32           (0b11 << RGB_GAIN_SHIFT_VALUE)   //0x18

#define IR_GAIN_SHIFT_VALUE   (5)
#define IR_GAIN_X1              (0b01 << IR_GAIN_SHIFT_VALUE)   //0x20
#define IR_GAIN_X32            (0b11 << IR_GAIN_SHIFT_VALUE)   //0x60

#define GAIN_X1              (1)
#define GAIN_X32            (32)
#define ERR_PARA   (-1)


/* REG_MODECONTROL2(0x42) */
#define RGBC_EN_ON             (1 << 4)  /* RGBC measurement is active */
#define RGBC_EN_OFF            (0 << 4)  /* RGBC measurement is inactive and becomes power down */

#define RGBC_VALID_HIGH        (1 << 7)

/************ definition to dependent on sensor IC ************/
#define BH1749_I2C_ADDRESS     (0x38) //7 bits slave address 011 1001

// GAIN change automaticly according to the current rgb ir raw data
#define AUTO_GAIN  (1)
#define BH1749_GAIN_CHANGE_MAX   (60000)
#define BH1749_GAIN_CHANGE_MIN   (100)
#define TIME_GAIN_MASK           (0xF8)           //reg41 bit0-2 is 0
#define RGB_GAIN_MASK            (0xE7)           //reg41 bit3-4 is 0
#define IR_GAIN_MASK             (0x9F)           //reg41 bit5-6 is 0

#define TIME_GAIN_VALUE(a)            (a & 0x07)   //get bit0-2
#define RGB_GAIN_VALUE(a)            ((a & 0x18) >> RGB_GAIN_SHIFT_VALUE)      //get bit3-4
#define IR_GAIN_VALUE(a)             ((a & 0x60) >> IR_GAIN_SHIFT_VALUE)       //get bit5-6


//max and abs define
#define MY_MAX(a, b)    (((a) > (b)) ? (a) : (b))
#define MY_ABS(a)       (((a)>=0)?(a): (-a))


/* structure to read data value from sensor */
typedef struct {
    unsigned int red;         /* data value of red data from sensor */
    unsigned int green;       /* data value of green data from sensor */
    unsigned int blue;        /* data value of blue data from sensor */
    unsigned int ir;          /* data value of ir data from sensor */
} raw_data_arg_s;



#define CONFIG_ROHM_LITTLE_ENDIAN 1
#ifdef CONFIG_ROHM_LITTLE_ENDIAN
#define ROHM_ENDIAN_1    (0)
#define ROHM_ENDIAN_2    (8)
#else
#define ROHM_ENDIAN_2    (0)
#define ROHM_ENDIAN_1    (8)
#endif

#define ROHM_PORT_portHndl   struct i2c_client

#define ROHM_BH1749_LUX_AVERAGE_COUNT    	(8)
#define ROHM_BH1749_CAL_AVERAGE			 (1)

#define ROHM_BH1749_HIGH    (0xFF)
#define ROHM_BH1749_LOW     (0x00)

#define ROHM_BH1749_INTEGRATION_TIME_PER_CYCLE 	(2778)
#define ROHM_BH1749_INTEGRATION_FACTOR         		(1000)
#define ROHM_BH1749_ATIME_TO_MS(x) ((((x + 1) * ROHM_BH1749_INTEGRATION_TIME_PER_CYCLE) / ROHM_BH1749_INTEGRATION_FACTOR))
#define ROHM_BH1749_MS_TO_ATIME(x) ((UINT8)((x * ROHM_BH1749_INTEGRATION_FACTOR) / ROHM_BH1749_INTEGRATION_TIME_PER_CYCLE))
#define ROHM_BH1749_ATIME_TO_CYCLES(x) (x + 1)

#define ROHM_BH1749_MATRIX_ROW_SIZE (3)
#define ROHM_BH1749_MATRIX_COL_SIZE (4)

#define ROHM_BH1749_DEVICE_OFF_TO_IDLE_MS       (10)

#define ROHM_BH1749_USEC_PER_TICK               (ROHM_BH1749_INTEGRATION_TIME_PER_CYCLE)
#define ROHM_BH1749_ACTUAL_USEC(x)                  (((x + ROHM_BH1749_USEC_PER_TICK / 2) / ROHM_BH1749_USEC_PER_TICK) * ROHM_BH1749_USEC_PER_TICK)
#define ROHM_BH1749_ALS_USEC_TO_REG(x)          (x / ROHM_BH1749_USEC_PER_TICK)
#define ROHM_BH1749_ADC_COUNT_HIGH 		(24000)
#define ROHM_BH1749_ADC_COUNT_LOW 		(100)

#ifndef ROHM_BH1749_UINT_MAX_VALUE
#define ROHM_BH1749_UINT_MAX_VALUE      (-1)
#endif

#define ROHM_BH1749_DEVICE_ID       	 (0xDC)
#define ROHM_BH1749_DEVICE_ID_MASK  (0xFC)
#define ROHM_BH1749_REV_ID         		 (0x01)
#define ROHM_BH1749_REV_ID_MASK     	 (0x07)

#define ROHM_REPORT_DATA_LEN         	(4)

#define ROHM_BH1749_ADC_BYTES		(6)
#define ROHM_BH1749_ADC_IR_BYTES      (2)

#define RGBAP_CALI_DATA_NV_NUM           (383)
#define RGBAP_CALI_DATA_SIZE                 (96)
#define ROHM_BH1749_FLOAT_TO_FIX	 (10000)
#define FLOAT_TO_FIX_LOW	                 (100)
#define ROHM_BH1749_CAL_THR                 (2)
#define AP_COLOR_DMD_DELAY_TIME_MS   (30000)
#define HIGH_THRESHOLD  (100000)
#define LOW_THRESHOLD (1)

#define ROHM_BH1749_LOW_LEVEL        (100)                         //
#define ROHM_BH1749_HIGH_LEVEL       (60000)		     //

#ifndef TRUE
#define TRUE    (1)
#define FALSE   (0)
#endif

#ifdef CONFIG_ROHM_OPTICAL_FLOAT_SUPPORT
struct rohm_bh1749_matrix_data_s {
    float high_ir[ROHM_BH1749_MATRIX_ROW_SIZE][ROHM_BH1749_MATRIX_COL_SIZE];
    float low_ir[ROHM_BH1749_MATRIX_ROW_SIZE][ROHM_BH1749_MATRIX_COL_SIZE];
};
typedef struct rohm_bh1749_matrix_data_s rohm_bh1749_matrix_data_t;
#endif

struct coefficients {
#ifdef CONFIG_ROHM_OPTICAL_FLOAT_SUPPORT
    rohm_bh1749_matrix_data_t * matrix;
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

#ifdef CONFIG_ROHM_OPTICAL_FLOAT_SUPPORT
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
typedef struct adc_data rohm_bh1749_adc_data_t;

struct als_xyz_data {
    rohm_bh1749_adc_data_t adc;
#ifdef CONFIG_ROHM_OPTICAL_FLOAT_SUPPORT
    struct cie_tristimulus tristimulus;
    float chromaticity_x;
    float chromaticity_y;
    float lux;
    float ir;
    UINT32 cct;
#endif
};
typedef struct als_xyz_data rohm_bh1749_als_xyz_data_t;


typedef struct _rohmAlsDataSet {
    rohm_bh1749_adc_data_t datasetArray;
    UINT32 gain;
    UINT32 gain_ir;
    UINT16 atime_ms;
    UINT8 status;
} rohm_bh1749_AlsDataSet_t;

typedef struct _rohmAlsContext {
    rohm_bh1749_AlsDataSet_t als_data;
    rohm_bh1749_als_xyz_data_t results;
    UINT16 saturation;
} rohm_bh1749_AlsContext_t;

typedef enum _deviceIdentifier_e {
    ROHM_UNKNOWN_DEVICE,
    ROHM_BH1749_REV0,
    ROHM_BH1749_REV1,
    ROHM_BH1749_LAST_DEVICE
} rohm_bh1749_deviceIdentifier_e;


typedef enum bh1749_regMasks {
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
}rohm_bh1749_regMask_t;
typedef struct _deviceRegisterTable {
    UINT8 address;
    UINT8 resetValue;
}rohm_bh1749_deviceRegisterTable_t;

typedef struct bh1749_config_data {
    UINT32 atime_ms;
    UINT32 gain;
}rohm_bh1749_deviceConfig_t;

typedef enum bh1749_feature {
    ROHM_BH1749_FEATURE_ALS = (1 << 0),
    ROHM_BH1749_FEATURE_LAST
}rohm_bh1749_configureFeature_t;

typedef enum bh1749_config_options {
    ROHM_BH1749_CONFIG_ENABLE,
    ROHM_BH1749_CONFIG_LAST
}rohm_bh1749_deviceConfigOptions_t;

typedef enum bh1749_mode {
    ROHM_BH1749_MODE_OFF            = (0),
    ROHM_BH1749_MODE_ALS            = (1 << 0),
    ROHM_BH1749_MODE_UNKNOWN    /* must be in last position */
} rohm_bh1749_rohm_mode_t;

typedef struct _calibrationData {
    UINT32 timeBase_us;
    coefficients_t calibbrationData;
} rohm_bh1749_calibrationData_t;

typedef struct bh1749Context {
    rohm_bh1749_deviceIdentifier_e deviceId;
    ROHM_PORT_portHndl * portHndl;
    rohm_bh1749_rohm_mode_t mode;
    rohm_bh1749_calibrationData_t * systemCalibrationData;
    rohm_bh1749_calibrationData_t defaultCalibrationData;
    rohm_bh1749_AlsContext_t algCtx;
    UINT32 updateAvailable;
    bool first_inte;
}rohm_bh1749_deviceCtx_t;

typedef struct _deviceInfo {
    UINT32    memorySize;
    rohm_bh1749_calibrationData_t defaultCalibrationData;
}rohm_bh1749_deviceInfo_t;


#define ROHMDRIVER_ALS_ENABLE 1
#define ROHMDRIVER_ALS_DISABLE 0


#define LOG_ERR             1
#define LOG_WRN             1
#define LOG_INF             1
#define LOG_DBG             1

#define ROHM_ERROR           0x00
#define ROHM_WARNING         0x01
#define ROHM_INFO            0x02
#define ROHM_DEBUG           0x03

#ifdef ROHM_MUTEX_DEBUG
#define ROHM_MUTEX_LOCK(m) {						\
		printk(KERN_INFO "%s: Mutex Lock\n", __func__);         \
		mutex_lock(m);						\
	}
#define ROHM_MUTEX_UNLOCK(m) {                                           \
		printk(KERN_INFO "%s: Mutex Unlock\n", __func__);       \
		mutex_unlock(m);					\
	}
#else
#define ROHM_MUTEX_LOCK(m) {                           \
		mutex_lock(m);			      \
	}
#define ROHM_MUTEX_UNLOCK(m) {                         \
		mutex_unlock(m);		      \
	}
#endif

#ifdef CONFIG_ROHM_DEBUG_LOG
#define ROHM_PORT_log(x)                 printk(KERN_ERR "ROHM_Driver: " x)
#define ROHM_PORT_log_1(x, a)            printk(KERN_ERR "ROHM_Driver: " x, a)
#define ROHM_PORT_log_2(x, a, b)         printk(KERN_ERR "ROHM_Driver: " x, a, b)
#define ROHM_PORT_log_3(x, a, b, c)      printk(KERN_ERR "ROHM_Driver: " x, a, b, c)
#define ROHM_PORT_log_4(x, a, b, c, d)   printk(KERN_ERR "ROHM_Driver: " x, a, b, c, d)
#else
#define ROHM_PORT_log(x)                 do{ } while (false)
#define ROHM_PORT_log_1(x, a)            do{ } while (false)
#define ROHM_PORT_log_2(x, a, b)         do{ } while (false)
#define ROHM_PORT_log_3(x, a, b, c)      do{ } while (false)
#define ROHM_PORT_log_4(x, a, b, c, d)   do{ } while (false)
#endif

#define ROHM_PORT_log_Msg(t, x)                      { \
        switch(t) \
        { \
        case (ROHM_ERROR): if(LOG_ERR == 1) ROHM_PORT_log(x); break; \
        case (ROHM_WARNING): if(LOG_WRN == 1) ROHM_PORT_log(x); break; \
        case (ROHM_INFO):  if(LOG_INF == 1) ROHM_PORT_log(x); break; \
        case (ROHM_DEBUG): if(LOG_DBG == 1) ROHM_PORT_log(x); break; \
        } \
}

#define ROHM_PORT_log_Msg_1(t, x, a)                 {  \
        switch(t) \
        { \
        case (ROHM_ERROR): if(LOG_ERR == 1) ROHM_PORT_log_1(x, a); break; \
        case (ROHM_INFO):  if(LOG_INF == 1) ROHM_PORT_log_1(x, a); break; \
        case (ROHM_WARNING): if(LOG_WRN == 1) ROHM_PORT_log_1(x, a); break; \
        case (ROHM_DEBUG): if(LOG_DBG == 1) ROHM_PORT_log_1(x, a); break; \
        } \
}

#define ROHM_PORT_log_Msg_2(t, x, a, b)              {  \
        switch(t) \
        { \
        case (ROHM_ERROR): if(LOG_ERR == 1) ROHM_PORT_log_2(x, a, b); break; \
        case (ROHM_INFO):  if(LOG_INF == 1) ROHM_PORT_log_2(x, a, b); break; \
        case (ROHM_WARNING): if(LOG_WRN == 1) ROHM_PORT_log_2(x, a,b); break; \
        case (ROHM_DEBUG): if(LOG_DBG == 1) ROHM_PORT_log_2(x, a, b); break; \
        } \
}

#define ROHM_PORT_log_Msg_3(t, x, a, b, c)           {  \
        switch(t) \
        { \
        case (ROHM_ERROR): if(LOG_ERR == 1) ROHM_PORT_log_3(x, a, b, c); break; \
        case (ROHM_INFO):  if(LOG_INF == 1) ROHM_PORT_log_3(x, a, b, c); break; \
        case (ROHM_WARNING): if(LOG_WRN == 1) ROHM_PORT_log_3(x, a,b, c); break; \
        case (ROHM_DEBUG): if(LOG_DBG == 1) ROHM_PORT_log_3(x, a, b, c); break; \
        } \
}

#define ROHM_PORT_log_Msg_4(t, x, a, b, c, d)        {  \
        switch(t) \
        { \
        case (ROHM_ERROR): if(LOG_ERR == 1) ROHM_PORT_log_4(x, a, b, c, d); break; \
        case (ROHM_INFO):  if(LOG_INF == 1) ROHM_PORT_log_4(x, a, b, c, d); break; \
        case (ROHM_WARNING): if(LOG_WRN == 1) ROHM_PORT_log_4(x, a,b, c, d); break; \
        case (ROHM_DEBUG): if(LOG_DBG == 1) ROHM_PORT_log_4(x, a, b, c, d); break; \
        } \
}

#define ROHM_PORT_get_timestamp_usec(x)

int rohmdriver_remove(struct i2c_client *client);
int rohmdriver_resume(struct device *dev);
int rohmdriver_suspend(struct device *dev);
int rohmdriver_probe(struct i2c_client *client,	const struct i2c_device_id *idp);
static bool rohm_bh1749_getDeviceInfo(rohm_bh1749_deviceInfo_t * info);
static void rohm_bh1749_deviceInit(rohm_bh1749_deviceCtx_t * ctx, ROHM_PORT_portHndl * portHndl);
static bool rohm_bh1749_deviceEventHandler(rohm_bh1749_deviceCtx_t * ctx, bool inCalMode);
static UINT8 rohm_bh1749_testForDevice(ROHM_PORT_portHndl * portHndl);
static bool rohm_bh1749_getDeviceInfo(rohm_bh1749_deviceInfo_t * info);

#ifdef	__cplusplus
}
#endif

#endif
#endif /* __ROHM_BH1749_H__ */
