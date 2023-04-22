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

#ifndef __AMS_TCS3707_H__
#define	__AMS_TCS3707_H__
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

/* Register map */
#define TCS3707_ENABLE_REG              0x80
#define TCS3707_ATIME_REG               0x81
#define TCS3707_PTIME_REG               0x82
#define TCS3707_WTIME_REG               0x83
#define TCS3707_AILTL_REG               0x84
#define TCS3707_AILTH_REG               0x85
#define TCS3707_AIHTL_REG               0x86
#define TCS3707_AIHTH_REG               0x87
#define TCS3707_PILT0L_REG              0x88
#define TCS3707_PILT0H_REG              0x89
#define TCS3707_PILT1L_REG              0x8A
#define TCS3707_PILT1H_REG              0x8B
#define TCS3707_PIHT0L_REG              0x8C
#define TCS3707_PIHT0H_REG              0x8D
#define TCS3707_PIHT1L_REG              0x8E
#define TCS3707_PIHT1H_REG              0x8F
#define TCS3707_AUXID_REG               0x90
#define TCS3707_REVID_REG               0x91
#define TCS3707_ID_REG                  0x92
#define TCS3707_STATUS_REG              0x93
#define TCS3707_ASTATUS_REG             0x94
#define TCS3707_ADATA0L_REG             0x95
#define TCS3707_ADATA0H_REG             0x96
#define TCS3707_ADATA1L_REG             0x97
#define TCS3707_ADATA1H_REG             0x98
#define TCS3707_ADATA2L_REG             0x99
#define TCS3707_ADATA2H_REG             0x9A
#define TCS3707_ADATA3L_REG             0x9B
#define TCS3707_ADATA3H_REG             0x9C
#define TCS3707_ADATA4L_REG             0x9D
#define TCS3707_ADATA4H_REG             0x9E
#define TCS3707_ADATA5L_REG             0x9F
#define TCS3707_ADATA5H_REG             0xA0
#define TCS3707_PDATAL_REG              0xA1
#define TCS3707_PDATAH_REG              0xA2
#define TCS3707_STATUS2_REG             0xA3
#define TCS3707_STATUS3_REG             0xA4
#define TCS3707_STATUS5_REG             0xA6
#define TCS3707_STATUS6_REG             0xA7
#define TCS3707_CFG0_REG                0xA9
#define TCS3707_CFG1_REG                0xAA
#define TCS3707_CFG3_REG                0xAC
#define TCS3707_CFG4_REG                0xAD
#define TCS3707_CFG8_REG                0xB1
#define TCS3707_CFG10_REG               0xB3
#define TCS3707_CFG11_REG               0xB4
#define TCS3707_CFG12_REG               0xB5
#define TCS3707_CFG14_REG               0xB7
#define TCS3707_PCFG1_REG               0xB8
#define TCS3707_PCFG2_REG               0xB9
#define TCS3707_PCFG4_REG               0xBB
#define TCS3707_PCFG5_REG               0xBC
#define TCS3707_PERS_REG                0xBD
#define TCS3707_GPIO_REG                0xBE
#define TCS3707_POFFSETL_REG            0xC7
#define TCS3707_POFFSETH_REG            0xC8
#define TCS3707_ASTEPL_REG              0xCA
#define TCS3707_ASTEPH_REG              0xCB
#define TCS3707_AGC_GAIN_MAX_REG        0xCF
#define TCS3707_PXAVGL_REG              0xD0
#define TCS3707_PXAVGH_REG              0xD1
#define TCS3707_PBSLNL_REG              0xD2
#define TCS3707_PBSLNH_REG              0xD3
#define TCS3707_AZ_CONFIG_REG           0xD6
#define TCS3707_FD_CFG0                 0xD7
#define TCS3707_FD_CFG1                 0xD8
#define TCS3707_FD_CFG3                 0xDA
#define TCS3707_CALIB_REG               0xEA
#define TCS3707_CALIBCFG0_REG           0xEB
#define TCS3707_CALIBCFG1_REG           0xEC
#define TCS3707_CALIBCFG2_REG           0xED
#define TCS3707_CALIBSTAT_REG           0xEEFD_CFG1_MASK
#define TCS3707_INTENAB_REG             0xF9
#define TCS3707_CONTROL_REG             0xFA
#define TCS3707_FIFO_MAP                0xFC
#define TCS3707_FIFO_STATUS             0xFD
#define TCS3707_FDADAL                  0xFE
#define TCS3707_FDADAH                  0xFF

/* Register bits map */
//ENABLE @ 0x80
#define PON                             (0x01 << 0)
#define AEN                             (0x01 << 1)
#define PEN                             (0x01 << 2)
#define WEN                             (0x01 << 3)
#define FDEN                            (0x01 << 6)


//AUXID @ 0x90
#define AUXID_MASK                      (0x0F << 0)

//REVID @ 0x91
#define REVID_MASK                      (0x07 << 0)

//ID_MASK @ 0x92
#define ID_MASK                         (0x3F << 2)

//STATUS @ 0x93
#define SINT                            (0x01 << 0)
#define CINT                            (0x01 << 1)
#define AINT                            (0x01 << 3)
#define PINT0                           (0x01 << 4)
#define PINT1                           (0x01 << 5)
#define PSAT                            (0x01 << 6)
#define ASAT                            (0x01 << 7)

#define AINT_MASK                       (0x01 << 3)
#define ASAT_MASK                       (0x01 << 7)
#define SINT_MASK                       (0x01 << 0)




//ASTATUS @0x94
#define AGAIN_STATUS_MASK               (0x0F << 0)
#define ASAT_STATUS                     (0x01 << 7)

//STATUS2 @0xA3
#define ASAT_ANALOG                     (0x01 << 3)
#define ASAT_DIGITAL                    (0x01 << 4)
#define PVALID                          (0x01 << 5)
#define AVALID                          (0x01 << 6)

//STATUS3 @0xA4
#define PSAT_AMBIENT                    (0x01 << 0)
#define PSAT_REFLECTIVE                 (0x01 << 1)
#define PSAT_ADC                        (0x01 << 2)
#define STATUS3_RVED                    (0x01 << 3)
#define AINT_AILT                       (0x01 << 4)
#define AINT_AIHT                       (0x01 << 5)

//STATUS4 @0xA5
#define PINT0_PILT                      (0x01 << 0)
#define PINT0_PIHT                      (0x01 << 1)
#define PINT1_PILT                      (0x01 << 2)
#define PINT1_PIHT                      (0x01 << 3)

//STATUS6 @0xA7
#define INIT_BUSY                       (0x01 << 0)
#define SAI_ACTIVE                      (0x01 << 1)
#define ALS_TRIGGER_ERROR               (0x01 << 2)
#define PROX_TRIGGER_ERROR              (0x01 << 3)
#define OVTEMP_DETECTED                 (0x01 << 5)

//CFG0 @0xA9
#define ALS_TRIGGER_LONG                (0x01 << 2)
#define PROX_TRIGGER_LONG               (0x01 << 3)
#define LOWPOWER_IDLE                   (0x01 << 5)

//CFG1 @0xAA
#define AGAIN_0_5X                      (0x00 << 0)
#define AGAIN_1X                        (0x01 << 0)
#define AGAIN_2X                        (0x02 << 0)
#define AGAIN_4X                        (0x03 << 0)
#define AGAIN_8X                        (0x04 << 0)
#define AGAIN_16X                       (0x05 << 0)
#define AGAIN_32X                       (0x06 << 0)
#define AGAIN_64X                       (0x07 << 0)
#define AGAIN_128X                      (0x08 << 0)
#define AGAIN_256X                      (0x09 << 0)
#define AGAIN_512X                      (0x0A << 0)
#define AGAIN_MASK                      (0x1F << 0)

//CFG3 @0xAC
#define CFG3_RVED                       (0x0C << 0)
#define SAI                             (0x01 << 4)
#define HXTALK_MODE1                    (0x01 << 5)

//CFG4 @0xAD
#define GPIO_PINMAP_DEFAULT             (0x00 << 0)
#define GPIO_PINMAP_RVED                (0x01 << 0)
#define GPIO_PINMAP_AINT                (0x02 << 0)
#define GPIO_PINMAP_PINT0               (0x03 << 0)
#define GPIO_PINMAP_PINT1               (0x04 << 0)
#define GPIO_PINMAP_MASK                (0x07 << 0)
#define INT_INVERT                      (0x01 << 3)
#define INT_PINMAP_NORMAL               (0x00 << 4)
#define INT_PINMAP_RVED                 (0x01 << 4)
#define INT_PINMAP_AINT                 (0x02 << 4)
#define INT_PINMAP_PINT0                (0x03 << 4)
#define INT_PINMAP_PINT1                (0x04 << 4)
#define INT_PINMAP_MASK                 (0x07 << 4)

//CFG8_REG @0xB1
#define SWAP_PROX_ALS5                  (0x01 << 0)
#define ALS_AGC_ENABLE                  (0x01 << 2)
#define CONCURRENT_PROX_AND_ALS         (0x01 << 4)

//CFG10_REG @0xB3
#define ALS_AGC_LOW_HYST_12_5           (0x00 << 4)
#define ALS_AGC_LOW_HYST_25             (0x01 << 4)
#define ALS_AGC_LOW_HYST_37_5           (0x02 << 4)
#define ALS_AGC_LOW_HYST_50             (0x03 << 4)
#define ALS_AGC_LOW_HYST_MASK           (0x03 << 4)
#define ALS_AGC_HIGH_HYST_50            (0x00 << 6)
#define ALS_AGC_HIGH_HYST_62_5          (0x01 << 6)
#define ALS_AGC_HIGH_HYST_75            (0x02 << 6)
#define ALS_AGC_HIGH_HYST_87_5          (0x03 << 6)
#define ALS_AGC_HIGH_HYST_MASK          (0x03 << 6)

//CFG11_REG @0xB4
#define PINT_DIRECT                     (0x01 << 6)
#define AINT_DIRECT                     (0x01 << 7)

//CFG12_REG @0xB5
#define ALS_TH_CHANNEL_0                (0x00 << 0)
#define ALS_TH_CHANNEL_1                (0x01 << 0)
#define ALS_TH_CHANNEL_2                (0x02 << 0)
#define ALS_TH_CHANNEL_3                (0x03 << 0)
#define ALS_TH_CHANNEL_4                (0x04 << 0)
#define ALS_TH_CHANNEL_MASK             (0x07 << 0)

//CFG14_REG @0xB7
#define PROX_OFFSET_COARSE_MASK         (0x1F << 0)
#define EN_PROX_OFFSET_RANGE            (0x01 << 5)
#define AUTO_CO_CAL_EN                  (0x01 << 6)

//PCFG1_REG @0xB8
#define PROX_FILTER_1                   (0x00 << 0)
#define PROX_FILTER_2                   (0x01 << 0)
#define PROX_FILTER_4                   (0x02 << 0)
#define PROX_FILTER_8                   (0x03 << 0)
#define PROX_FILTER_MASK                (0x03 << 0)
#define PROX_FILTER_DOWNSAMPLE          (0x01 << 2)
#define HXTALK_MODE2                    (0x01 << 7)

//PCFG2_REG @0xB9
#define PLDRIVE0_MASK                   (0x7F << 0)//2xPLDRIVE0 + 4mA

//PCFG4_REG @0xBB
#define PGAIN_1X                        (0x00 << 0)
#define PGAIN_2X                        (0x01 << 0)
#define PGAIN_4X                        (0x02 << 0)
#define PGAIN_8X                        (0x03 << 0)
#define PGAIN_MASK                      (0x03 << 0)

//PCFG5_REG @0xBC
#define PPULSE_MASK                     (0x3F << 0)
#define PPULSE_LEN_4US                  (0x00 << 6)
#define PPULSE_LEN_8US                  (0x01 << 6)
#define PPULSE_LEN_16US                 (0x02 << 6)
#define PPULSE_LEN_32US                 (0x03 << 6)
#define PPULSE_LEN_MASK                 (0x03 << 6)

//PERS_REG @0xBD
#define APERS_MASK                      (0x0F << 0)
#define PPERS_MASK                      (0x0F << 4)

//GPIO_REG @0xBE
#define GPIO_IN                         (0x01 << 0)
#define GPIO_OUT                        (0x01 << 1)
#define GPIO_IN_EN                      (0x01 << 2)
#define GPIO_INVERT                     (0x01 << 3)

//GAIN_MAX_REG @0xCF
#define AGC_AGAIN_MAX_MASK              (0x0F << 0) //2^(AGC_AGAIN_MAX)

//CALIB_REG @0xEA
#define START_OFFSET_CALIB              (0x01 << 0)

//CALIBCFG0_REG @0xEB
#define DCAVG_ITERATIONS_MASK           (0x07 << 0)//0 is skip, 2^(ITERATIONS)
#define BINSRCH_SKIP                    (0x01 << 3)
#define DCAVG_AUTO_OFFSET_ADJUST        (0x01 << 6)
#define DCAVG_AUTO_BSLN                 (0x01 << 7)

//CALIBCFG1_REG @0xEC
#define PXAVG_ITERATIONS_MASK           (0x07 << 0)//0 is skip, 2^(ITERATIONS)
#define PXAVG_AUTO_BSLN                 (0x01 << 3)
#define PROX_AUTO_OFFSET_ADJUST         (0x01 << 6)

//CALIBCFG1_REG @0xED
#define BINSRCH_TARGET_3                (0x00 << 5)
#define BINSRCH_TARGET_7                (0x01 << 5)
#define BINSRCH_TARGET_15               (0x02 << 5)
#define BINSRCH_TARGET_31               (0x03 << 5)
#define BINSRCH_TARGET_63               (0x04 << 5)
#define BINSRCH_TARGET_127              (0x05 << 5)
#define BINSRCH_TARGET_255              (0x06 << 5)
#define BINSRCH_TARGET_511              (0x07 << 5)
#define BINSRCH_TARGET_MASK             (0x07 << 5)

//CALIBSTAT_REG @0xEE
#define CALIB_FINISHED                  (0x01 << 0)
#define OFFSET_ADJUSTED                 (0x01 << 1)
#define BASELINE_ADJUSTED               (0x01 << 2)

//INTENAB_REG @0xF9
#define SIEN                            (0x01 << 0)
#define CIEN                            (0x01 << 1)
#define AIEN                            (0x01 << 3)
#define PIEN0                           (0x01 << 4)
#define PIEN1                           (0x01 << 5)
#define PSIEN                           (0x01 << 6)
#define ASIEN                           (0x01 << 7)

//CONTROL_REG @0xFA
#define CLEAR_FIFO                		(0x01 << 0)
#define ALS_MANUAL_AZ                   (0x01 << 2)


//FD_CFG0@0xD7


//FD_CFG1@0xD8


//FD_CFG3@0xDA
#define FD_GAIN_0_5X                      (0x00 << 3)
#define FD_GAIN_1X                        (0x01 << 3)
#define FD_GAIN_2X                        (0x02 << 3)
#define FD_GAIN_4X                        (0x03 << 3)
#define FD_GAIN_8X                        (0x04 << 3)
#define FD_GAIN_16X                       (0x05 << 3)
#define FD_GAIN_32X                       (0x06 << 3)
#define FD_GAIN_64X                       (0x07 << 3)
#define FD_GAIN_128X                      (0x08 << 3)
#define FD_GAIN_256X                      (0x09 << 3)
#define FD_GAIN_512X                      (0x0A << 3)
#define FD_GAIN_MASK                      (0x1F << 3)
#define FD_TIME_HIGH_3_BIT                (0x07 << 0)




//Configration calculations
#define ASTEP_US_PER_100                278
#define ASTEP_US(us)                    (uint16_t)(((uint32_t)us*100+(ASTEP_US_PER_100 >> 1))/ASTEP_US_PER_100 - 1)
#define PTIME_MS_PER_100                278
#define PTIME_MS(ms)                    (uint8_t)(((uint32_t)ms*100 + (PTIME_MS_PER_100 >> 1)) / PTIME_MS_PER_100 - 1)
#define WTIME_MS_PER_100                278
#define WTIME_MS(ms)                    (uint8_t)(((uint32_t)ms*100 + (WTIME_MS_PER_100 >> 1)) / WTIME_MS_PER_100 - 1)
#define PLDRIVE_MA(ma)                  (uint8_t)((ma-4) >> 1)
#define PPULSES(c)                      (uint8_t)((c - 1))
#define ALS_PERSIST(p)                  (uint8_t)(((p) & 0x0F) << 0)
#define PROX_PERSIST(p)                 (uint8_t)(((p) & 0x0F) << 4)

#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))


#define AMS_TCS3707_DEVICE_ID 0x18
#define AMS_TCS3707_REV_ID    0x11

#define AMS_TCS3707_LEFT_EDGE_DEFAULT		0.1
#define AMS_TCS3707_RIGHT_EDGE_DEFAULT		0.9

#define AMS_TCS3707_K_VALUE_DEFAULT		1.0
#define AMS_TCS3707_K_VALUE_MIN			0.8
#define AMS_TCS3707_K_VALUE_MAX			1.2
#define AMS_TCS3707_NUM_K_VALUE 		5

#define AMS_TCS3707_HIGH_IR_THRESH_DEFAULT	AMS_TCS3707_HIGH_IR_THRESH_MAX
#define AMS_TCS3707_HIGH_IR_THRESH_MIN		0
#define AMS_TCS3707_HIGH_IR_THRESH_MAX		0.5

#define AMS_TCS3707_CLAMP_DEFAULT               AMS_TCS3707_CLAMP_MAX
#define AMS_TCS3707_CLAMP_MIN			0
#define AMS_TCS3707_CLAMP_MAX			1

#define AMS_TCS3707_GAIN_SCALE	  (1000)
#define AMS_TCS3707_ITIME_DEFAULT       50
#define AMS_TCS3707_AGAIN_DEFAULT       (128 * AMS_TCS3707_GAIN_SCALE)
#define AMS_TCS3707_GAIN_OF_GOLDEN       256
#define AMS_TCS3707_FD_GAIN_OF_GOLDEN    128

#define AMS_REPORT_LOG_COUNT_NUM         20

#define AMS_TCS3707_ITIME_FOR_FIRST_DATA       10
#define AMS_TCS3707_AGAIN_FOR_FIRST_DATA       (128 * AMS_TCS3707_GAIN_SCALE)


#define ONE_BYTE_LENGTH_8_BITS  (8)

#define GAIN_QUICKLY_FIX_LEVEL_1 720
#define GAIN_QUICKLY_FIX_LEVEL_2 1440
#define GAIN_QUICKLY_FIX_LEVEL_3 3600


#define AMS_PORT_portHndl   struct i2c_client

#define AMS_TCS3707_LUX_AVERAGE_COUNT    8
#define AMS_TCS3707_CAL_AVERAGE			 1

#define AMS_TCS3707_HIGH    0xFF
#define AMS_TCS3707_LOW     0x00

#define AMS_TCS3707_INTEGRATION_TIME_PER_CYCLE 2778
#define AMS_TCS3707_INTEGRATION_FACTOR         1000
#define AMS_TCS3707_ATIME_TO_MS(x) ((((x + 1) * AMS_TCS3707_INTEGRATION_TIME_PER_CYCLE) / AMS_TCS3707_INTEGRATION_FACTOR))
#define AMS_TCS3707_MS_TO_ATIME(x) ((UINT8)((x * AMS_TCS3707_INTEGRATION_FACTOR) / AMS_TCS3707_INTEGRATION_TIME_PER_CYCLE))
#define AMS_TCS3707_ATIME_TO_CYCLES(x) (x + 1)

#define AMS_TCS3707_MATRIX_ROW_SIZE 3
#define AMS_TCS3707_MATRIX_COL_SIZE 4

#define AMS_TCS3707_DEVICE_OFF_TO_IDLE_MS       10

#define AMS_TCS3707_USEC_PER_TICK               (AMS_TCS3707_INTEGRATION_TIME_PER_CYCLE)
#define AMS_TCS3707_ACTUAL_USEC(x)                  (((x + AMS_TCS3707_USEC_PER_TICK / 2) / AMS_TCS3707_USEC_PER_TICK) * AMS_TCS3707_USEC_PER_TICK)
#define AMS_TCS3707_ALS_USEC_TO_REG(x)          (x / AMS_TCS3707_USEC_PER_TICK)
#define AMS_TCS3707_ADC_COUNT_HIGH 24000
#define AMS_TCS3707_ADC_COUNT_LOW 100

#ifndef AMS_TCS3707_UINT_MAX_VALUE
#define AMS_TCS3707_UINT_MAX_VALUE      (-1)
#endif

#define CHOOSE_GAIN_0         0
#define CHOOSE_GAIN_1         1
#define CHOOSE_GAIN_2         2
#define CHOOSE_GAIN_4         4 
#define CHOOSE_GAIN_8         8//use1
#define CHOOSE_GAIN_16        16
#define CHOOSE_GAIN_32        32 //use4
#define CHOOSE_GAIN_64        64
#define CHOOSE_GAIN_128       128//use16
#define CHOOSE_GAIN_256       256//use64
#define CHOOSE_GAIN_512       512//use128


#define CHOOSE_FD_GAIN_0         0
#define CHOOSE_FD_GAIN_1         1
#define CHOOSE_FD_GAIN_2         2
#define CHOOSE_FD_GAIN_4         4 
#define CHOOSE_FD_GAIN_8         8//use1
#define CHOOSE_FD_GAIN_16        16
#define CHOOSE_FD_GAIN_32        32 //use4
#define CHOOSE_FD_GAIN_64        64
#define CHOOSE_FD_GAIN_128       128//use16
#define CHOOSE_FD_GAIN_256       256//use64
#define CHOOSE_FD_GAIN_512       512//use128

#define FD_POLLING_TIME   100
#define FD_CLOCK_CALIBRATE_TIME   20


#define default_astep_us    1670
#define ASTEP_LOW_BYTE   (uint8_t)(ASTEP_US(default_astep_us) & 0xff)
#define ASTEP_HIGH_BYTE  (uint8_t)((ASTEP_US(default_astep_us) >> 8) & 0xff)

#define fd_gain_default 32
#define fd_time_default_multiply_10  5//the actual fd time is 0.5ms, this multiply 10 to forbid float type, so the frequency is 2000hz
#define fd_time_scale 10
#define fd_ratio_scale 1000


#define AMS_REPORT_DATA_LEN         	(5)
#define AMS_TCS3707_ADC_BYTES		(10)
#define RGBAP_CALI_DATA_NV_NUM       383
#define RGBAP_CALI_DATA_SIZE          96
#define AMS_TCS3707_FLOAT_TO_FIX	(10000)
#define FLOAT_TO_FIX_LOW	         100
#define AMS_TCS3707_CAL_THR           2
#define AP_COLOR_DMD_DELAY_TIME_MS   30000

#define AMS_TCS3707_LOW_LEVEL       100
#define AMS_TCS3707_FD_LOW_LEVEL     50


#ifndef TRUE
#define TRUE    1
#define FALSE   0
#endif

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
//#define AMS_MUTEX_DEBUG

#ifdef AMS_MUTEX_DEBUG
#define AMS_MUTEX_LOCK(m) {						\
		printk(KERN_ERR "%s: AMS Mutex Lock\n", __func__);         \
		mutex_lock(m);						\
	}
#define AMS_MUTEX_UNLOCK(m) {                                           \
		printk(KERN_ERR "%s: AMS Mutex Unlock\n", __func__);       \
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

#define CONFIG_AMS_DEBUG_LOG
#ifdef CONFIG_AMS_DEBUG_LOG
#define AMS_PORT_log(x)                 printk(KERN_ERR "AMS_Driver: " x)
#define AMS_PORT_log_1(x, a)            printk(KERN_ERR "AMS_Driver: " x, a)
#define AMS_PORT_log_2(x, a, b)         printk(KERN_ERR "AMS_Driver: " x, a, b)
#define AMS_PORT_log_3(x, a, b, c)      printk(KERN_ERR "AMS_Driver: " x, a, b, c)
#define AMS_PORT_log_4(x, a, b, c, d)   printk(KERN_ERR "AMS_Driver: " x, a, b, c, d)
#define AMS_PORT_log_5(x, a, b, c, d, e)   printk(KERN_ERR "AMS_Driver: " x, a, b, c, d, e)

#else
#define AMS_PORT_log(x)                 do{ } while (false)
#define AMS_PORT_log_1(x, a)            do{ } while (false)
#define AMS_PORT_log_2(x, a, b)         do{ } while (false)
#define AMS_PORT_log_3(x, a, b, c)      do{ } while (false)
#define AMS_PORT_log_4(x, a, b, c, d)   do{ } while (false)
#define AMS_PORT_log_5(x, a, b, c, d, e)   do{ } while (false)

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

#ifdef CONFIG_AMS_OPTICAL_FLOAT_SUPPORT
struct ams_tcs3707_matrix_data_s {
    float high_ir[AMS_TCS3707_MATRIX_ROW_SIZE][AMS_TCS3707_MATRIX_COL_SIZE];
    float low_ir[AMS_TCS3707_MATRIX_ROW_SIZE][AMS_TCS3707_MATRIX_COL_SIZE];
};
typedef struct ams_tcs3707_matrix_data_s ams_tcs3707_matrix_data_t;
#endif

struct coefficients {
#ifdef CONFIG_AMS_OPTICAL_FLOAT_SUPPORT
    ams_tcs3707_matrix_data_t * matrix;
    float kx;
    float ky;
    float kz;
    float kir1;
    float high_ir_thresh;
    float left_edge;
    float right_edge;
#endif
};
typedef struct coefficients coefficients_t;

#ifdef CONFIG_AMS_OPTICAL_FLOAT_SUPPORT
#if 0
struct cie_tristimulus {
    float x;
    float y;
    float z;
};
#endif
#endif
struct adc_data { /* do not change the order of elements */
    UINT16 c;
    UINT16 r;
    UINT16 g;
    UINT16 b; 
	UINT16 w;
};
typedef struct adc_data ams_tcs3707_adc_data_t;
#if 0
struct als_xyz_data {
    ams_tcs3707_adc_data_t adc;
#ifdef CONFIG_AMS_OPTICAL_FLOAT_SUPPORT
    struct cie_tristimulus tristimulus;
    float chromaticity_x;
    float chromaticity_y;
    float lux;
	float ir;
    UINT32 cct;
#endif
};
typedef struct als_xyz_data ams_tcs3707_als_xyz_data_t;
#endif

typedef enum _amsAlsStatus {
    AMS_3707_ALS_STATUS_IRQ  = (1 << 0),
    AMS_3707_ALS_STATUS_RDY  = (1 << 1),
    AMS_3707_ALS_STATUS_OVFL = (1 << 2)
}ams_tcs3707_AlsStatus_t;

typedef struct _amsAlsDataSet {
    ams_tcs3707_adc_data_t datasetArray;
    UINT32 gain;
    UINT16 itime_ms;
    UINT8 status;
} ams_tcs3707_AlsDataSet_t;

typedef struct _amsAlsContext {
    ams_tcs3707_AlsDataSet_t als_data;
 //   ams_tcs3707_als_xyz_data_t results;
    UINT16 saturation;
} ams_tcs3707_AlsContext_t;

typedef enum _deviceIdentifier_e {
    AMS_UNKNOWN_DEVICE,
    AMS_TCS3707_REV0,   
    AMS_TCS3707_LAST_DEVICE
} ams_tcs3707_deviceIdentifier_e;


typedef struct tcs3707_reg_setting{
    UINT8 reg;
    UINT8 value;
}tcs3707_reg_setting;

 

typedef struct _gainCaliThreshold{
    UINT32 low_thr;
    UINT32 high_thr;
}ams_tcs3707_gainCaliThreshold_t;

typedef struct _3707_config_data {
    UINT32 atime_ms;
    UINT32 gain;
}ams_tcs3707_deviceConfig_t;

typedef enum _3707_feature {
    AMS_TCS3707_FEATURE_ALS = (1 << 0),
    AMS_TCS3707_FEATURE_LAST
}ams_tcs3707_configureFeature_t;

typedef enum _3707_config_options {
    AMS_TCS3707_CONFIG_ENABLE,
    AMS_TCS3707_CONFIG_LAST
}ams_tcs3707_deviceConfigOptions_t;

typedef enum _3707_mode {
    AMS_TCS3707_MODE_OFF            = (0),
    AMS_TCS3707_MODE_ALS            = (1 << 0),
    AMS_TCS3707_MODE_UNKNOWN    /* must be in last position */
} ams_tcs3707_ams_mode_t;

typedef struct _calibrationData {
    UINT32 timeBase_us;
    coefficients_t calibbrationData;
} ams_tcs3707_calibrationData_t;

typedef struct _3707Context {
    ams_tcs3707_deviceIdentifier_e deviceId;
    AMS_PORT_portHndl * portHndl;
    ams_tcs3707_ams_mode_t mode;
    ams_tcs3707_calibrationData_t * systemCalibrationData;
    ams_tcs3707_calibrationData_t defaultCalibrationData;
    ams_tcs3707_AlsContext_t algCtx;
    UINT32 updateAvailable;
	bool first_inte;
	bool first_fd_inte;
}ams_tcs3707_deviceCtx_t;

typedef struct _deviceInfo {
    UINT32    memorySize;
    ams_tcs3707_calibrationData_t defaultCalibrationData;
}ams_tcs3707_deviceInfo_t;



int amsdriver_remove(struct i2c_client *client);
int amsdriver_resume(struct device *dev);
int amsdriver_suspend(struct device *dev);
int amsdriver_probe(struct i2c_client *client,	const struct i2c_device_id *idp);
static bool ams_tcs3707_getDeviceInfo(ams_tcs3707_deviceInfo_t * info);
static bool ams_tcs3707_deviceInit(ams_tcs3707_deviceCtx_t * ctx, AMS_PORT_portHndl * portHndl);
static bool ams_tcs3707_deviceEventHandler(ams_tcs3707_deviceCtx_t * ctx, bool inCalMode);
static ams_tcs3707_deviceIdentifier_e ams_tcs3707_testForDevice(AMS_PORT_portHndl * portHndl);
static bool ams_tcs3707_getDeviceInfo(ams_tcs3707_deviceInfo_t * info);

#ifdef	__cplusplus
}
#endif

#endif
#endif /* __AMS_TCS3707_H__ */
