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
 * Input Driver Module
 */

/*
 * @@AMS_REVISION_Id:
 */

#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <linux/unistd.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/pm.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <linux/timer.h>
#include <linux/version.h>
#include <huawei_platform/log/hw_log.h>
#ifdef CONFIG_HUAWEI_DSM
#include <dsm/dsm_pub.h>
#endif
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <huawei_platform/devdetect/hw_dev_dec.h>
#endif
#include "ams_tcs3707.h"

static struct timeval last_time;
static struct timeval curr_time;
UINT8 static fifo_level_last = 0;
UINT8 static fifo_level_curr = 0;
UINT32 static fd_ratio = 1;
#define ONE_BYTE 1

#define HWLOG_TAG color_sensor
HWLOG_REGIST();

static struct class *color_class;
static int report_value[AMS_REPORT_DATA_LEN] = {0};
extern int color_report_val[MAX_REPORT_LEN];

static struct colorDriver_chip *p_chip = NULL;
static bool color_calibrate_result = true;
static bool report_calibrate_result = false;
static at_color_sensor_cali_para_nv color_nv_para;
static int read_nv_first_in = 0;
static int enable_status_before_calibrate = 0;
#ifdef CONFIG_HUAWEI_DSM
static bool color_devcheck_dmd_result = true;
extern struct dsm_client* shb_dclient;
#endif

struct delayed_work ams_tcs3707_dmd_work;
static UINT8 report_logcount = 0;

struct file *file_fd = NULL;

#if defined(CONFIG_AMS_OPTICAL_SENSOR_ALS)

typedef struct{
    UINT32 rawC;
    UINT32 rawR;
    UINT32 rawG;
    UINT32 rawB;
    UINT32 rawW;
}export_alsData_t;

typedef struct color_sensor_input_tcs3707 {
	UINT32 enable;
	UINT32 tar_c;
	UINT32 tar_r;
	UINT32 tar_g;
	UINT32 tar_b;
	UINT32 tar_w;
	UINT32 reserverd[3];
}color_sensor_input_para_tcs3707;

static UINT8 const tcs3707_ids[] = {
	0x18,
};

tcs3707_reg_setting default_setting[] = {	
	{TCS3707_ATIME_REG,   0x1d},//default atime is 29
	{TCS3707_CFG0_REG,    0x00},//reserverd
	{TCS3707_CFG1_REG,	  AGAIN_128X},//default again is 128X
	{TCS3707_CFG3_REG,    0x0C},//reserverd
	{TCS3707_CFG4_REG,    0x80},//use fd_gain as the adata5's gain value,, not again
	{TCS3707_CFG8_REG,    0x98},//bit3 is set to disable flicker AGC, bit2 is 0 means disable ALS AGC
	{TCS3707_CFG10_REG,   0xf2},//reserverd
	{TCS3707_CFG11_REG,   0x40},//reserverd
	{TCS3707_CFG12_REG,   0x00},//reserverd
	{TCS3707_CFG14_REG,   0x00},//reserverd
	{TCS3707_PERS_REG,    0x00},//reserverd
	{TCS3707_GPIO_REG,    0x02},//reserverd
	{TCS3707_ASTEPL_REG,  ASTEP_LOW_BYTE},//default astep is 1.67ms
	{TCS3707_ASTEPH_REG,  ASTEP_HIGH_BYTE},
	{TCS3707_AGC_GAIN_MAX_REG,0x99},//reserverd
	{TCS3707_AZ_CONFIG_REG, 0xff},//reserverd
	{TCS3707_FD_CFG0, 0x80},//enable fd_fifo_mode
	{TCS3707_FD_CFG1, 0xb4},//default fd_time = 0.5ms so the sampel rate is 2000hz
	{TCS3707_FD_CFG3, 0x30},// default fd_gain = 32x	
	{TCS3707_FIFO_MAP,0x00} //reserverd
};


static UINT32 const ams_tcs3707_alsGain_conversion[] = {
//	1 * (AMS_TCS3707_GAIN_SCALE/2),//first gain actual is 0.5, so devided by 2 here
//	1 * AMS_TCS3707_GAIN_SCALE,
//	2 * AMS_TCS3707_GAIN_SCALE,
//	4 * AMS_TCS3707_GAIN_SCALE,
	8 * AMS_TCS3707_GAIN_SCALE,
//	16 * AMS_TCS3707_GAIN_SCALE,
	32 * AMS_TCS3707_GAIN_SCALE,
//	64 * AMS_TCS3707_GAIN_SCALE,
	128 * AMS_TCS3707_GAIN_SCALE,
	256 * AMS_TCS3707_GAIN_SCALE,
	512 * AMS_TCS3707_GAIN_SCALE
};

static UINT32 const ams_tcs3707_als_gains[] = {
//	0,//actual gain is 0.5
//	1,
//	2,
//	4,
	8,
//	16,
	32,
//	64,
	128,
	256,
	512
};

static UINT16 const ams_tcs3707_als_gain_values[] = {
	0,//actual gain is 0.5
	1,
	2,
	4,
	8,
	16,
	32,
	64,
	128,
	256,
	512
};

const ams_tcs3707_gainCaliThreshold_t  ams_tcs3707_setGainThreshold[CAL_STATE_GAIN_LAST] = {
	{(AMS_TCS3707_FLOAT_TO_FIX/AMS_TCS3707_CAL_THR), (AMS_TCS3707_FLOAT_TO_FIX*AMS_TCS3707_CAL_THR)},//set threshold 1x to 0~100
	{(AMS_TCS3707_FLOAT_TO_FIX/AMS_TCS3707_CAL_THR), (AMS_TCS3707_FLOAT_TO_FIX*AMS_TCS3707_CAL_THR)},
	{(AMS_TCS3707_FLOAT_TO_FIX/AMS_TCS3707_CAL_THR), (AMS_TCS3707_FLOAT_TO_FIX*AMS_TCS3707_CAL_THR)},
	{(AMS_TCS3707_FLOAT_TO_FIX/AMS_TCS3707_CAL_THR), (AMS_TCS3707_FLOAT_TO_FIX*AMS_TCS3707_CAL_THR)},
	{(AMS_TCS3707_FLOAT_TO_FIX/AMS_TCS3707_CAL_THR), (AMS_TCS3707_FLOAT_TO_FIX*AMS_TCS3707_CAL_THR)},	
};
 

static bool ams_tcs3707_getDeviceInfo(ams_tcs3707_deviceInfo_t * info);
static bool ams_tcs3707_deviceInit(ams_tcs3707_deviceCtx_t * ctx, AMS_PORT_portHndl * portHndl);
static bool ams_tcs3707_deviceEventHandler(ams_tcs3707_deviceCtx_t * ctx, bool inCalMode);
static ams_tcs3707_deviceIdentifier_e ams_tcs3707_testForDevice(AMS_PORT_portHndl * portHndl);
static bool ams_tcs3707_getDeviceInfo(ams_tcs3707_deviceInfo_t * info);
extern int ap_color_report(int value[], int length);
extern int color_register(struct colorDriver_chip *chip);
extern int read_color_calibrate_data_from_nv(int nv_number, int nv_size, char * nv_name, char * temp);
extern int write_color_calibrate_data_to_nv(int nv_number, int nv_size, char * nv_name, char * temp);
extern int (*color_default_enable)(bool enable);


int AMS_PORT_TCS3707_getByte(AMS_PORT_portHndl * handle, uint8_t reg, uint8_t * data, uint8_t len){
    int ret = 0;
    if ((handle == NULL) || (data == NULL )){
	    hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
	    return -EPERM;
    }

    ret = i2c_smbus_read_i2c_block_data(handle, reg, len, data);
    if (ret < 0)
	    hwlog_err("%s: failed at address %x (%d bytes)\n",__func__, reg, len);

    return ret;
}

int AMS_PORT_TCS3707_setByte(AMS_PORT_portHndl * handle, uint8_t reg, uint8_t* data, uint8_t len){
	int ret = 0;
	if ((handle == NULL) || (data == NULL) ){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
	    return -EPERM;
	}

	ret = i2c_smbus_write_i2c_block_data(handle, reg, len, data);
	if (ret < 0)
	    	hwlog_err("%s: failed at address %x (%d bytes)\n", __func__, reg, len);

	return ret;
}

static int ams_tcs3707_getByte(AMS_PORT_portHndl * portHndl, UINT8 reg, UINT8 * readData)
{
	int read_count = 0;

	if (portHndl == NULL || readData == NULL){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return read_count;
	}
	read_count = AMS_PORT_TCS3707_getByte(portHndl,reg, readData,ONE_BYTE);
	//AMS_PORT_log_2("I2C reg getByte = 0x%02x, data: 0x%02x\n", reg,(UINT8)readData[0]);

	return read_count;
}

static int ams_tcs3707_setByte(AMS_PORT_portHndl * portHndl, UINT8 reg, UINT8 data)
{
	int write_count = 0;

	if (portHndl == NULL ){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return write_count;
	}

	write_count = AMS_PORT_TCS3707_setByte(portHndl,reg,&data,ONE_BYTE);
	//AMS_PORT_log_2("I2C reg setByte = 0x%02x, data: 0x%02x\n", reg, (UINT8)data);
	return write_count;
}

static int ams_tcs3707_getBuf(AMS_PORT_portHndl * portHndl, UINT8 reg, UINT8 * readData, UINT8 length)
{
	int read_count = 0;

	if ((portHndl == NULL) || (readData == NULL) ){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return read_count;
	}

	read_count = AMS_PORT_TCS3707_getByte(portHndl,reg,readData,length);	 
	return read_count;
}

static int ams_tcs3707_report_data(int value[])
{
	hwlog_debug("ams_tcs3707_report_data\n");
	return ap_color_report(value, AMS_REPORT_DATA_LEN*sizeof(int));
}

static int ams_tcs3707_getField(AMS_PORT_portHndl * portHndl, UINT8 reg, UINT8 * readData, UINT8 mask)
{
	int read_count = 0;

	if ((portHndl == NULL) || (readData == NULL) ){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return read_count;
	}

	read_count = AMS_PORT_TCS3707_getByte(portHndl,reg, readData,ONE_BYTE);
	*readData &= mask;
	return read_count;
}

static int ams_tcs3707_setField(AMS_PORT_portHndl * portHndl, UINT8 reg, UINT8 data, UINT8 mask)
{
	int write_count = 0;
	UINT8 original_data = 0, new_data = 0;

	if (portHndl == NULL){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return 0;
	}

	ams_tcs3707_getByte(portHndl, reg, &original_data);

	new_data = original_data & ~mask;
	new_data |= (data & mask);

	if (new_data != original_data){
	    write_count = ams_tcs3707_setByte(portHndl,reg,new_data);
	}

	return write_count;
}

static ams_tcs3707_deviceIdentifier_e ams_tcs3707_testForDevice(AMS_PORT_portHndl * portHndl)
{
	UINT8 chipId = 0,  i = 0;
	if (portHndl == NULL){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return AMS_UNKNOWN_DEVICE;
	}
	ams_tcs3707_getByte(portHndl, TCS3707_ID_REG, &chipId);
	
	hwlog_warn("ams_tcs3707_testForDevice: chipId = 0x%02x \n", chipId);
	for(i = 0; i < ARRAY_SIZE(tcs3707_ids);i++){
		if(chipId == tcs3707_ids[i]){
			return AMS_TCS3707_REV0;
		}
	}

	return AMS_UNKNOWN_DEVICE;	
}

static void ams_tcs3707_resetAllRegisters(AMS_PORT_portHndl * portHndl)
{
	UINT8 i = 0;
	if (portHndl == NULL){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return;
	}

	for(i=0; i<ARRAY_SIZE(default_setting);i++){
		ams_tcs3707_setByte(portHndl,default_setting[i].reg, default_setting[i].value);
	}	
	hwlog_info("ASTEP_LOW_BYTE = %d,ASTEP_HIGH_BYTE = %d, ASTEP_US(default_astep_us) = %d\n",
		ASTEP_LOW_BYTE,ASTEP_HIGH_BYTE,ASTEP_US(default_astep_us));
	hwlog_warn("_3707_resetAllRegisters\n");
}

static UINT8 ams_tcs3707_gainToReg(UINT32 x)
{
	UINT8 i = 0;

	for (i = sizeof(ams_tcs3707_alsGain_conversion)/sizeof(UINT32)-1; i != 0; i--) {
	    	if (x >= ams_tcs3707_alsGain_conversion[i]) break;
	}
	//hwlog_warn("ams_tcs3707_gainToReg: %d %d\n", x, i);
	return (i);
}


static INT32 ams_tcs3707_getGain(ams_tcs3707_deviceCtx_t * ctx)
{
	UINT8 cfg1_reg_data = 0;	 
	INT32 gain = 0;

	if (ctx == NULL){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return true;
	}

	ams_tcs3707_getField(ctx->portHndl, TCS3707_CFG1_REG, &cfg1_reg_data, AGAIN_MASK);
	
	if(cfg1_reg_data <= 10){
		gain = ams_tcs3707_als_gain_values[cfg1_reg_data];
	}
	return gain;
}

static INT32 ams_tcs3707_setGain(ams_tcs3707_deviceCtx_t * ctx, uint32_t gain)
{
	INT32 ret = 0;
	UINT8 cfg1 = 0;

	if (ctx == NULL){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return true;
	}	

	switch (gain)
	{
	case 0:
		cfg1 = AGAIN_0_5X;
		break;
	case 1*AMS_TCS3707_GAIN_SCALE:
		cfg1 = AGAIN_1X;
		break;
	case 2*AMS_TCS3707_GAIN_SCALE:
		cfg1 = AGAIN_2X;
		break;
	case 4*AMS_TCS3707_GAIN_SCALE:
		cfg1 = AGAIN_4X;
		break;
	case 8*AMS_TCS3707_GAIN_SCALE:
		cfg1 = AGAIN_8X;
		break;
	case 16*AMS_TCS3707_GAIN_SCALE:
		cfg1 = AGAIN_16X;
		break;
	case 32*AMS_TCS3707_GAIN_SCALE:
		cfg1 = AGAIN_32X;
		break;
	case 64*AMS_TCS3707_GAIN_SCALE:
		cfg1 = AGAIN_64X;
		break;
	case 128*AMS_TCS3707_GAIN_SCALE:
		cfg1 = AGAIN_128X;
		break;
	case 256*AMS_TCS3707_GAIN_SCALE:
		cfg1 = AGAIN_256X;
		break;
	case 512*AMS_TCS3707_GAIN_SCALE:
		cfg1 = AGAIN_512X;
		break;
	default:
		break;
	}	
		
	ret = ams_tcs3707_setField(ctx->portHndl, TCS3707_CFG1_REG, cfg1 , AGAIN_MASK);	 
	ctx->algCtx.als_data.gain =  gain;

	hwlog_warn("ams_tcs3707_setGain: gain = %d, cfg1 = %d\n", gain,cfg1); 

	return (ret);
}
#define MS_2_US 1000
static INT32 ams_tcs3707_setItime(ams_tcs3707_deviceCtx_t * ctx, int itime_ms)
{
	INT32 ret = 0;	
	int itime_us = 0;
	int atime = 0;

	if (ctx == NULL){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return true;
	}

	itime_us = itime_ms *MS_2_US;
	//defaut integration step is default_astep_us as set in the ams_tcs3707_resetAllRegisters() function
	atime = ((itime_us + default_astep_us/2)/default_astep_us) - 1;	//set atime algo
	if(atime < 0){
		atime = 0;
	}
	ret = ams_tcs3707_setByte(ctx->portHndl, TCS3707_ATIME_REG, atime);

	return ret;
}

static bool ams_tcs3707_alsRegUpdate(ams_tcs3707_deviceCtx_t * ctx, bool inCalMode)
{

	if (ctx == NULL){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return true;
	}

	/* For first integration after AEN */
	ctx->first_inte = false;
	if (!(ctx->mode & AMS_TCS3707_MODE_ALS) && !inCalMode)
	{
		ctx->algCtx.als_data.gain = AMS_TCS3707_AGAIN_FOR_FIRST_DATA;
		ctx->algCtx.als_data.itime_ms = AMS_TCS3707_ITIME_FOR_FIRST_DATA;
		ctx->first_inte = true;
	}

	ams_tcs3707_setItime(ctx, ctx->algCtx.als_data.itime_ms);
	ams_tcs3707_setGain(ctx, ctx->algCtx.als_data.gain);
	ams_tcs3707_setField(ctx->portHndl, TCS3707_PERS_REG, 0x00, APERS_MASK);

    return false;
}

/* --------------------------------------------------------------------*/
/* Set DCB config parameters                                           */
/* --------------------------------------------------------------------*/
static bool ams_tcs3707_deviceSetConfig(ams_tcs3707_deviceCtx_t * ctx, ams_tcs3707_configureFeature_t feature,
										ams_tcs3707_deviceConfigOptions_t option, UINT32 data, bool inCalMode)
{	

	if (ctx == NULL){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return true;
	}

	if (feature == AMS_TCS3707_FEATURE_ALS){
	   	AMS_PORT_log_Msg(AMS_DEBUG, "ams_configureFeature_t  AMS_CONFIG_ALS_LUX\n");
	    	switch (option)
	    	{
	        	case AMS_TCS3707_CONFIG_ENABLE: /* ON / OFF */
	            	AMS_PORT_log_Msg_1(AMS_DEBUG,"deviceConfigOptions_t   AMS_CONFIG_ENABLE(%u)\n", data);
	            	AMS_PORT_log_Msg_1(AMS_DEBUG,"current mode            %d\n", ctx->mode);
	            	if (data == 0) {						
						ams_tcs3707_setField(ctx->portHndl, TCS3707_ENABLE_REG, 0, (AEN|PON));
						ams_tcs3707_setByte(ctx->portHndl, TCS3707_INTENAB_REG, 0x00);
						ams_tcs3707_setByte(ctx->portHndl, TCS3707_STATUS_REG, (ASAT_MASK|AINT_MASK));
						ctx->mode = AMS_TCS3707_MODE_OFF;
	            	} else {						
						ams_tcs3707_alsRegUpdate(ctx, inCalMode);
						ams_tcs3707_setByte(ctx->portHndl, TCS3707_INTENAB_REG, AIEN);
						ams_tcs3707_setByte(ctx->portHndl, TCS3707_STATUS_REG, (ASAT_MASK|AINT_MASK));
						ams_tcs3707_setByte(ctx->portHndl, TCS3707_ENABLE_REG, AEN | PON);
						msleep(3);//delay for mode set ready
						ctx->mode |= AMS_TCS3707_MODE_ALS;
	            	}
	            	break;
	        default:
	            	break;
	    	}
	}
	return 0;
}

#define DOUBLE_BYTE 2
#define MAX_SATURATION 65535
static bool ams_tcs3707_saturation_check(ams_tcs3707_deviceCtx_t * ctx, ams_tcs3707_adc_data_t *current_raw)
{
	UINT32 saturation = 0;
	UINT8 atime = 0;
	UINT8 astep[DOUBLE_BYTE] = {0};
	UINT16 astep_inte = 0;

	ams_tcs3707_getByte(ctx->portHndl, TCS3707_ATIME_REG, &atime);
	ams_tcs3707_getBuf(ctx->portHndl, TCS3707_ASTEPL_REG, astep, DOUBLE_BYTE);
	astep_inte = (astep[1] << 8) | astep[0];	

	saturation = (atime +1)*(astep_inte +1) ;//calculate saturation value
	if(saturation > MAX_SATURATION){
		saturation = MAX_SATURATION;
	}
	saturation = (saturation*9)/10;//get saturation x 90%

	//hwlog_info("atime = %d, astep_inte = %d, saturation = %d\n",atime,astep_inte,saturation);

	if ((current_raw->c > saturation) || (current_raw->w > saturation)){
		return true;
	}
	return false;
}

static bool ams_tcs3707_insufficience_check(ams_tcs3707_deviceCtx_t * ctx, ams_tcs3707_adc_data_t *current_raw)
{
	if(current_raw == NULL){
		hwlog_err("%s NULL poniter\n", __func__);
		return false;
	}
	if (current_raw->c < AMS_TCS3707_LOW_LEVEL){
		return true;
	}
	return false;
}

static bool ams_tcs3707_handleALS(ams_tcs3707_deviceCtx_t * ctx, bool inCalMode)
{

	UINT8 adc_data[AMS_TCS3707_ADC_BYTES] = {0};
	UINT32 max_raw = 0, min_raw = 0;
	bool saturation_check = false;
	bool insufficience_check = false;
	bool re_enable = false;

	ams_tcs3707_adc_data_t current_raw = {
		.c   = 0,
		.r   = 0,
		.g   = 0,
		.b   = 0,
		.w   = 0
	};

	if (ctx == NULL){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return true;
	}

	/* read raw ADC data */
	ams_tcs3707_getBuf(ctx->portHndl, TCS3707_ADATA0L_REG, adc_data, AMS_TCS3707_ADC_BYTES);	
		
	current_raw.c   = (adc_data[0] << 0) | (adc_data[1] << ONE_BYTE_LENGTH_8_BITS);
	current_raw.r   = (adc_data[2] << 0) | (adc_data[3] << ONE_BYTE_LENGTH_8_BITS);
	current_raw.g = (adc_data[4] << 0) | (adc_data[5] << ONE_BYTE_LENGTH_8_BITS);			
	current_raw.b = (adc_data[6] << 0) | (adc_data[7] << ONE_BYTE_LENGTH_8_BITS);
	current_raw.w = (adc_data[8] << 0) | (adc_data[9] << ONE_BYTE_LENGTH_8_BITS);	

	if (inCalMode)
	{
		ctx->updateAvailable |= AMS_TCS3707_FEATURE_ALS;
	
		goto handleALS_exit;
	}

/*	hwlog_info("before enter gain adjust, ams_tcs3707_handleALS: ITIME = %d, AGAIN = %d\n",
		(UINT16)ctx->algCtx.als_data.itime_ms,(UINT32)ctx->algCtx.als_data.gain);
*/
	/* use a short timer to quickly fix a proper gain for first data */
	if (ctx->first_inte)
	{	
		//hwlog_info("ams_tcs3707_handleALS comes into the ctx->first_inte\n");
		hwlog_info("before enter gain adjust, ams_tcs3707_handleALS: ITIME = %d, AGAIN = %d\n",
			(UINT16)ctx->algCtx.als_data.itime_ms,(UINT32)ctx->algCtx.als_data.gain);
		//first integration time is 10ms, so the adc saturation value is (ATIME +1)*(ASTEP +1) = 6 * 600 = 3600
		//saturation_new = 3600*0.8 = 2880,the init gain value is 128,
		//so if gain change to 256, also not saturated, so the threshold is 2880/2 = 1440;
		//so if gain change to 512, also not saturated, so the threshold is 2880/4 = 720;
		if(current_raw.c <= GAIN_QUICKLY_FIX_LEVEL_1){
			ams_tcs3707_setGain(ctx, (CHOOSE_GAIN_512 * AMS_TCS3707_GAIN_SCALE));//use 512 gain
		}else if(current_raw.c <= GAIN_QUICKLY_FIX_LEVEL_2){
			ams_tcs3707_setGain(ctx, (CHOOSE_GAIN_256 * AMS_TCS3707_GAIN_SCALE));//use 256 gain
		}else if(current_raw.c <= GAIN_QUICKLY_FIX_LEVEL_3){
			ams_tcs3707_setGain(ctx, (CHOOSE_GAIN_128 * AMS_TCS3707_GAIN_SCALE));//use 128 gain
		}else{
			ams_tcs3707_setGain(ctx, (CHOOSE_GAIN_32 * AMS_TCS3707_GAIN_SCALE));//use 32 gain
		}
		//note: if the init gain changes or the sensor can recieve much light, above threshold values can be changed.	

		//change ITIME back to AMS_TCS3707_ITIME_DEFAUL again after changing the gain
		ctx->algCtx.als_data.itime_ms = AMS_TCS3707_ITIME_DEFAULT;		
		ams_tcs3707_setItime(ctx, ctx->algCtx.als_data.itime_ms);
		
		re_enable = true;
		ctx->first_inte = false;
		goto handleALS_exit;
	}

	saturation_check = ams_tcs3707_saturation_check(ctx, &current_raw);
	insufficience_check = ams_tcs3707_insufficience_check(ctx, &current_raw);

	/* Adjust gain setting */
	if (saturation_check &&	ctx->algCtx.als_data.gain == (CHOOSE_GAIN_32* AMS_TCS3707_GAIN_SCALE))
	{
		ams_tcs3707_setGain(ctx, (CHOOSE_GAIN_8 * AMS_TCS3707_GAIN_SCALE));
		re_enable = true;
	}
	else if ((saturation_check && ctx->algCtx.als_data.gain == (CHOOSE_GAIN_128 * AMS_TCS3707_GAIN_SCALE)) ||
			(insufficience_check &&	ctx->algCtx.als_data.gain == (CHOOSE_GAIN_8 * AMS_TCS3707_GAIN_SCALE)))
	{
		ams_tcs3707_setGain(ctx, (CHOOSE_GAIN_32 * AMS_TCS3707_GAIN_SCALE));
		re_enable = true;
	}
	else if ((saturation_check && ctx->algCtx.als_data.gain == (CHOOSE_GAIN_256 * AMS_TCS3707_GAIN_SCALE)) ||
			(insufficience_check &&	ctx->algCtx.als_data.gain == (CHOOSE_GAIN_32 * AMS_TCS3707_GAIN_SCALE)))
	{
		ams_tcs3707_setGain(ctx, (CHOOSE_GAIN_128 * AMS_TCS3707_GAIN_SCALE));
		re_enable = true;
	}
	else if ((saturation_check && ctx->algCtx.als_data.gain == (CHOOSE_GAIN_512 * AMS_TCS3707_GAIN_SCALE)) ||
			(insufficience_check && ctx->algCtx.als_data.gain == (CHOOSE_GAIN_128 * AMS_TCS3707_GAIN_SCALE)))
	{
		ams_tcs3707_setGain(ctx, (CHOOSE_GAIN_256 * AMS_TCS3707_GAIN_SCALE));
		re_enable = true;
	}
	else if (insufficience_check &&	ctx->algCtx.als_data.gain == (CHOOSE_GAIN_256 * AMS_TCS3707_GAIN_SCALE))
	{
		ams_tcs3707_setGain(ctx, (CHOOSE_GAIN_512 * AMS_TCS3707_GAIN_SCALE));
		re_enable = true;
	}
	else 
	{
		ctx->updateAvailable |= AMS_TCS3707_FEATURE_ALS;
	}

handleALS_exit:
	if (!re_enable)
	{
		ctx->algCtx.als_data.datasetArray.c   = current_raw.c;
		ctx->algCtx.als_data.datasetArray.r   = current_raw.r;
		ctx->algCtx.als_data.datasetArray.g   = current_raw.g;
		ctx->algCtx.als_data.datasetArray.b   = current_raw.b;
		ctx->algCtx.als_data.datasetArray.w   = current_raw.w;
	}

	/*hwlog_info("after enter gain adjust, ams_tcs3707_handleALS: ITIME = %d, AGAIN = %d\n",
		(UINT16)ctx->algCtx.als_data.itime_ms,(UINT32)ctx->algCtx.als_data.gain);
*/
	return re_enable;
}

/* --------------------------------------------------------------------*/
/* Called by the OSAL interrupt service routine                        */
/* --------------------------------------------------------------------*/
static bool ams_tcs3707_deviceEventHandler(ams_tcs3707_deviceCtx_t * ctx, bool inCalMode)
{
    bool ret = false;

	if (ctx == NULL){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return false;
	}

	ams_tcs3707_getByte(ctx->portHndl, TCS3707_STATUS_REG, &ctx->algCtx.als_data.status);
	
    if (ctx->algCtx.als_data.status & (AINT_MASK)){
	    if (ctx->mode & AMS_TCS3707_MODE_ALS){
			ret = ams_tcs3707_handleALS(ctx, inCalMode);	     
			/* Clear the interrupt */			
			ams_tcs3707_setByte(ctx->portHndl, TCS3707_STATUS_REG, (ASAT_MASK | AINT_MASK));
		}
    }
    return ret;
}

static bool ams_tcs3707_deviceGetMode(ams_tcs3707_deviceCtx_t * ctx, ams_tcs3707_ams_mode_t *mode)
{
	if ((ctx == NULL) || (mode == NULL)){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return true;
	}
    *mode = ctx->mode;
    return false;
}

static UINT32 ams_tcs3707_deviceGetResult(ams_tcs3707_deviceCtx_t * ctx)
{
	if (ctx == NULL){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return true;
	}
	return ctx->updateAvailable;
}

static bool ams_tcs3707_deviceGetAls(ams_tcs3707_deviceCtx_t * ctx, export_alsData_t * exportData)
{
	if ((ctx == NULL) || (exportData == NULL)){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return true;
	}
	ctx->updateAvailable &= ~(AMS_TCS3707_FEATURE_ALS);

	exportData->rawC = ctx->algCtx.als_data.datasetArray.c;
	exportData->rawR = ctx->algCtx.als_data.datasetArray.r;
	exportData->rawG = ctx->algCtx.als_data.datasetArray.g;
	exportData->rawB = ctx->algCtx.als_data.datasetArray.b;
	exportData->rawW = ctx->algCtx.als_data.datasetArray.w;

	hwlog_debug("ams_tcs3707_deviceGetAls rawC = %d,rawR = %d, rawG = %d, rawB = %d,, rawW = %d\n",
					exportData->rawC,exportData->rawR,exportData->rawG,exportData->rawB,exportData->rawW);
    return false;
}



/* --------------------------------------------------------------------
 * Return default calibration data
 * --------------------------------------------------------------------*/
static void ams_tcs3707_getDefaultCalData(ams_tcs3707_calibrationData_t *cal_data)
{
	if (cal_data == NULL){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return true;
	}

    cal_data->timeBase_us = AMS_TCS3707_USEC_PER_TICK;
	return;
}

static bool ams_tcs3707_deviceInit(ams_tcs3707_deviceCtx_t * ctx, AMS_PORT_portHndl * portHndl)
{
    int ret = 0;

	if ((ctx == NULL) || (portHndl == NULL)){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return true;
	}

	hwlog_info("enter [%s] \n", __func__);

    memset(ctx, 0, sizeof(ams_tcs3707_deviceCtx_t));
    ctx->portHndl = portHndl;
    ctx->mode = AMS_TCS3707_MODE_OFF;
    ctx->deviceId = ams_tcs3707_testForDevice(ctx->portHndl);

    ams_tcs3707_resetAllRegisters(ctx->portHndl);

    ctx->algCtx.als_data.itime_ms = AMS_TCS3707_ITIME_DEFAULT;
    ctx->algCtx.als_data.gain = AMS_TCS3707_AGAIN_DEFAULT;	
	
	ams_tcs3707_setByte(ctx->portHndl, TCS3707_ENABLE_REG, (AEN|PON));
	msleep(3);//need dealy for ready
	
	ams_tcs3707_setField(ctx->portHndl, TCS3707_ENABLE_REG, 0, AEN);
	ams_tcs3707_setByte(ctx->portHndl, TCS3707_INTENAB_REG, 0x00);
	ams_tcs3707_setByte(ctx->portHndl, TCS3707_STATUS_REG, (ASAT_MASK|AINT_MASK));

	hwlog_info("ams_deviceInit end\n");
    return ret;
}

static bool ams_tcs3707_getDeviceInfo(ams_tcs3707_deviceInfo_t * info)
{
	if (info == NULL){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return true;
	}
	memset(info, 0, sizeof(ams_tcs3707_deviceInfo_t));
	info->memorySize =  sizeof(ams_tcs3707_deviceCtx_t);
	ams_tcs3707_getDefaultCalData(&info->defaultCalibrationData);
	return false;
}

void osal_tcs3707_als_timerHndl(unsigned long data)
{
	struct colorDriver_chip *chip = (struct colorDriver_chip*) data;

	if (chip == NULL){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return;
	}
	schedule_work(&chip->als_work);
}

static ssize_t osal_als_enable_set(struct colorDriver_chip *chip, uint8_t valueToSet)
{
	ssize_t rc = 0;
	ams_tcs3707_ams_mode_t mode = 0;

	if (chip == NULL){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return true;
	}

	ams_tcs3707_deviceGetMode(chip->deviceCtx, &mode);

#ifdef CONFIG_AMS_OPTICAL_SENSOR_ALS
	rc |= ams_tcs3707_deviceSetConfig(chip->deviceCtx, AMS_TCS3707_FEATURE_ALS, AMS_TCS3707_CONFIG_ENABLE, valueToSet, chip->inCalMode);
#endif
    hwlog_info("osal_als_enable_set = %d\n",valueToSet);

#ifndef CONFIG_AMS_OPTICAL_SENSOR_IRQ
    if (valueToSet){
		if ((mode & AMS_TCS3707_MODE_ALS) != AMS_TCS3707_MODE_ALS){
			if(chip->inCalMode == false){
				mod_timer(&chip->work_timer, jiffies + msecs_to_jiffies(12));//first enable timer
				hwlog_info("osal_als_enable_set 12ms for finding a proper gain quickly \n");
				report_logcount = AMS_REPORT_LOG_COUNT_NUM;
			}else{
				mod_timer(&chip->work_timer, jiffies + msecs_to_jiffies(120));//first enable set the timer as 120ms
				hwlog_info("in calibrate mode timer set as 120ms \n");
			}			
		} else {
			hwlog_info( "osal_als_enable_set: timer already running\n");
		}
    } else {
		if ((mode & AMS_TCS3707_MODE_ALS) == AMS_TCS3707_MODE_ALS){
			hwlog_info( "osal_als_enable_set: del_timer\n");
		}
    }
#endif

	return 0;
}
static int get_cal_para_from_nv(void)
{
	int i = 0, ret = 0;

	ret = read_color_calibrate_data_from_nv(RGBAP_CALI_DATA_NV_NUM, RGBAP_CALI_DATA_SIZE, "RGBAP", &color_nv_para);
	if(ret < 0){
		hwlog_err("AMS_Driver: %s: fail,use default para!!\n", __func__);
		for (i = 0; i < CAL_STATE_GAIN_LAST; i++){
			hwlog_err("AMS_Driver: get_cal_para_from_nv: gain[%d]: [%d, %d, %d, %d,%d]\n", i,
			color_nv_para.calCratio[i], color_nv_para.calRratio[i], color_nv_para.calGratio[i], 
			color_nv_para.calBratio[i], color_nv_para.calWratio[i]);
		}
		return 0;
	}

	for (i = 0; i < CAL_STATE_GAIN_LAST; i++){
		hwlog_warn("AMS_Driver: get_cal_para_from_nv: gain[%d]: [%d, %d, %d, %d, %d]\n", i,
		color_nv_para.calCratio[i], color_nv_para.calRratio[i], color_nv_para.calGratio[i], 
		color_nv_para.calBratio[i],color_nv_para.calWratio[i]);
		if(!color_nv_para.calCratio[i]||!color_nv_para.calRratio[i]
			||!color_nv_para.calGratio[i]||!color_nv_para.calBratio[i]||!color_nv_para.calWratio[i]){
			color_nv_para.calCratio[i] = AMS_TCS3707_FLOAT_TO_FIX ;
			color_nv_para.calRratio[i]  = AMS_TCS3707_FLOAT_TO_FIX ;
			color_nv_para.calGratio[i]  = AMS_TCS3707_FLOAT_TO_FIX ;
			color_nv_para.calBratio[i] = AMS_TCS3707_FLOAT_TO_FIX ;
			color_nv_para.calWratio[i] = AMS_TCS3707_FLOAT_TO_FIX ;
		}
/*		else if (color_nv_para.calCratio[i]>=FLOAT_TO_FIX_LOW/AMS_TCS3707_CAL_THR
				&&color_nv_para.calCratio[i]<=FLOAT_TO_FIX_LOW*AMS_TCS3707_CAL_THR
				&&color_nv_para.calRratio[i]>=FLOAT_TO_FIX_LOW/AMS_TCS3707_CAL_THR
				&&color_nv_para.calRratio[i]<=FLOAT_TO_FIX_LOW*AMS_TCS3707_CAL_THR
				&&color_nv_para.calGratio[i]>=FLOAT_TO_FIX_LOW/AMS_TCS3707_CAL_THR
				&&color_nv_para.calGratio[i]<=FLOAT_TO_FIX_LOW*AMS_TCS3707_CAL_THR
				&&color_nv_para.calBratio[i]>=FLOAT_TO_FIX_LOW/AMS_TCS3707_CAL_THR
				&&color_nv_para.calBratio[i]<=FLOAT_TO_FIX_LOW*AMS_TCS3707_CAL_THR
				&&color_nv_para.calWratio[i]>=FLOAT_TO_FIX_LOW/AMS_TCS3707_CAL_THR
				&&color_nv_para.calWratio[i]<=FLOAT_TO_FIX_LOW*AMS_TCS3707_CAL_THR){
			color_nv_para.calCratio[i] *= (AMS_TCS3707_FLOAT_TO_FIX/FLOAT_TO_FIX_LOW);
			color_nv_para.calRratio[i] *= (AMS_TCS3707_FLOAT_TO_FIX/FLOAT_TO_FIX_LOW);
			color_nv_para.calGratio[i] *= (AMS_TCS3707_FLOAT_TO_FIX/FLOAT_TO_FIX_LOW);
			color_nv_para.calBratio[i] *= (AMS_TCS3707_FLOAT_TO_FIX/FLOAT_TO_FIX_LOW);
			color_nv_para.calWratio[i] *= (AMS_TCS3707_FLOAT_TO_FIX/FLOAT_TO_FIX_LOW);
			hwlog_info("AMS_Driver: low_level nv calibrate data\n");
		}*/
	}
#if 0
	for (i = 0; i <= CAL_STATE_GAIN_2; i++){
		color_nv_para.calCratio[i] = color_nv_para.calCratio[CAL_STATE_GAIN_3] ;
		color_nv_para.calRratio[i] = color_nv_para.calRratio[CAL_STATE_GAIN_3] ;
		color_nv_para.calGratio[i] = color_nv_para.calGratio[CAL_STATE_GAIN_3] ;
		color_nv_para.calBratio[i] = color_nv_para.calBratio[CAL_STATE_GAIN_3] ;
		color_nv_para.calWratio[i] = color_nv_para.calWratio[CAL_STATE_GAIN_3] ;
		if(i == CAL_STATE_GAIN_1){
			hwlog_info("AMS_Driver: 1xGain equal to 16xGain for nv calibrate data\n");
		}else if(i == CAL_STATE_GAIN_2){
			hwlog_info("AMS_Driver: 4xGain equal to 16xGain for nv calibrate data\n");
		}
	}
#endif
	return 1;
}
static int save_cal_para_to_nv(struct colorDriver_chip *chip)
{
	int i = 0, ret;
	if (chip == NULL){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return 0;
	}

	color_nv_para.nv_Ctarget = chip->calibrationCtx.calCtarget;
	color_nv_para.nv_Rtarget = chip->calibrationCtx.calRtarget;
	color_nv_para.nv_Gtarget = chip->calibrationCtx.calGtarget;
	color_nv_para.nv_Btarget = chip->calibrationCtx.calBtarget;
	color_nv_para.nv_Wtarget = chip->calibrationCtx.calWtarget;

	for(i = 0; i < CAL_STATE_GAIN_LAST; i++){
		color_nv_para.calCratio[i] = chip->calibrationCtx.calCresult[i];
		color_nv_para.calRratio[i]  = chip->calibrationCtx.calRresult[i];
		color_nv_para.calGratio[i]  = chip->calibrationCtx.calGresult[i];
		color_nv_para.calBratio[i] = chip->calibrationCtx.calBresult[i];
		color_nv_para.calWratio[i] = chip->calibrationCtx.calWresult[i];
		hwlog_info("AMS_Driver: save_cal_para_to_nv: gain[%d]: [%d, %d, %d, %d,%d]\n", i,
		color_nv_para.calCratio[i], color_nv_para.calRratio[i], color_nv_para.calGratio[i], 
		color_nv_para.calBratio[i],color_nv_para.calWratio[i]);

	}

	ret = write_color_calibrate_data_to_nv(RGBAP_CALI_DATA_NV_NUM, RGBAP_CALI_DATA_SIZE, "RGBAP", &color_nv_para);
	if(ret < 0){
		hwlog_err("AMS_Driver: %s: fail\n", __func__);
	}
	return 1;
}

static void osal_calHandl_als(struct colorDriver_chip *chip)
{
	export_alsData_t outData;
	ams_tcs3707_deviceCtx_t * ctx = NULL;
	UINT32 currentGain = 0;
	UINT32 result = 0; /* remember, this is for fixed point and can cause lower performance */

	hwlog_info( "comes into osal_calHandl_als \n");

	if (chip == NULL){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return;
	}
	if (chip->deviceCtx == NULL){
		hwlog_err("AMS_Driver: %s: deviceCtx is NULL\n", __func__);
		return;
	}

	ctx = chip->deviceCtx;
	currentGain = (ctx->algCtx.als_data.gain / AMS_TCS3707_GAIN_SCALE);

	ams_tcs3707_deviceGetAls(chip->deviceCtx, &outData);

	hwlog_info( "osal_calHandl_als: state %d\n", chip->calibrationCtx.calState);
	hwlog_info( "osal_calHandl_als: count %d\n", chip->calibrationCtx.calSampleCounter);

	if (chip->calibrationCtx.calState < CAL_STATE_GAIN_LAST && chip->calibrationCtx.calState >= 0){
		chip->calibrationCtx.calSampleCounter++;
		chip->calibrationCtx.calCsample += outData.rawC;
		chip->calibrationCtx.calRsample += outData.rawR;
		chip->calibrationCtx.calGsample += outData.rawG;
		chip->calibrationCtx.calBsample += outData.rawB;
		chip->calibrationCtx.calWsample += outData.rawW;
		
		if((chip->calibrationCtx.calState < CAL_STATE_GAIN_LAST) && (chip->calibrationCtx.calState >= 0)){
			//judge C channel
			result = (chip->calibrationCtx.calCsample / AMS_TCS3707_CAL_AVERAGE);
			if (result){
				result = (chip->calibrationCtx.calCtarget * (currentGain *
					AMS_TCS3707_FLOAT_TO_FIX / AMS_TCS3707_GAIN_OF_GOLDEN)) / result;
				if(result > ams_tcs3707_setGainThreshold[chip->calibrationCtx.calState].high_thr||
					result < ams_tcs3707_setGainThreshold[chip->calibrationCtx.calState].low_thr){
					hwlog_err("%s: ratio is out bound[%d, %d]! calCresult[%d] = %d\n" , __func__,
						ams_tcs3707_setGainThreshold[chip->calibrationCtx.calState].high_thr,
						ams_tcs3707_setGainThreshold[chip->calibrationCtx.calState].low_thr, chip->calibrationCtx.calState, result);
					color_calibrate_result = false;
				}
			} else {
				/* cant devide by zero, maintain the default calibration scaling factor */
				result = AMS_TCS3707_FLOAT_TO_FIX;
			}
			chip->calibrationCtx.calCresult[chip->calibrationCtx.calState] = result;

			//judge R channel
			result = (chip->calibrationCtx.calRsample / AMS_TCS3707_CAL_AVERAGE);
			if (result){
				result = (chip->calibrationCtx.calRtarget * (currentGain *
					AMS_TCS3707_FLOAT_TO_FIX / AMS_TCS3707_GAIN_OF_GOLDEN)) / result;
				if(result > ams_tcs3707_setGainThreshold[chip->calibrationCtx.calState].high_thr
					|| result < ams_tcs3707_setGainThreshold[chip->calibrationCtx.calState].low_thr ){
					hwlog_err("%s: ratio is out bound[%d, %d]! calRresult[%d] = %d\n" , __func__,
						ams_tcs3707_setGainThreshold[chip->calibrationCtx.calState].high_thr,
						ams_tcs3707_setGainThreshold[chip->calibrationCtx.calState].low_thr, chip->calibrationCtx.calState, result);
					color_calibrate_result = false;
				}
			} else {
				/* cant devide by zero, maintain the default calibration scaling factor */
				result = AMS_TCS3707_FLOAT_TO_FIX;
			}
			chip->calibrationCtx.calRresult[chip->calibrationCtx.calState] = result;

			//judge G channel
			result = (chip->calibrationCtx.calGsample / AMS_TCS3707_CAL_AVERAGE);
			if (result){
				result = (chip->calibrationCtx.calGtarget * (currentGain *
					AMS_TCS3707_FLOAT_TO_FIX / AMS_TCS3707_GAIN_OF_GOLDEN)) / result;
				if(result > ams_tcs3707_setGainThreshold[chip->calibrationCtx.calState].high_thr
					|| result < ams_tcs3707_setGainThreshold[chip->calibrationCtx.calState].low_thr ){

					hwlog_err("%s: ratio is out bound[%d, %d]! calGresult[%d] = %d\n" , __func__,
						ams_tcs3707_setGainThreshold[chip->calibrationCtx.calState].high_thr,
						ams_tcs3707_setGainThreshold[chip->calibrationCtx.calState].low_thr, chip->calibrationCtx.calState, result);
					color_calibrate_result = false;
				}
			} else {
				/* cant devide by zero, maintain the default calibration scaling factor */
				result = AMS_TCS3707_FLOAT_TO_FIX;
			}
			chip->calibrationCtx.calGresult[chip->calibrationCtx.calState] = result;

			//judge B channel
			result = (chip->calibrationCtx.calBsample / AMS_TCS3707_CAL_AVERAGE);
			if (result){
				result = (chip->calibrationCtx.calBtarget * (currentGain *
					AMS_TCS3707_FLOAT_TO_FIX / AMS_TCS3707_GAIN_OF_GOLDEN)) / result;
				if(result > ams_tcs3707_setGainThreshold[chip->calibrationCtx.calState].high_thr||
					result < ams_tcs3707_setGainThreshold[chip->calibrationCtx.calState].low_thr ){

					hwlog_err("%s: ratio is out bound[%d, %d]! calBresult[%d] = %d\n" , __func__,	
						ams_tcs3707_setGainThreshold[chip->calibrationCtx.calState].high_thr,
						ams_tcs3707_setGainThreshold[chip->calibrationCtx.calState].low_thr, chip->calibrationCtx.calState, result);
					color_calibrate_result = false;
				}
			} else {
				/* cant devide by zero, maintain the default calibration scaling factor */
				hwlog_err("%s: calBresult[%d] = 0!!\n" , __func__, chip->calibrationCtx.calState);
				result = AMS_TCS3707_FLOAT_TO_FIX;
			}
			chip->calibrationCtx.calBresult[chip->calibrationCtx.calState] = result;

			//judge W channel
			result = (chip->calibrationCtx.calWsample / AMS_TCS3707_CAL_AVERAGE);
			if (result){
				result = (chip->calibrationCtx.calWtarget * (currentGain *
					AMS_TCS3707_FLOAT_TO_FIX / AMS_TCS3707_GAIN_OF_GOLDEN)) / result;
				if(result > ams_tcs3707_setGainThreshold[chip->calibrationCtx.calState].high_thr||
					result < ams_tcs3707_setGainThreshold[chip->calibrationCtx.calState].low_thr ){

					hwlog_err("%s: ratio is out bound[%d, %d]! calWresult[%d] = %d\n" , __func__,	
						ams_tcs3707_setGainThreshold[chip->calibrationCtx.calState].high_thr,
						ams_tcs3707_setGainThreshold[chip->calibrationCtx.calState].low_thr, chip->calibrationCtx.calState, result);
					color_calibrate_result = false;
				}
			} else {
				/* cant devide by zero, maintain the default calibration scaling factor */
				hwlog_err("%s: calWresult[%d] = 0!!\n" , __func__, chip->calibrationCtx.calState);
				result = AMS_TCS3707_FLOAT_TO_FIX;
			}
			chip->calibrationCtx.calWresult[chip->calibrationCtx.calState] = result;

			hwlog_info( "osal_calHandl_als: calCresult  %d\n", chip->calibrationCtx.calCresult[chip->calibrationCtx.calState]);
			hwlog_info( "osal_calHandl_als: calRresult  %d\n", chip->calibrationCtx.calRresult[chip->calibrationCtx.calState]);
			hwlog_info( "osal_calHandl_als: calGresult  %d\n", chip->calibrationCtx.calGresult[chip->calibrationCtx.calState]);
			hwlog_info( "osal_calHandl_als: calBresult %d\n", chip->calibrationCtx.calBresult[chip->calibrationCtx.calState]);
			hwlog_info( "osal_calHandl_als: calWresult %d\n", chip->calibrationCtx.calWresult[chip->calibrationCtx.calState]);

			chip->calibrationCtx.calState++;
			chip->calibrationCtx.calSampleCounter = 0;
			chip->calibrationCtx.calCsample = 0;
			chip->calibrationCtx.calRsample = 0;
			chip->calibrationCtx.calGsample = 0;
			chip->calibrationCtx.calBsample = 0;
			chip->calibrationCtx.calWsample = 0;
			if(chip->calibrationCtx.calState < CAL_STATE_GAIN_LAST){
				osal_als_enable_set(chip, AMSDRIVER_ALS_DISABLE);
				ctx->algCtx.als_data.gain = ams_tcs3707_alsGain_conversion[chip->calibrationCtx.calState];
				osal_als_enable_set(chip, AMSDRIVER_ALS_ENABLE);
			}
		}
	}
	else {
		if(true == color_calibrate_result){
			save_cal_para_to_nv(chip);
			report_calibrate_result = true;
		}else {
			dev_err(&chip->client->dev, "color_calibrate_result fail\n");
			report_calibrate_result = false;
		}
		chip->inCalMode = false;
		osal_als_enable_set(chip, AMSDRIVER_ALS_DISABLE);
		ams_tcs3707_setGain(chip->deviceCtx, AMS_TCS3707_AGAIN_DEFAULT);
		if(1 == enable_status_before_calibrate){
			osal_als_enable_set(chip, AMSDRIVER_ALS_ENABLE);
		}else{
			hwlog_info( "color sensor disabled before calibrate\n");
		}
		hwlog_info( "osal_calHandl_als: done\n");
	}
	return;
}

static void  osal_report_als(struct colorDriver_chip *chip)
{
	export_alsData_t outData = {0};
	UINT8 currentGainIndex = 0;
	UINT32 currentGain = 0;
	UINT8 i = 0,j=0;
	ams_tcs3707_deviceCtx_t * ctx = NULL;

	if (chip == NULL){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return;
	}
	ctx = chip->deviceCtx;

	ams_tcs3707_deviceGetAls(chip->deviceCtx, &outData);
	if(NULL != ctx){
		currentGain = ctx->algCtx.als_data.gain;
	}
	if (currentGain < ams_tcs3707_alsGain_conversion[CAL_STATE_GAIN_5]){
		currentGainIndex = ams_tcs3707_gainToReg(currentGain);		
	} else {
		currentGainIndex = CAL_STATE_GAIN_5;
	}

	/* adjust the report data when the calibrate ratio is acceptable */

	outData.rawC *= color_nv_para.calCratio[currentGainIndex];
	outData.rawC /= AMS_TCS3707_FLOAT_TO_FIX;
	
	outData.rawR *= color_nv_para.calRratio[currentGainIndex];
	outData.rawR /= AMS_TCS3707_FLOAT_TO_FIX;
	
	outData.rawG *= color_nv_para.calGratio[currentGainIndex];
	outData.rawG /= AMS_TCS3707_FLOAT_TO_FIX;
	
	outData.rawB *= color_nv_para.calBratio[currentGainIndex];
	outData.rawB /= AMS_TCS3707_FLOAT_TO_FIX;

	outData.rawW *= color_nv_para.calWratio[currentGainIndex];
	outData.rawW /= AMS_TCS3707_FLOAT_TO_FIX;	

	report_value[0] = (int)outData.rawC;
	report_value[1] = (int)outData.rawR;
	report_value[2] = (int)outData.rawG;
	report_value[3] = (int)outData.rawB;
	report_value[4] = (int)outData.rawW;
	color_report_val[0] = AMS_REPORT_DATA_LEN;

	for(j = 0; j < 5; j++){
		report_value[j] *= AMS_TCS3707_GAIN_OF_GOLDEN;
		report_value[j] /= ams_tcs3707_als_gains[currentGainIndex];
		color_report_val[j+1] = report_value[j];
	} 

	ams_tcs3707_report_data(report_value);
	
	report_logcount++;
	if(report_logcount >= AMS_REPORT_LOG_COUNT_NUM){
		hwlog_warn("COLOR SENSOR tcs3707 report data %d, %d, %d, %d, %d\n",
			report_value[0], report_value[1], report_value[2], report_value[3], report_value[4]);
/*		hwlog_info("AMS_Driver: currentGain[%d],calCratio[%d],calRratio[%d],calGratio[%d],calBratio[%d],calWratio[%d]\n",
			ams_tcs3707_als_gains[currentGainIndex],
			color_nv_para.calCratio[currentGainIndex],
			color_nv_para.calRratio[currentGainIndex],
			color_nv_para.calGratio[currentGainIndex],
			color_nv_para.calBratio[currentGainIndex],
			color_nv_para.calWratio[currentGainIndex]);*/
		report_logcount = 0;
	}
}

#if defined(CONFIG_AMS_OPTICAL_SENSOR_ALS_XYZ)
static ssize_t osal_als_C_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	export_alsData_t outData;
	if(NULL == dev || NULL == attr || NULL == buf){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return 0;
		}
	struct colorDriver_chip *chip = dev_get_drvdata(dev);
	ams_tcs3707_deviceGetAls(chip->deviceCtx, &outData);
	return snprintf(buf, PAGE_SIZE, "%d\n", outData.rawC);
}

static ssize_t osal_als_R_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	export_alsData_t outData;
	if(NULL == dev){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return 0;
		}
	struct colorDriver_chip *chip = dev_get_drvdata(dev);
	ams_tcs3707_deviceGetAls(chip->deviceCtx, &outData);
	return snprintf(buf, PAGE_SIZE, "%d\n", outData.rawR);
}
static ssize_t osal_als_G_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	export_alsData_t outData;
	if(NULL == dev){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return 0;
		}
	struct colorDriver_chip *chip = dev_get_drvdata(dev);
	ams_tcs3707_deviceGetAls(chip->deviceCtx, &outData);
	return snprintf(buf, PAGE_SIZE, "%d\n", outData.rawG);
}

static ssize_t osal_als_B_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	export_alsData_t outData;
	if(NULL == dev){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return 0;
		}
	struct colorDriver_chip *chip = dev_get_drvdata(dev);
	ams_tcs3707_deviceGetAls(chip->deviceCtx, &outData);
	return snprintf(buf, PAGE_SIZE, "%d\n", outData.rawB);
}

static ssize_t osal_als_W_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	export_alsData_t outData;
	if(NULL == dev){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return 0;
		}
	struct colorDriver_chip *chip = dev_get_drvdata(dev);
	ams_tcs3707_deviceGetAls(chip->deviceCtx, &outData);
	return snprintf(buf, PAGE_SIZE, "%d\n", outData.rawW);
}
#endif 

int ams_tcs3707_setenable(bool enable)
{
	struct colorDriver_chip *chip = p_chip;

	if (enable)
		osal_als_enable_set(chip, AMSDRIVER_ALS_ENABLE);
	else
		osal_als_enable_set(chip, AMSDRIVER_ALS_DISABLE);

	hwlog_info("ams_tcs3707_setenable success\n");
	return 1;
}
EXPORT_SYMBOL_GPL(ams_tcs3707_setenable);

static ssize_t osal_als_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	if(NULL == dev){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return 0;
		}
	struct colorDriver_chip *chip = dev_get_drvdata(dev);
	ams_tcs3707_ams_mode_t mode = AMS_TCS3707_MODE_OFF;

	ams_tcs3707_deviceGetMode(chip->deviceCtx, &mode);

	if (mode & AMS_TCS3707_MODE_ALS){
		return snprintf(buf, PAGE_SIZE, "%d\n", 1);
	} else {
		return snprintf(buf, PAGE_SIZE, "%d\n", 0);
	}
}

static ssize_t osal_als_enable_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	if(NULL == dev ||NULL==attr || NULL==buf)
	{
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return 0;
		}
	struct colorDriver_chip *chip = dev_get_drvdata(dev);
	bool value;

	if (strtobool(buf, &value))
		return -EINVAL;

	if (value)
		osal_als_enable_set(chip, AMSDRIVER_ALS_ENABLE);
	else
		osal_als_enable_set(chip, AMSDRIVER_ALS_DISABLE);

	return size;
}


void ams_tcs3707_show_calibrate(struct colorDriver_chip *chip, at_color_sensor_output_para * out_para)
{
	int i = 0;	

	if (NULL == out_para || NULL == chip){
		hwlog_err("ams_show_calibrate input para NULL \n");
		return;
	}

	if (chip->inCalMode == false){
		hwlog_err("ams_show_calibrate not in calibration mode \n");
	}
	
	out_para->result = (UINT32)report_calibrate_result;
	hwlog_info(" color_calibrate_show result = %d\n", out_para->result);
	out_para->gain_arr = CAL_STATE_GAIN_LAST;
	out_para->color_arr = AMS_REPORT_DATA_LEN;
	memcpy(out_para->report_gain, ams_tcs3707_als_gains, sizeof(out_para->report_gain));
	memcpy(out_para->report_raw[0], chip->calibrationCtx.calCresult, sizeof(out_para->report_raw[0]));
	memcpy(out_para->report_raw[1],  chip->calibrationCtx.calRresult,  sizeof(out_para->report_raw[1]));
	memcpy(out_para->report_raw[2],  chip->calibrationCtx.calGresult,  sizeof(out_para->report_raw[2]));
	memcpy(out_para->report_raw[3],  chip->calibrationCtx.calBresult,  sizeof(out_para->report_raw[3]));
	memcpy(out_para->report_raw[4],  chip->calibrationCtx.calWresult,  sizeof(out_para->report_raw[4]));
	
	for(i = 0;i<CAL_STATE_GAIN_LAST; i++)
	{
		hwlog_info(" color_calibrate_show i = %d: %d,%d,%d,%d,%d.\n", i,
		out_para->report_raw[0][i],out_para->report_raw[1][i],out_para->report_raw[2][i],out_para->report_raw[3][i],out_para->report_raw[4][i]);
	}
	return;
}
void ams_tcs3707_store_calibrate(struct colorDriver_chip *chip, at_color_sensor_input_para * in_para)
{
	ams_tcs3707_deviceCtx_t * ctx = NULL;
	ams_tcs3707_ams_mode_t mode = 0;
	color_sensor_input_para_tcs3707 input_para;
	if((NULL == chip) || (NULL == in_para)){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return;
	}

	input_para.enable = in_para->enable;
	input_para.tar_c = in_para->reserverd[0]; /* calculate targer for gain 1x (assuming its set at 64x) */
	input_para.tar_r = in_para->reserverd[1];
	input_para.tar_g = in_para->reserverd[2];
	input_para.tar_b = in_para->reserverd[3];	
	input_para.tar_w = in_para->reserverd[4];

	if (input_para.enable &&  chip->inCalMode){
		hwlog_err("ams_store_calibrate: Already in calibration mode.\n");
		return;
	}

	if (input_para.enable){
		ctx = chip->deviceCtx;
		hwlog_info( "ams_store_calibrate: starting calibration mode\n");
		chip->calibrationCtx.calSampleCounter = 0;
		chip->calibrationCtx.calCsample = 0;
		chip->calibrationCtx.calRsample = 0;
		chip->calibrationCtx.calGsample = 0;
		chip->calibrationCtx.calBsample = 0;
		chip->calibrationCtx.calWsample = 0;
		chip->calibrationCtx.calCtarget= input_para.tar_c; /* calculate targer for gain 1x (assuming its set at 64x) */
		chip->calibrationCtx.calRtarget= input_para.tar_r;
		chip->calibrationCtx.calGtarget= input_para.tar_g;
		chip->calibrationCtx.calBtarget= input_para.tar_b;
		chip->calibrationCtx.calWtarget= input_para.tar_w;
		chip->calibrationCtx.calState = CAL_STATE_GAIN_1;
		hwlog_info( "input_para->tar_c = %d,input_para->tar_r = %d,,input_para->tar_g = %d,input_para->tar_b = %d ,input_para->tar_w = %d\n",
		input_para.tar_c,input_para.tar_r,input_para.tar_g,input_para.tar_b,input_para.tar_w);
		
		if(NULL != ctx){
			ctx->algCtx.als_data.gain = ams_tcs3707_alsGain_conversion[CAL_STATE_GAIN_1];
		}
		ams_tcs3707_deviceGetMode(chip->deviceCtx, &mode);
		if ((mode & AMS_TCS3707_MODE_ALS) == AMS_TCS3707_MODE_ALS){
			enable_status_before_calibrate = 1;//enabled before calibrate
			hwlog_info("AMS_Driver: %s: enabled before calibrate\n", __func__);
			osal_als_enable_set(chip, AMSDRIVER_ALS_DISABLE);
			msleep(10);//sleep 10 ms to make sure disable timer
		}else{
			enable_status_before_calibrate = 0;//disabled before calibrate
			hwlog_info("AMS_Driver: %s: disabled before calibrate\n", __func__);
		}
		chip->inCalMode = true;
		color_calibrate_result = true;//make the calibrate_result true for calibrate again!!
		osal_als_enable_set(chip, AMSDRIVER_ALS_ENABLE);
	} else {
		hwlog_info( "ams_store_calibrate: stopping calibration mode\n");
		chip->inCalMode = false;
		}
	return;
}
void ams_tcs3707_show_enable(struct colorDriver_chip *chip, int *state)
{
	ams_tcs3707_ams_mode_t mode = 0;

	if((NULL == chip) || (NULL == state)){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return;
		}
	
	ams_tcs3707_deviceGetMode(chip->deviceCtx, &mode);
	if (mode & AMS_TCS3707_MODE_ALS){
		*state = 1;
	} else {
		*state = 0;
	}
	
}

void ams_tcs3707_store_enable(struct colorDriver_chip *chip, int state)
{
	ams_tcs3707_ams_mode_t mode = 0;
	hwlog_info("enter [%s] \n", __func__);

	if(NULL == chip){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return;
	}
	if (state)
		osal_als_enable_set(chip, AMSDRIVER_ALS_ENABLE);
	else
		osal_als_enable_set(chip, AMSDRIVER_ALS_DISABLE);

}

#endif


/****************************************************************************
 *                     OSAL Linux Input Driver
 ****************************************************************************/

static int amsdriver_pltf_power_on(struct colorDriver_chip *chip)
{
	int rc = 0;
	if(NULL == chip){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return rc;
	}
	return rc;
}

static int amsdriver_pltf_power_off(struct colorDriver_chip *chip)
{
	int rc = 0;
	if(NULL == chip){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return rc;
	}
	return rc;
}

static int amsdriver_power_on(struct colorDriver_chip *chip)
{
	int rc = 0;
	if(NULL == chip){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return rc;
	}
	rc = amsdriver_pltf_power_on(chip);
	if (rc){
		return rc;
	}
	hwlog_info( "%s: chip was off, restoring regs\n",
			__func__);
	return ams_tcs3707_deviceInit((ams_tcs3707_deviceCtx_t*)chip->deviceCtx, chip->client);
}

#ifdef CONFIG_AMS_OPTICAL_SENSOR_ALS
static int osal_als_idev_open(struct input_dev *idev)
{
	if(NULL == idev){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return 0;
	}
	struct colorDriver_chip *chip = dev_get_drvdata(&idev->dev);
	int rc = 0;

	hwlog_info("%s\n", __func__);
	AMS_MUTEX_LOCK(&chip->lock);
	if (chip->unpowered) {
		rc = amsdriver_power_on(chip);
		if (rc)
			goto chip_on_err;
	}

	rc = osal_als_enable_set(chip, AMSDRIVER_ALS_ENABLE);
	if (rc)
		amsdriver_pltf_power_off(chip);
chip_on_err:
	AMS_MUTEX_UNLOCK(&chip->lock);
	return 0;
}

static void osal_als_idev_close(struct input_dev *idev)
{
	if(NULL == idev){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return;
	}
	int rc = 0;
	struct colorDriver_chip *chip = dev_get_drvdata(&idev->dev);
	hwlog_info("%s\n", __func__);

	AMS_MUTEX_LOCK(&chip->lock);
	rc = osal_als_enable_set(chip, AMSDRIVER_ALS_DISABLE);
	if (rc){
		amsdriver_pltf_power_off(chip);
	}
	AMS_MUTEX_UNLOCK(&chip->lock);
}
#endif

static void amsdriver_work(struct work_struct *work)
{
	int ret = 0;
	bool re_enable = false;
	ams_tcs3707_ams_mode_t mode = 0;

	if(NULL == work){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return;
	}
	struct colorDriver_chip *chip = \
		container_of(work, struct colorDriver_chip, als_work);
	if(NULL == chip){
		hwlog_err("AMS_Driver: %s: Pointer chip is NULL\n", __func__);
		return;
	}
	AMS_MUTEX_LOCK(&chip->lock);
	if(0 == read_nv_first_in){
		ret = get_cal_para_from_nv();
		if(!ret){
			hwlog_err("\ams_tcs3707: get_cal_para_from_nv fail \n");
		}
		read_nv_first_in = -1;// -1: do not read again.
	}	
	re_enable = ams_tcs3707_deviceEventHandler((ams_tcs3707_deviceCtx_t*)chip->deviceCtx, chip->inCalMode);

	if (re_enable)
	{
		ams_tcs3707_deviceCtx_t* ctx = (ams_tcs3707_deviceCtx_t*)chip->deviceCtx;
		ams_tcs3707_setField(ctx->portHndl, TCS3707_ENABLE_REG, 0, AEN);
		ams_tcs3707_setField(ctx->portHndl, TCS3707_ENABLE_REG, AEN, AEN);
	}

#ifdef CONFIG_AMS_OPTICAL_SENSOR_ALS
	if (ams_tcs3707_deviceGetResult((ams_tcs3707_deviceCtx_t*)chip->deviceCtx) & AMS_TCS3707_FEATURE_ALS){
		if (chip->inCalMode == false && !re_enable){
			//hwlog_info( "amsdriver_work: osal_report_als\n");
			osal_report_als(chip);
		} else {
			hwlog_info( "amsdriver_work: calibration mode\n");
			osal_calHandl_als(chip);
		}
	}
#endif
	ams_tcs3707_deviceGetMode(chip->deviceCtx, &mode);
	if (((mode & AMS_TCS3707_MODE_ALS) == AMS_TCS3707_MODE_ALS)){
		if(chip->inCalMode == false){
			if (re_enable)
			{
				mod_timer(&chip->work_timer, jiffies + msecs_to_jiffies(106));// timer set as 106ms
				hwlog_info("crgb timer set as 106ms \n");
			}
			else
			{
				mod_timer(&chip->work_timer, jiffies + HZ/10);// timer set as 100ms
			}
		}else{
			mod_timer(&chip->work_timer, jiffies + msecs_to_jiffies(120));//calibrate mode set timer for 120ms
			hwlog_info("in calibrate mode mod timer set as 120ms \n");
		}
	}else{
		hwlog_warn("amsdriver_work: not mod timer mode = %d\n",mode);
	}

bypass:

	AMS_MUTEX_UNLOCK(&chip->lock);
}

#ifdef CONFIG_HUAWEI_DSM
static void amsdriver_dmd_work(void)
{
	if (!dsm_client_ocuppy(shb_dclient)) {
		if (color_devcheck_dmd_result == false){
			dsm_client_record(shb_dclient, "ap_color_sensor_detected fail\n");
			dsm_client_notify(shb_dclient, DSM_AP_ERR_COLORSENSOR_DETECT);
			hwlog_err("AMS_Driver: %s: DMD ap_color_sensor_detected fail\n", __func__);
		}
	}
}
#endif

int ams_tcs3707_probe(struct i2c_client *client,
	const struct i2c_device_id *idp)
{
	int ret = 0;
	int i = 0;
	hwlog_info("enter [%s] \n", __func__);
	if(NULL == client){
		hwlog_err("ams_tcs3707: %s: Pointer is NULL\n", __func__);
		return -1;
	}
	struct device *dev = &client->dev;
	static struct colorDriver_chip *chip = NULL;
	struct driver_i2c_platform_data *pdata = dev->platform_data;
	ams_tcs3707_deviceInfo_t amsDeviceInfo;
	ams_tcs3707_deviceIdentifier_e deviceId;

	/****************************************/
	/* Validate bus and device registration */
	/****************************************/
	if (!i2c_check_functionality(client->adapter,
			I2C_FUNC_SMBUS_BYTE_DATA)) {
		hwlog_err("%s: i2c smbus byte data unsupported\n", __func__);
		ret = -EOPNOTSUPP;
		goto init_failed;
	}

	chip = kzalloc(sizeof(struct colorDriver_chip), GFP_KERNEL);
	if (!chip) {
		ret = -ENOMEM;
		goto init_failed;
	}

	mutex_init(&chip->lock);
	chip->client = client;
	chip->pdata = pdata;
	i2c_set_clientdata(chip->client, chip);

	chip->in_suspend = 0;
	chip->inCalMode = false;
	chip->calibrationCtx.calState = 0;

	for (i = 0; i < CAL_STATE_GAIN_LAST; i++){
		chip->calibrationCtx.calCresult[i]  = AMS_TCS3707_FLOAT_TO_FIX ;
		chip->calibrationCtx.calRresult[i]  = AMS_TCS3707_FLOAT_TO_FIX ;
		chip->calibrationCtx.calGresult[i]  = AMS_TCS3707_FLOAT_TO_FIX ;
		chip->calibrationCtx.calBresult[i] = AMS_TCS3707_FLOAT_TO_FIX ;
		chip->calibrationCtx.calWresult[i] = AMS_TCS3707_FLOAT_TO_FIX ;
		color_nv_para.calCratio[i] = AMS_TCS3707_FLOAT_TO_FIX ;
		color_nv_para.calRratio[i]  = AMS_TCS3707_FLOAT_TO_FIX ;
		color_nv_para.calGratio[i]  = AMS_TCS3707_FLOAT_TO_FIX ;
		color_nv_para.calBratio[i] = AMS_TCS3707_FLOAT_TO_FIX ;
		color_nv_para.calWratio[i] = AMS_TCS3707_FLOAT_TO_FIX ;
	}

#ifdef CONFIG_HUAWEI_DSM
	INIT_DELAYED_WORK(&ams_tcs3707_dmd_work, amsdriver_dmd_work);
#endif
	/********************************************************************/
	/* Validate the appropriate ams device is available for this driver */
	/********************************************************************/
    deviceId = ams_tcs3707_testForDevice(chip->client);

	hwlog_info("ams_tcs3707: ams_tcs3707_testForDevice() %d \n", deviceId);

	if (deviceId == AMS_UNKNOWN_DEVICE) {
		hwlog_info( "ams_tcs3707_testForDevice failed: AMS_UNKNOWN_DEVICE\n");
#ifdef CONFIG_HUAWEI_DSM
		color_devcheck_dmd_result = false;
		schedule_delayed_work(&ams_tcs3707_dmd_work, msecs_to_jiffies(AP_COLOR_DMD_DELAY_TIME_MS));
#endif
		goto id_failed;
	}

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
	set_hw_dev_flag(DEV_I2C_AP_COLOR_SENSOR);
#endif
	ams_tcs3707_getDeviceInfo(&amsDeviceInfo);
	hwlog_info( "ams_amsDeviceInfo() ok\n");

	chip->deviceCtx = kzalloc(amsDeviceInfo.memorySize, GFP_KERNEL);
	if (chip->deviceCtx) {
		ret = ams_tcs3707_deviceInit(chip->deviceCtx, chip->client);
		if (ret == false){
			hwlog_info( "ams_amsDeviceInit() ok\n");
		} else {
			hwlog_info( "ams_deviceInit failed.\n");
			goto id_failed;
		}
	} else {
		hwlog_info( "ams_tcs3707 kzalloc failed.\n");
		goto malloc_failed;
	}

	/*********************/
	/* Initialize ALS    */
	/*********************/

	init_timer(&chip->work_timer);
	setup_timer(&chip->work_timer, osal_tcs3707_als_timerHndl, (unsigned long) chip);
	INIT_WORK(&chip->als_work, amsdriver_work);

	chip->at_color_show_calibrate_state = ams_tcs3707_show_calibrate;
	chip->at_color_store_calibrate_state = ams_tcs3707_store_calibrate;
	chip->color_enable_show_state = ams_tcs3707_show_enable;
	chip->color_enable_store_state = ams_tcs3707_store_enable;
	chip->color_sensor_getGain = ams_tcs3707_getGain;
	chip->color_sensor_setGain = ams_tcs3707_setGain;

	p_chip = chip;
	ret = color_register(chip);
	if(ret < 0){
		hwlog_err("\ams_tcs3707: color_register fail \n");
	}
	color_default_enable = ams_tcs3707_setenable;

	hwlog_info("[%s]ams_tcs3707 probe ok \n", __func__);
	return 0;

id_failed:
	if (chip->deviceCtx) kfree(chip->deviceCtx);
	i2c_set_clientdata(client, NULL);
malloc_failed:
	kfree(chip);

init_failed:
	dev_err(dev, "Probe failed.\n");
	return ret;
}

int ams_tcs3707_suspend(struct device *dev)
{
	if(NULL == dev){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return -1;
	}
	struct colorDriver_chip  *chip = dev_get_drvdata(dev);
	if(NULL == chip){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return -1;
	}

	hwlog_info( "%s\n", __func__);
	AMS_MUTEX_LOCK(&chip->lock);
	chip->in_suspend = 1;

	if (chip->wake_irq) {
		irq_set_irq_wake(chip->client->irq, 1);
	} else if (!chip->unpowered) {
		hwlog_info( "powering off\n");
		/* TODO
		   platform power off */
	}
	AMS_MUTEX_UNLOCK(&chip->lock);

	return 0;
}

int ams_tcs3707_resume(struct device *dev)
{
	if(NULL == dev){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return -1;
	}
	struct colorDriver_chip *chip = dev_get_drvdata(dev);
	if(NULL == chip){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return -1;
	}

	return 0;
	AMS_MUTEX_LOCK(&chip->lock);

	chip->in_suspend = 0;

	if (chip->wake_irq) {
		irq_set_irq_wake(chip->client->irq, 0);
		chip->wake_irq = 0;
	}

/* err_power: */
	AMS_MUTEX_UNLOCK(&chip->lock);

	return 0;
}

int ams_tcs3707_remove(struct i2c_client *client)
{
	if(NULL == client){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return -1;
	}
	struct colorDriver_chip *chip = i2c_get_clientdata(client);
	if(NULL == chip){
		hwlog_err("AMS_Driver: %s: Pointer is NULL\n", __func__);
		return -1;
	}

	free_irq(client->irq, chip);

	/* TODO
	   platform teardown */

	i2c_set_clientdata(client, NULL);
	kfree(chip->deviceCtx);
	kfree(chip);
	return 0;
}

static struct i2c_device_id amsdriver_idtable[] = {
	{"ams_tcs3707", 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, amsdriver_idtable);

static const struct dev_pm_ops ams_tcs3707_pm_ops = {
	.suspend = ams_tcs3707_suspend,
	.resume  = ams_tcs3707_resume,
};

static const struct of_device_id amsdriver_of_id_table[] = {
	{.compatible = "ams,tcs3707"},
	{},
};


static struct i2c_driver ams_tcs3707_driver = {
	.driver = {
		.name = "ams_tcs3707",
		.owner = THIS_MODULE,
		.of_match_table = amsdriver_of_id_table,
	},
	.id_table = amsdriver_idtable,
	.probe = ams_tcs3707_probe,
	.remove = ams_tcs3707_remove,
};

static int __init ams_tcs3707_init(void)
{
	int rc;
	hwlog_info("ams_tcs3707: init()\n");

	rc = i2c_add_driver(&ams_tcs3707_driver);

	printk(KERN_ERR "ams_tcs3707:  %d", rc);
	return rc;
}

static void __exit ams_tcs3707_exit(void)
{
	hwlog_info("ams_tcs3707: exit()\n");
	i2c_del_driver(&ams_tcs3707_driver);
}

module_init(ams_tcs3707_init);
module_exit(ams_tcs3707_exit);

MODULE_AUTHOR("AMS AOS Software<cs.americas@ams.com>");
MODULE_DESCRIPTION("AMS tcs3707 ALS, XYZ color sensor driver");
MODULE_LICENSE("GPL");
