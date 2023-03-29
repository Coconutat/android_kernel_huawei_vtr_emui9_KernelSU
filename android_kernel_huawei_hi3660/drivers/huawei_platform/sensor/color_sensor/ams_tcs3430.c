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
#include "ams_tcs3430.h"

HWLOG_REGIST();

/****************************************************************************
 *                     OSAL Device Com Interface
 ****************************************************************************/
static struct class *color_class;
static bool ams_tcs3430_getDeviceInfo(ams_tcs3430_deviceInfo_t * info);
static bool ams_tcs3430_deviceInit(ams_tcs3430_deviceCtx_t * ctx, AMS_PORT_portHndl * portHndl);
static bool ams_tcs3430_deviceEventHandler(ams_tcs3430_deviceCtx_t * ctx, bool inCalMode);
static ams_tcs3430_deviceIdentifier_e ams_tcs3430_testForDevice(AMS_PORT_portHndl * portHndl);
static bool ams_tcs3430_getDeviceInfo(ams_tcs3430_deviceInfo_t * info);
static int report_value[AMS_REPORT_DATA_LEN] = {0};
static struct colorDriver_chip *p_chip = NULL;
static bool color_calibrate_result = true;
static bool report_calibrate_result = false;
static color_sensor_cali_para_nv color_nv_para;
static int read_nv_first_in = 0;
static int enable_status_before_calibrate = 0;
#ifdef CONFIG_HUAWEI_DSM
static bool color_devcheck_dmd_result = true;
extern struct dsm_client* shb_dclient;
#endif
extern int ap_color_report(int value[], int length);
extern int color_register(struct colorDriver_chip *chip);
extern int read_color_calibrate_data_from_nv(int nv_number, int nv_size, char * nv_name, char * temp);
extern int write_color_calibrate_data_to_nv(int nv_number, int nv_size, char * nv_name, char * temp);
extern int (*color_default_enable)(bool enable);
struct delayed_work ams_dmd_work;
static uint8_t report_logcount = 0;

#if defined(CONFIG_AMS_OPTICAL_SENSOR_ALS)

typedef struct{
    uint32_t rawX;
    uint32_t rawY;
    uint32_t rawZ;
    uint32_t rawIR;
    uint32_t rawIR2;
}export_alsData_t;



#define AMS_TCS3430_LEFT_EDGE_DEFAULT		0.1
#define AMS_TCS3430_RIGHT_EDGE_DEFAULT		0.9

#define AMS_TCS3430_K_VALUE_DEFAULT		1.0
#define AMS_TCS3430_K_VALUE_MIN			0.8
#define AMS_TCS3430_K_VALUE_MAX			1.2
#define AMS_TCS3430_NUM_K_VALUE 		5

#define AMS_TCS3430_HIGH_IR_THRESH_DEFAULT	AMS_TCS3430_HIGH_IR_THRESH_MAX
#define AMS_TCS3430_HIGH_IR_THRESH_MIN		0
#define AMS_TCS3430_HIGH_IR_THRESH_MAX		0.5

#define AMS_TCS3430_CLAMP_DEFAULT               AMS_TCS3430_CLAMP_MAX
#define AMS_TCS3430_CLAMP_MIN			0
#define AMS_TCS3430_CLAMP_MAX			1

#define AMS_TCS3430_NOMINAL_ATIME_DEFAULT       100
#define AMS_TCS3430_NOMINAL_AGAIN_DEFAULT       16
#define AMS_TCS3430_ATIME_DEFAULT               700
#define AMS_TCS3430_AGAIN_DEFAULT               (64 * 1000)
#define AMS_TCS3430_GAIN_OF_GOLDEN              64
#define AMS_REPORT_LOG_COUNT_NUM                20

#define ONE_BYTE_LENGTH_8_BITS  (8)

typedef struct{
	UINT8                 deviceId;
	UINT8                 deviceIdMask;
	UINT8                 deviceRef;
	UINT8                 deviceRefMask;
	ams_tcs3430_deviceIdentifier_e  device;
}ams_tcs_3430_deviceIdentifier_t;

static ams_tcs_3430_deviceIdentifier_t const deviceIdentifier[]={
	{AMS_TCS3430_DEVICE_ID, MASK_ID, AMS_TCS3430_REV_ID, MASK_REVID, AMS_TCS3430_REV1},
	{AMS_TCS3430_DEVICE_ID, MASK_ID, 0, MASK_REVID, AMS_TCS3430_REV0},
	{0, 0, 0, 0, AMS_TCS3430_LAST_DEVICE}
};

#define AMS_TCS3430_GAIN_SCALE	(1000)
static UINT32 const ams_tcs3430_alsGain_conversion[] = {
	1 * AMS_TCS3430_GAIN_SCALE,
	4 * AMS_TCS3430_GAIN_SCALE,
	16 * AMS_TCS3430_GAIN_SCALE,
	64 * AMS_TCS3430_GAIN_SCALE,
	128 * AMS_TCS3430_GAIN_SCALE
};

static UINT8 const ams_tcs3430_als_gains[] = {
	1,
	4,
	16,
	64,
	128
};
const ams_tcs3430_gainCaliThreshold_t  ams_tcs3430_setGainThreshold[CAL_STATE_GAIN_LAST] = {
	{0, (100*AMS_TCS3430_FLOAT_TO_FIX)},//set threshold 1x to 0~100
	{(AMS_TCS3430_FLOAT_TO_FIX/AMS_TCS3430_CAL_THR), (AMS_TCS3430_FLOAT_TO_FIX*AMS_TCS3430_CAL_THR)},
	{(AMS_TCS3430_FLOAT_TO_FIX/AMS_TCS3430_CAL_THR), (AMS_TCS3430_FLOAT_TO_FIX*AMS_TCS3430_CAL_THR)},
	{(AMS_TCS3430_FLOAT_TO_FIX/AMS_TCS3430_CAL_THR), (AMS_TCS3430_FLOAT_TO_FIX*AMS_TCS3430_CAL_THR)},
	{(AMS_TCS3430_FLOAT_TO_FIX/AMS_TCS3430_CAL_THR), (AMS_TCS3430_FLOAT_TO_FIX*AMS_TCS3430_CAL_THR)},
};
const ams_tcs3430_deviceRegisterTable_t ams_tcs3430_deviceRegisterDefinition[AMS_TCS3430_DEVREG_REG_MAX] = {
	{ 0x80, 0x01 },          /* DEVREG_ENABLE */
	{ 0x81, 0x23 },          /* DEVREG_ATIME */
	{ 0x83, 0x00 },          /* DEVREG_WTIME */
	{ 0x84, 0x00 },          /* DEVREG_AILTL */
	{ 0x85, 0x00 },          /* DEVREG_AILTH */
	{ 0x86, 0x00 },          /* DEVREG_AIHTL */
	{ 0x87, 0x00 },          /* DEVREG_AIHTH */
	{ 0x8C, 0x00 },          /* DEVREG_PERS */
	{ 0x8D, 0x80 },          /* DEVREG_CFG0 */
	{ 0x90, 0x00 },          /* DEVREG_CFG1 */
	{ 0x91, AMS_TCS3430_REV_ID },    /* DEVREG_REVID */
	{ 0x92, AMS_TCS3430_DEVICE_ID }, /* DEVREG_ID */
	{ 0x93, 0x00 },          /* DEVREG_STATUS */
	{ 0x94, 0x00 },          /* DEVREG_CH0DATAL */
	{ 0x95, 0x00 },          /* DEVREG_CH0DATAH */
	{ 0x96, 0x00 },          /* DEVREG_CH1DATAL */
	{ 0x97, 0x00 },          /* DEVREG_CH1DATAH */
	{ 0x98, 0x00 },          /* DEVREG_CH2DATAL */
	{ 0x99, 0x00 },          /* DEVREG_CH2DATAH */
	{ 0x9A, 0x00 },          /* DEVREG_CH3DATAL */
	{ 0x9B, 0x00 },          /* DEVREG_CH3DATAH */
	{ 0x9F, 0x04 },          /* DEVREG_CFG2 */       /* Datasheet is wrong. Reset value is 0x04 */
	{ 0xAB, 0x00 },          /* DEVREG_CFG3 */
	{ 0xD6, 0x7F },          /* DEVREG_AZ_CONFIG */  /* Datasheet is wrong. Reset value is 0x7F */
	{ 0xDD, 0x00 },          /* DEVREG_INTENAB */
};

int AMS_PORT_TCS3430_getByte(AMS_PORT_portHndl * handle, uint8_t reg, uint8_t * data, uint8_t len){
    int ret;
    if ((handle == NULL) || (data == NULL )){
	    hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
	    return -EPERM;
    }

    ret = i2c_smbus_read_i2c_block_data(handle, reg, len, data);
    if (ret < 0)
	    dev_err(&handle->dev, "%s: failed at address %x (%d bytes)\n",
		    __func__, reg, len);

    return ret;
}

int AMS_PORT_TCS3430_setByte(AMS_PORT_portHndl * handle, uint8_t reg, uint8_t* data, uint8_t len){
	int ret;
	if ((handle == NULL) || (data == NULL) ){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
	    return -EPERM;
	}

	ret = i2c_smbus_write_i2c_block_data(handle, reg, len, data);
	if (ret < 0)
	    	dev_err(&handle->dev, "%s: failed at address %x (%d bytes)\n",
		    	__func__, reg, len);

	return ret;
}

static int ams_tcs3430_getByte(AMS_PORT_portHndl * portHndl, ams_tcs3430_deviceRegister_t reg, UINT8 * readData)
{
	int read_count = 0;
	UINT8 length = 1;

	if (portHndl == NULL || readData == NULL){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return read_count;
	}

	read_count = AMS_PORT_TCS3430_getByte(portHndl,
	                        ams_tcs3430_deviceRegisterDefinition[reg].address,
	                        readData,
	                        length);

	AMS_PORT_log_2("I2C reg getByte = 0x%02x, data: 0x%02x\n", ams_tcs3430_deviceRegisterDefinition[reg].address, (UINT8)readData[0]);

	return read_count;
}

static int ams_tcs3430_setByte(AMS_PORT_portHndl * portHndl, ams_tcs3430_deviceRegister_t reg, UINT8 data)
{
	int write_count = 0;
	UINT8 length = 1;

	if (portHndl == NULL ){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return write_count;
	}

	write_count = AMS_PORT_TCS3430_setByte(portHndl,
	                            ams_tcs3430_deviceRegisterDefinition[reg].address,
	                            &data,
	                            length);
	AMS_PORT_log_2("I2C reg setByte = 0x%02x, data: 0x%02x\n", ams_tcs3430_deviceRegisterDefinition[reg].address, (UINT8)data);
	return write_count;
}

static int ams_tcs3430_getBuf(AMS_PORT_portHndl * portHndl, ams_tcs3430_deviceRegister_t reg, UINT8 * readData, UINT8 length)
{
	int read_count = 0;

	if ((portHndl == NULL) || (readData == NULL) ){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return read_count;
	}

	read_count = AMS_PORT_TCS3430_getByte(portHndl,
	                        ams_tcs3430_deviceRegisterDefinition[reg].address,
	                        readData,
	                        length);

	//PRINT_INFO("I2C reg getBuf  = 0x%x, len= %d, data: 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", ams_tcs3430_deviceRegisterDefinition[reg].address, length,
	//	(UINT8)readData[0], (UINT8)readData[1], (UINT8)readData[2], (UINT8)readData[3], (UINT8)readData[4], (UINT8)readData[5], (UINT8)readData[6], (UINT8)readData[7] );
	return read_count;
}

static int ams_tcs3430_report_data(int value[])
{
	AMS_PORT_log("ams_tcs3430_report_data\n");
	return ap_color_report(value, AMS_REPORT_DATA_LEN*sizeof(int));
}
static int ams_tcs3430_setWord(AMS_PORT_portHndl * portHndl, ams_tcs3430_deviceRegister_t reg, UINT16 data)
{
	int write_count = 0;
	UINT8 length = sizeof(UINT16);
	UINT8 buffer[sizeof(UINT16)] = {0};

	if (portHndl == NULL){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return write_count;
	}

	buffer[0] = ((data >> AMS_ENDIAN_1) & 0xff);
	buffer[1] = ((data >> AMS_ENDIAN_2) & 0xff);

	write_count = AMS_PORT_TCS3430_setByte(portHndl,
	                            ams_tcs3430_deviceRegisterDefinition[reg].address,
	                            &buffer[0],
	                            length);
	return write_count;
}

static int ams_tcs3430_getField(AMS_PORT_portHndl * portHndl, ams_tcs3430_deviceRegister_t reg, UINT8 * readData, ams_tcs3430_regMask_t mask)
{
	int read_count = 0;
	UINT8 length = 1;

	if ((portHndl == NULL) || (readData == NULL) ){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return read_count;
	}

	read_count = AMS_PORT_TCS3430_getByte(portHndl,
	                            ams_tcs3430_deviceRegisterDefinition[reg].address,
	                            readData,
	                            length);

	*readData &= mask;

	return read_count;
}

static int ams_tcs3430_setField(AMS_PORT_portHndl * portHndl, ams_tcs3430_deviceRegister_t reg, UINT8 data, ams_tcs3430_regMask_t mask)
{
	int write_count = 1;
	UINT8 length = 1;
	UINT8 original_data = 0;
	UINT8 new_data = 0;

	if (portHndl == NULL){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return 0;
	}

	ams_tcs3430_getByte(portHndl,
	                    reg,
	                    &original_data);

	new_data = original_data & ~mask;
	new_data |= (data & mask);

	if (new_data != original_data){
	    write_count = ams_tcs3430_setByte(portHndl,
	                    reg,
	                    new_data);
	}

	return write_count;
}

static ams_tcs3430_deviceIdentifier_e ams_tcs3430_testForDevice(AMS_PORT_portHndl * portHndl){
	UINT8 chipId = 0;
	UINT8 revId = 0;
	UINT8 i = 0;

	if (portHndl == NULL){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return AMS_UNKNOWN_DEVICE;
	}

	ams_tcs3430_getByte(portHndl, AMS_TCS3430_DEVREG_ID, &chipId);
	ams_tcs3430_getByte(portHndl, AMS_TCS3430_DEVREG_REVID, &revId);
	AMS_PORT_log_2("ams_tcs3430_testForDevice: 0x%02x 0x%02x\n", chipId, revId);

	do{
		if (((chipId & deviceIdentifier[i].deviceIdMask) ==
		    (deviceIdentifier[i].deviceId & deviceIdentifier[i].deviceIdMask)) &&
		    ((revId & deviceIdentifier[i].deviceRefMask) ==
		     (deviceIdentifier[i].deviceRef & deviceIdentifier[i].deviceRefMask))){
		        	return deviceIdentifier[i].device;
		}
	    i++;
	}while (deviceIdentifier[i].device != AMS_TCS3430_LAST_DEVICE);

	return AMS_UNKNOWN_DEVICE;
}

static void ams_tcs3430_resetAllRegisters(AMS_PORT_portHndl * portHndl){
	ams_tcs3430_deviceRegister_t i = 0;

	if (portHndl == NULL){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return;
	}

	AMS_PORT_log("_3430_resetAllRegisters\n");

	for (i = AMS_TCS3430_DEVREG_ENABLE; i <= AMS_TCS3430_DEVREG_CFG1; i++) {
	    	ams_tcs3430_setByte(portHndl, i, ams_tcs3430_deviceRegisterDefinition[i].resetValue);
	}

	/* Skipping REVID and ID */

	for (i = AMS_TCS3430_DEVREG_STATUS; i < AMS_TCS3430_DEVREG_REG_MAX; i++) {
	    	ams_tcs3430_setByte(portHndl, i, ams_tcs3430_deviceRegisterDefinition[i].resetValue);
	}
}

static UINT8 ams_tcs3430_gainToReg(UINT32 x){
	UINT8 i;

	for (i = sizeof(ams_tcs3430_alsGain_conversion)/sizeof(UINT32)-1; i != 0; i--) {
	    	if (x >= ams_tcs3430_alsGain_conversion[i]) break;
	}
	AMS_PORT_log_2("ams_tcs3430_gainToReg: %d %d\n", x, i);
	return (i);
}


static INT32 ams_tcs3430_getGain(ams_tcs3430_deviceCtx_t * ctx){
	UINT8 cfg1_reg_data = 0;
	UINT8 cfg2_reg_data = 0;
	INT32 gain = 0;

	if (ctx == NULL){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return true;
	}

	ams_tcs3430_getField(ctx->portHndl, AMS_TCS3430_DEVREG_CFG1, &cfg1_reg_data, MASK_AGAIN);
	ams_tcs3430_getField(ctx->portHndl, AMS_TCS3430_DEVREG_CFG2, &cfg2_reg_data, MASK_HGAIN);
	if (cfg2_reg_data)
	{
		if (cfg1_reg_data == AGAIN_64)
		{
		   	gain = ams_tcs3430_als_gains[cfg1_reg_data + 1];
		}
		else
		{
		   	AMS_PORT_log_Msg(AMS_DEBUG,"WARNING: illegal gain setting, HGAIN set but AGAIN not 64x\n");
		   	gain = ams_tcs3430_als_gains[cfg1_reg_data]; /* gain is undetermined, but the algorithm
		                                     * needs a non-zero value */
		}
	}
	else
	{
	    gain = ams_tcs3430_als_gains[cfg1_reg_data];
	}
	return gain;
}

static INT32 ams_tcs3430_setGain(ams_tcs3430_deviceCtx_t * ctx, uint32_t gain){
	INT32 ret = 0;

	if (ctx == NULL){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return true;
	}

	AMS_PORT_log_1("ams_tcs3430_setGain: %d\n", gain);

	if (gain >= ams_tcs3430_alsGain_conversion[(sizeof(ams_tcs3430_alsGain_conversion) / sizeof(uint32_t)) -1])
	{
	   	 /* First, set AGAIN to 64x in CFG1*/
	    ret += ams_tcs3430_setField(ctx->portHndl, AMS_TCS3430_DEVREG_CFG1, AGAIN_64, MASK_AGAIN);

	    	/* Second set the HGAIN bit in CFG2*/
	    ret += ams_tcs3430_setField(ctx->portHndl, AMS_TCS3430_DEVREG_CFG2, HGAIN, MASK_HGAIN);
	}
	else
	{
	   	 /* Make sure HGAIN bit is clear in CFG2*/
	    ret += ams_tcs3430_setField(ctx->portHndl, AMS_TCS3430_DEVREG_CFG2, 0, MASK_HGAIN);

	    	/* Set AGAIN in CFG1 */
	    ret += ams_tcs3430_setField(ctx->portHndl, AMS_TCS3430_DEVREG_CFG1, ams_tcs3430_gainToReg(gain), MASK_AGAIN);
	}

	ctx->algCtx.als_data.gain =  gain;

	return (ret);
}

static INT32 ams_tcs3430_setAtimeInMs(ams_tcs3430_deviceCtx_t * ctx, int atime_ms){
	INT32 ret = 0;
	UINT8 atime_reg_data;

	if (ctx == NULL){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return true;
	}

	atime_reg_data = AMS_TCS3430_MS_TO_ATIME(atime_ms);
	ret = ams_tcs3430_setByte(ctx->portHndl, AMS_TCS3430_DEVREG_ATIME, atime_reg_data);

	return (ret);
}

static bool ams_tcs3430_alsRegUpdate(ams_tcs3430_deviceCtx_t * ctx, bool inCalMode){

	if (ctx == NULL){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return true;
	}

	/* For first integration after AEN */
	ctx->first_inte = false;
	if (!(ctx->mode & AMS_TCS3430_MODE_ALS) && !inCalMode)
	{
		ctx->algCtx.als_data.gain = 16 * AMS_TCS3430_GAIN_SCALE;
		ctx->algCtx.als_data.atime_ms = 2;
		ctx->first_inte = true;
	}

	ams_tcs3430_setAtimeInMs(ctx, ctx->algCtx.als_data.atime_ms);
	ams_tcs3430_setGain(ctx, ctx->algCtx.als_data.gain);
	ams_tcs3430_setField(ctx->portHndl, AMS_TCS3430_DEVREG_PERS, 0x00, MASK_APERS);
	/* set thresholds such that it would generate an interrupt */
	ams_tcs3430_setWord(ctx->portHndl, AMS_TCS3430_DEVREG_AILTL, 0xffff);  /* 2 bytes, AILTL and AILTH */
	ams_tcs3430_setWord(ctx->portHndl, AMS_TCS3430_DEVREG_AIHTL, 0x0000);  /* 2 bytes, AIHTL and AIHTH */

    return false;
}

/* --------------------------------------------------------------------*/
/* Set DCB config parameters                                           */
/* --------------------------------------------------------------------*/
static bool ams_tcs3430_deviceSetConfig(ams_tcs3430_deviceCtx_t * ctx, ams_tcs3430_configureFeature_t feature,
										ams_tcs3430_deviceConfigOptions_t option, UINT32 data, bool inCalMode){

	if (ctx == NULL){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return true;
	}

	if (feature == AMS_TCS3430_FEATURE_ALS){
	   	AMS_PORT_log_Msg(AMS_DEBUG, "ams_configureFeature_t  AMS_CONFIG_ALS_LUX\n");
	    	switch (option)
	    	{
	        	case AMS_TCS3430_CONFIG_ENABLE: /* ON / OFF */
	            	AMS_PORT_log_Msg_1(AMS_DEBUG,"deviceConfigOptions_t   AMS_CONFIG_ENABLE(%u)\n", data);
	            	AMS_PORT_log_Msg_1(AMS_DEBUG,"current mode            %d\n", ctx->mode);
	            	if (data == 0) {
						ams_tcs3430_setByte(ctx->portHndl, AMS_TCS3430_DEVREG_ENABLE, PON);
						ams_tcs3430_setByte(ctx->portHndl, AMS_TCS3430_DEVREG_INTENAB, 0x00);
						ams_tcs3430_setByte(ctx->portHndl, AMS_TCS3430_DEVREG_STATUS, (MASK_ASAT | MASK_AINT));
						ctx->mode = AMS_TCS3430_MODE_OFF;
	            	} else {
						ams_tcs3430_alsRegUpdate(ctx, inCalMode);
						ams_tcs3430_setByte(ctx->portHndl, AMS_TCS3430_DEVREG_INTENAB, AIEN);
						ams_tcs3430_setByte(ctx->portHndl, AMS_TCS3430_DEVREG_ENABLE, (AEN | PON));
						ams_tcs3430_setByte(ctx->portHndl, AMS_TCS3430_DEVREG_STATUS, (MASK_ASAT | MASK_AINT));
						ctx->mode |= AMS_TCS3430_MODE_ALS;
	            	}
	            	break;
	        default:
	            	break;
	    	}
	}
	return 0;
}

static bool ams_tcs3430_get_max_min_raw(ams_tcs3430_adc_data_t *current_raw, uint32_t* max, uint32_t *min)
{
	*max = current_raw->x;
	if (current_raw->y > *max)
	{
		*max = current_raw->y;
	}
	if (current_raw->z > *max)
	{
		*max = current_raw->z;
	}
	if (current_raw->ir1 > *max)
	{
		*max = current_raw->ir1;
	}

	*min = current_raw->x;
	if (current_raw->y < *min)
	{
		*min = current_raw->y;
	}
	if (current_raw->z < *min)
	{
		*min = current_raw->z;
	}
}

static bool ams_tcs3430_saturation_check(ams_tcs3430_deviceCtx_t * ctx, ams_tcs3430_adc_data_t *current_raw)
{
	uint32_t saturation = 0;
	saturation = (AMS_TCS3430_MS_TO_ATIME(ctx->algCtx.als_data.atime_ms) + 1) << 10 - 1;//calculate saturation value
	saturation *= 9;//threadhold ratio 0.9 for saturation
	saturation /= 10;//threadhold ratio 0.9 for saturation

	if (current_raw->x > saturation ||
		current_raw->y > saturation ||
		current_raw->z > saturation ||
		current_raw->ir1 > saturation)
	{
		return true;
	}
	return false;
}

static bool ams_tcs343_insufficience_check(ams_tcs3430_deviceCtx_t * ctx, ams_tcs3430_adc_data_t *current_raw)
{
	if (current_raw->x < AMS_TCS3430_LOW_LEVEL ||
		current_raw->y < AMS_TCS3430_LOW_LEVEL ||
		current_raw->z < AMS_TCS3430_LOW_LEVEL)
	{
		return true;
	}
	return false;
}

static bool ams_tcs3430_handleALS(ams_tcs3430_deviceCtx_t * ctx, bool inCalMode){
	UINT8 adc_data[AMS_TCS3430_ADC_BYTES] = {0};
	ams_tcs3430_adc_data_t current_raw = {
		.x   = 0,
		.y   = 0,
		.z   = 0,
		.ir1 = 0,
		.ir2 = 0};
	uint8_t amux_data = 0;
	bool re_enable = false;

	if (ctx == NULL){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return true;
	}

	/* read ADC data */
	ams_tcs3430_getBuf(ctx->portHndl, AMS_TCS3430_DEVREG_CH0DATAL, adc_data, AMS_TCS3430_ADC_BYTES);
	ams_tcs3430_getField(ctx->portHndl, AMS_TCS3430_DEVREG_CFG1, &amux_data, MASK_AMUX);

	/* extract raw X/IR2 channel data */
	switch (ctx->deviceId){
		case AMS_TCS3430_REV0:{
			current_raw.x   = (adc_data[0] << 0) | (adc_data[1] << ONE_BYTE_LENGTH_8_BITS);
			current_raw.y   = (adc_data[2] << 0) | (adc_data[3] << ONE_BYTE_LENGTH_8_BITS);
			current_raw.ir2 = (adc_data[4] << 0) | (adc_data[5] << ONE_BYTE_LENGTH_8_BITS);
			if (amux_data == 0){
				current_raw.z   = (adc_data[6] << 0) | (adc_data[7] << ONE_BYTE_LENGTH_8_BITS);
			} else {
				current_raw.ir1 = (adc_data[6] << 0) | (adc_data[7] << ONE_BYTE_LENGTH_8_BITS);
			}
			break;
		}
		case AMS_TCS3430_REV1:{
			current_raw.z   = (adc_data[0] << 0) | (adc_data[1] << ONE_BYTE_LENGTH_8_BITS);
			current_raw.y   = (adc_data[2] << 0) | (adc_data[3] << ONE_BYTE_LENGTH_8_BITS);
			current_raw.ir1 = (adc_data[4] << 0) | (adc_data[5] << ONE_BYTE_LENGTH_8_BITS);
			if (amux_data == 0){
				current_raw.x   = (adc_data[6] << 0) | (adc_data[7] << ONE_BYTE_LENGTH_8_BITS);
			} else {
				current_raw.ir2 = (adc_data[6] << 0) | (adc_data[7] << ONE_BYTE_LENGTH_8_BITS);
			}
			break;
		}
		default:{
			break;
		}
	}

	if (inCalMode)
	{
		ctx->updateAvailable |= AMS_TCS3430_FEATURE_ALS;
		goto handleALS_exit;
	}

	/* First integration is special */
	if (ctx->first_inte)
	{
		/* Get max and min raw data */
		uint32_t max_raw, min_raw;
		ams_tcs3430_get_max_min_raw(&current_raw, &max_raw, &min_raw);
		AMS_PORT_log_1("ams_tcs3430_handleALS: adc   max=%d\n", (UINT16)max_raw);
		AMS_PORT_log_1("ams_tcs3430_handleALS: adc   min=%d\n", (UINT16)min_raw);

		/* Decide the proper gain setting */
		if (max_raw < 100)//threadhold of 128x
		{
			ams_tcs3430_setGain(ctx, (128 * AMS_TCS3430_GAIN_SCALE));//Set Gain to 128x
		}
		else if (max_raw < 200)//threadhold of 64x
		{
			ams_tcs3430_setGain(ctx, (64 * AMS_TCS3430_GAIN_SCALE));//Set Gain to 64x
		}
		else if (max_raw < 800)//threadhold of 16x
		{
			//Keep 16x Gain
		}
		else if (min_raw > 60)//threadhold of 1x
		{
			ams_tcs3430_setGain(ctx, (1 * AMS_TCS3430_GAIN_SCALE));//Set Gain to 1x
		}
		else
		{
			ams_tcs3430_setGain(ctx, (4 * AMS_TCS3430_GAIN_SCALE));//Set Gain to 4x
		}
		ctx->algCtx.als_data.atime_ms = AMS_TCS3430_NOMINAL_ATIME_DEFAULT;
		ams_tcs3430_setAtimeInMs(ctx, ctx->algCtx.als_data.atime_ms);
		re_enable = true;
		ctx->first_inte = false;
		goto handleALS_exit;
	}

	/* Adjust gain setting */
	if (ams_tcs3430_saturation_check(ctx, &current_raw) &&
			ctx->algCtx.als_data.gain == (4 * AMS_TCS3430_GAIN_SCALE))
	{
		ams_tcs3430_setGain(ctx, (1 * AMS_TCS3430_GAIN_SCALE));//Set Gain to 1x
		re_enable = true;
	}
	else if ((ams_tcs3430_saturation_check(ctx, &current_raw) &&
				ctx->algCtx.als_data.gain == (16 * AMS_TCS3430_GAIN_SCALE)) ||
			(ams_tcs343_insufficience_check(ctx, &current_raw) &&
				ctx->algCtx.als_data.gain == (1 * AMS_TCS3430_GAIN_SCALE)))//between 1x and 16x
	{
		ams_tcs3430_setGain(ctx, (4 * AMS_TCS3430_GAIN_SCALE));//Set Gain to 4x
		re_enable = true;
	}
	else if ((ams_tcs3430_saturation_check(ctx, &current_raw) &&
				ctx->algCtx.als_data.gain == (64 * AMS_TCS3430_GAIN_SCALE)) ||
			(ams_tcs343_insufficience_check(ctx, &current_raw) &&
				ctx->algCtx.als_data.gain == (4 * AMS_TCS3430_GAIN_SCALE)))//between 4x and 64x
	{
		ams_tcs3430_setGain(ctx, (16 * AMS_TCS3430_GAIN_SCALE));//Set Gain to 16x
		re_enable = true;
	}
	else if ((ams_tcs3430_saturation_check(ctx, &current_raw) &&
				ctx->algCtx.als_data.gain == (128 * AMS_TCS3430_GAIN_SCALE)) ||
			(ams_tcs343_insufficience_check(ctx, &current_raw) &&
				ctx->algCtx.als_data.gain == (16 * AMS_TCS3430_GAIN_SCALE)))//between 16x and 128x
	{
		ams_tcs3430_setGain(ctx, (64 * AMS_TCS3430_GAIN_SCALE));//Set Gain to 64x
		re_enable = true;
	}
	else if (ams_tcs343_insufficience_check(ctx, &current_raw) &&
				ctx->algCtx.als_data.gain == (64 * AMS_TCS3430_GAIN_SCALE))//64x
	{
		ams_tcs3430_setGain(ctx, (128 * AMS_TCS3430_GAIN_SCALE));//Set Gain to 128x
		re_enable = true;
	}
	else
	{
		ctx->updateAvailable |= AMS_TCS3430_FEATURE_ALS;
	}

handleALS_exit:
	if (!re_enable)
	{
		ctx->algCtx.als_data.datasetArray.x   = current_raw.x;
		ctx->algCtx.als_data.datasetArray.y   = current_raw.y;
		ctx->algCtx.als_data.datasetArray.z   = current_raw.z;
		ctx->algCtx.als_data.datasetArray.ir1 = current_raw.ir1;
		ctx->algCtx.als_data.datasetArray.ir2 = current_raw.ir2;
	}

#if 1
	AMS_PORT_log_2("ams_tcs3430_handleALS: ATIME = %d, AGAIN = %d\n", \
		(UINT16)ctx->algCtx.als_data.atime_ms, \
		(UINT32)ctx->algCtx.als_data.gain);
#endif

	return re_enable;
}

static void ams_tcs3430_amuxToggle(ams_tcs3430_deviceCtx_t * ctx)
{
    uint8_t reg_data =0;

	if (ctx == NULL){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return true;
	}

    ams_tcs3430_getField(ctx->portHndl, AMS_TCS3430_DEVREG_CFG1, &reg_data, MASK_AMUX);

    if (reg_data & MASK_AMUX)
       	reg_data &= ~MASK_AMUX;  /* clear */
    else
        reg_data |= MASK_AMUX;   /* set */

    ams_tcs3430_setField(ctx->portHndl, AMS_TCS3430_DEVREG_CFG1, reg_data, MASK_AMUX);
}

/* --------------------------------------------------------------------*/
/* Called by the OSAL interrupt service routine                        */
/* --------------------------------------------------------------------*/
static bool ams_tcs3430_deviceEventHandler(ams_tcs3430_deviceCtx_t * ctx, bool inCalMode)
{
    bool ret = false;

	if (ctx == NULL){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return false;
	}

	ams_tcs3430_getByte(ctx->portHndl, AMS_TCS3430_DEVREG_STATUS, &ctx->algCtx.als_data.status);

	//PRINT_INFO("ams_tcs3430_deviceEventHandler: 0x%x\n", ctx->algCtx.als_data.status);
    if (ctx->algCtx.als_data.status & (MASK_AINT)){
	    if (ctx->mode & AMS_TCS3430_MODE_ALS){
			ret = ams_tcs3430_handleALS(ctx, inCalMode);
	        /*ams_tcs3430_amuxToggle(ctx);*/
			/* Clear the interrupt */
			//ams_tcs3430_setField(ctx->portHndl, AMS_TCS3430_DEVREG_STATUS, (MASK_ASAT | MASK_AINT), AMS_TCS3430_HIGH);
			ams_tcs3430_setByte(ctx->portHndl, AMS_TCS3430_DEVREG_STATUS, (MASK_ASAT | MASK_AINT));
		}
    }
    return ret;
}

static bool ams_tcs3430_deviceGetMode(ams_tcs3430_deviceCtx_t * ctx, ams_tcs3430_ams_mode_t *mode)
{
	if ((ctx == NULL) || (mode == NULL)){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return true;
	}

    *mode = ctx->mode;
    return false;
}

static uint32_t ams_tcs3430_deviceGetResult(ams_tcs3430_deviceCtx_t * ctx)
{
	if (ctx == NULL){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return true;
	}
	return ctx->updateAvailable;
}

static bool ams_tcs3430_deviceGetAls(ams_tcs3430_deviceCtx_t * ctx, export_alsData_t * exportData)
{
	if ((ctx == NULL) || (exportData == NULL)){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return true;
	}
	ctx->updateAvailable &= ~(AMS_TCS3430_FEATURE_ALS);

	exportData->rawX = ctx->algCtx.als_data.datasetArray.x;
	exportData->rawY = ctx->algCtx.als_data.datasetArray.y;
	exportData->rawZ = ctx->algCtx.als_data.datasetArray.z;
	exportData->rawIR = ctx->algCtx.als_data.datasetArray.ir1;
	exportData->rawIR2 = ctx->algCtx.als_data.datasetArray.ir2;

    return false;
}



/* --------------------------------------------------------------------
 * Return default calibration data
 * --------------------------------------------------------------------*/
static void ams_tcs3430_getDefaultCalData(ams_tcs3430_calibrationData_t *cal_data)
{
	if (cal_data == NULL){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return true;
	}

    cal_data->timeBase_us = AMS_TCS3430_USEC_PER_TICK;

    cal_data->calibbrationData.nominal_atime = AMS_TCS3430_NOMINAL_ATIME_DEFAULT;
    cal_data->calibbrationData.nominal_again = AMS_TCS3430_NOMINAL_AGAIN_DEFAULT;

	return;
}

static bool ams_tcs3430_deviceInit(ams_tcs3430_deviceCtx_t * ctx, AMS_PORT_portHndl * portHndl)
{
    int ret = 0;

	if ((ctx == NULL) || (portHndl == NULL)){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return true;
	}

    AMS_PORT_log("ams_deviceInit\n");

    memset(ctx, 0, sizeof(ams_tcs3430_deviceCtx_t));
    ctx->portHndl = portHndl;
    ctx->mode = AMS_TCS3430_MODE_OFF;
    ctx->deviceId = ams_tcs3430_testForDevice(ctx->portHndl);
	AMS_PORT_log("ams_deviceInit - 0\n");

    ams_tcs3430_resetAllRegisters(ctx->portHndl);
	AMS_PORT_log("ams_deviceInit - 1\n");

    ctx->algCtx.als_data.atime_ms = AMS_TCS3430_NOMINAL_ATIME_DEFAULT;
    ctx->algCtx.als_data.gain = AMS_TCS3430_AGAIN_DEFAULT;

	ams_tcs3430_setGain(ctx, AMS_TCS3430_AGAIN_DEFAULT);

	AMS_PORT_log("ams_deviceInit - enable\n");
	ams_tcs3430_setByte(ctx->portHndl, AMS_TCS3430_DEVREG_ENABLE, (AEN | PON));
	msleep(8);//sleep 8ms start the AZ_MODE
	ams_tcs3430_setByte(ctx->portHndl, AMS_TCS3430_DEVREG_ENABLE, PON);
	AMS_PORT_log("ams_deviceInit - disable\n");
	AMS_PORT_log("ams_deviceInit - 2\n");

    return ret;
}

static bool ams_tcs3430_getDeviceInfo(ams_tcs3430_deviceInfo_t * info)
{
	if (info == NULL){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return true;
	}
	memset(info, 0, sizeof(ams_tcs3430_deviceInfo_t));
	info->memorySize =  sizeof(ams_tcs3430_deviceCtx_t);
	ams_tcs3430_getDefaultCalData(&info->defaultCalibrationData);
	return false;
}

void osal_als_timerHndl(unsigned long data)
{
	struct colorDriver_chip *chip = (struct colorDriver_chip*) data;

	if (chip == NULL){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return;
	}
	schedule_work(&chip->als_work);
}

static ssize_t osal_als_enable_set(struct colorDriver_chip *chip, uint8_t valueToSet)
{
	ssize_t rc = 0;
	ams_tcs3430_ams_mode_t mode = 0;

	if (chip == NULL){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return true;
	}

	ams_tcs3430_deviceGetMode(chip->deviceCtx, &mode);

#ifdef CONFIG_AMS_OPTICAL_SENSOR_ALS
	rc |= ams_tcs3430_deviceSetConfig(chip->deviceCtx, AMS_TCS3430_FEATURE_ALS, AMS_TCS3430_CONFIG_ENABLE, valueToSet, chip->inCalMode);
#endif
    hwlog_info("\n\nosal_als_enable_set = %d\n",valueToSet);

#ifndef CONFIG_AMS_OPTICAL_SENSOR_IRQ
    if (valueToSet){
		if ((mode & AMS_TCS3430_MODE_ALS) != AMS_TCS3430_MODE_ALS){
			if(chip->inCalMode == false){
				msleep(3);// first enable sleep 3ms for auto gain
				hwlog_info("first enable sleep 3ms \n");
				schedule_work(&chip->als_work);
				report_logcount = AMS_REPORT_LOG_COUNT_NUM;
			}else{
				mod_timer(&chip->work_timer, jiffies + msecs_to_jiffies(120));//first enable set the timer as 120ms
				hwlog_info("in calibrate mode timer set as 120ms \n");
			}
			dev_info(&chip->client->dev, "osal_als_enable_set: add_timer\n");
		} else {
			dev_info(&chip->client->dev, "osal_als_enable_set: timer already running\n");
		}
    } else {
		if ((mode & AMS_TCS3430_MODE_ALS) == AMS_TCS3430_MODE_ALS){
			dev_info(&chip->client->dev, "osal_als_enable_set: del_timer\n");
		}
    }
#endif

	return 0;
}
static int get_cal_para_from_nv(void)
{
	int i,ret;

	ret = read_color_calibrate_data_from_nv(RGBAP_CALI_DATA_NV_NUM, RGBAP_CALI_DATA_SIZE, "RGBAP", &color_nv_para);
	if(ret < 0){
		hwlog_err("\nAMS_Driver: %s: fail,use default para!!\n", __func__);
		for (i = 0; i < CAL_STATE_GAIN_LAST; i++){
			hwlog_err("\nAMS_Driver: get_cal_para_from_nv: gain[%d]: [%d, %d, %d, %d]\n", i,
			color_nv_para.calXratio[i], color_nv_para.calYratio[i], color_nv_para.calZratio[i], color_nv_para.calIratio[i]);
		}
		return 0;
	}

	for (i = 0; i < CAL_STATE_GAIN_LAST; i++){
		hwlog_info("\nAMS_Driver: get_cal_para_from_nv: gain[%d]: [%d, %d, %d, %d]\n", i,
		color_nv_para.calXratio[i], color_nv_para.calYratio[i], color_nv_para.calZratio[i], color_nv_para.calIratio[i]);
		if(!color_nv_para.calXratio[i]||!color_nv_para.calYratio[i]
			||!color_nv_para.calZratio[i]||!color_nv_para.calIratio[i]){

			color_nv_para.calXratio[i] = AMS_TCS3430_FLOAT_TO_FIX ;
			color_nv_para.calYratio[i]  = AMS_TCS3430_FLOAT_TO_FIX ;
			color_nv_para.calZratio[i]  = AMS_TCS3430_FLOAT_TO_FIX ;
			color_nv_para.calIratio[i] = AMS_TCS3430_FLOAT_TO_FIX ;
		}else if (color_nv_para.calXratio[i]>=FLOAT_TO_FIX_LOW/AMS_TCS3430_CAL_THR
				&&color_nv_para.calXratio[i]<=FLOAT_TO_FIX_LOW*AMS_TCS3430_CAL_THR
				&&color_nv_para.calYratio[i]>=FLOAT_TO_FIX_LOW/AMS_TCS3430_CAL_THR
				&&color_nv_para.calYratio[i]<=FLOAT_TO_FIX_LOW*AMS_TCS3430_CAL_THR
				&&color_nv_para.calZratio[i]>=FLOAT_TO_FIX_LOW/AMS_TCS3430_CAL_THR
				&&color_nv_para.calZratio[i]<=FLOAT_TO_FIX_LOW*AMS_TCS3430_CAL_THR
				&&color_nv_para.calIratio[i]>=FLOAT_TO_FIX_LOW/AMS_TCS3430_CAL_THR
				&&color_nv_para.calIratio[i]<=FLOAT_TO_FIX_LOW*AMS_TCS3430_CAL_THR){
			color_nv_para.calXratio[i] *= (AMS_TCS3430_FLOAT_TO_FIX/FLOAT_TO_FIX_LOW);
			color_nv_para.calYratio[i] *= (AMS_TCS3430_FLOAT_TO_FIX/FLOAT_TO_FIX_LOW);
			color_nv_para.calZratio[i] *= (AMS_TCS3430_FLOAT_TO_FIX/FLOAT_TO_FIX_LOW);
			color_nv_para.calIratio[i] *= (AMS_TCS3430_FLOAT_TO_FIX/FLOAT_TO_FIX_LOW);
			hwlog_info("AMS_Driver: low_level nv calibrate data\n");
		}
	}
	for (i = 0; i <= CAL_STATE_GAIN_2; i++){
		color_nv_para.calXratio[i] = color_nv_para.calXratio[CAL_STATE_GAIN_3] ;
		color_nv_para.calYratio[i] = color_nv_para.calYratio[CAL_STATE_GAIN_3] ;
		color_nv_para.calZratio[i] = color_nv_para.calZratio[CAL_STATE_GAIN_3] ;
		color_nv_para.calIratio[i] = color_nv_para.calIratio[CAL_STATE_GAIN_3] ;
		if(i == CAL_STATE_GAIN_1){
			hwlog_info("AMS_Driver: 1xGain equal to 16xGain for nv calibrate data\n");
		}else if(i == CAL_STATE_GAIN_2){
			hwlog_info("AMS_Driver: 4xGain equal to 16xGain for nv calibrate data\n");
		}
	}
	return 1;
}
static int save_cal_para_to_nv(struct colorDriver_chip *chip)
{
	int i = 0, ret;
	if (chip == NULL){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return 0;
	}

	color_nv_para.nv_Xtarget = chip->calibrationCtx.calXtarget;
	color_nv_para.nv_Ytarget = chip->calibrationCtx.calYtarget;
	color_nv_para.nv_Ztarget = chip->calibrationCtx.calZtarget;
	color_nv_para.nv_IRtarget = chip->calibrationCtx.calIRtarget;

	for(i = 0; i < CAL_STATE_GAIN_LAST; i++){
		color_nv_para.calXratio[i] = chip->calibrationCtx.calXresult[i];
		color_nv_para.calYratio[i]  = chip->calibrationCtx.calYresult[i];
		color_nv_para.calZratio[i]  = chip->calibrationCtx.calZresult[i];
		color_nv_para.calIratio[i] = chip->calibrationCtx.calIRresult[i];
		hwlog_info("\nAMS_Driver: save_cal_para_to_nv: gain[%d]: [%d, %d, %d, %d]\n", i,
		color_nv_para.calXratio[i], color_nv_para.calYratio[i], color_nv_para.calZratio[i], color_nv_para.calIratio[i]);

	}

	ret = write_color_calibrate_data_to_nv(RGBAP_CALI_DATA_NV_NUM, RGBAP_CALI_DATA_SIZE, "RGBAP", &color_nv_para);
	if(ret < 0){
		hwlog_err("\nAMS_Driver: %s: fail\n", __func__);
	}
	return 1;
}

static void osal_calHandl_als(struct colorDriver_chip *chip){
	export_alsData_t outData;
	ams_tcs3430_deviceCtx_t * ctx = NULL;
	uint32_t currentGain = 0;
	uint32_t result; /* remember, this is for fixed point and can cause lower performance */

	if (chip == NULL){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return;
	}
	if (chip->deviceCtx == NULL){
		hwlog_err("\nAMS_Driver: %s: deviceCtx is NULL\n", __func__);
		return;
	}

	ctx = chip->deviceCtx;
	currentGain = (ctx->algCtx.als_data.gain / AMS_TCS3430_GAIN_SCALE);

	ams_tcs3430_deviceGetAls(chip->deviceCtx, &outData);

	dev_info(&chip->client->dev, "osal_calHandl_als: state %d\n", chip->calibrationCtx.calState);
	dev_info(&chip->client->dev, "osal_calHandl_als: count %d\n", chip->calibrationCtx.calSampleCounter);

	if (chip->calibrationCtx.calState < CAL_STATE_GAIN_LAST && chip->calibrationCtx.calState >= 0){
		chip->calibrationCtx.calSampleCounter++;
		chip->calibrationCtx.calXsample += outData.rawX;
		chip->calibrationCtx.calYsample += outData.rawY;
		chip->calibrationCtx.calZsample += outData.rawZ;
		chip->calibrationCtx.calIRsample += outData.rawIR;

		if((chip->calibrationCtx.calState < CAL_STATE_GAIN_LAST) && (chip->calibrationCtx.calState >= 0)){
			result = (chip->calibrationCtx.calXsample / AMS_TCS3430_CAL_AVERAGE);
			if (result){
				result = (chip->calibrationCtx.calXtarget * (currentGain *
					AMS_TCS3430_FLOAT_TO_FIX / AMS_TCS3430_GAIN_OF_GOLDEN)) / result;
				if(result > ams_tcs3430_setGainThreshold[chip->calibrationCtx.calState].high_thr||
					result < ams_tcs3430_setGainThreshold[chip->calibrationCtx.calState].low_thr){
					hwlog_err("\n %s: ratio is out bound[%d, %d]! calXresult[%d] = %d\n" , __func__,
						ams_tcs3430_setGainThreshold[chip->calibrationCtx.calState].high_thr,
						ams_tcs3430_setGainThreshold[chip->calibrationCtx.calState].low_thr, chip->calibrationCtx.calState, result);
					color_calibrate_result = false;
				}
			} else {
				/* cant devide by zero, maintain the default calibration scaling factor */
				result = AMS_TCS3430_FLOAT_TO_FIX;
			}
			chip->calibrationCtx.calXresult[chip->calibrationCtx.calState] = result;

			result = (chip->calibrationCtx.calYsample / AMS_TCS3430_CAL_AVERAGE);
			if (result){
				result = (chip->calibrationCtx.calYtarget * (currentGain *
					AMS_TCS3430_FLOAT_TO_FIX / AMS_TCS3430_GAIN_OF_GOLDEN)) / result;
				if(result > ams_tcs3430_setGainThreshold[chip->calibrationCtx.calState].high_thr
					|| result < ams_tcs3430_setGainThreshold[chip->calibrationCtx.calState].low_thr ){
					hwlog_err("\n %s: ratio is out bound[%d, %d]! calYresult[%d] = %d\n" , __func__,
						ams_tcs3430_setGainThreshold[chip->calibrationCtx.calState].high_thr,
						ams_tcs3430_setGainThreshold[chip->calibrationCtx.calState].low_thr, chip->calibrationCtx.calState, result);
					color_calibrate_result = false;
				}
			} else {
				/* cant devide by zero, maintain the default calibration scaling factor */
				result = AMS_TCS3430_FLOAT_TO_FIX;
			}
			chip->calibrationCtx.calYresult[chip->calibrationCtx.calState] = result;

			result = (chip->calibrationCtx.calZsample / AMS_TCS3430_CAL_AVERAGE);
			if (result){
				result = (chip->calibrationCtx.calZtarget * (currentGain *
					AMS_TCS3430_FLOAT_TO_FIX / AMS_TCS3430_GAIN_OF_GOLDEN)) / result;
				if(result > ams_tcs3430_setGainThreshold[chip->calibrationCtx.calState].high_thr
					|| result < ams_tcs3430_setGainThreshold[chip->calibrationCtx.calState].low_thr ){

					hwlog_err("\n %s: ratio is out bound[%d, %d]! calZresult[%d] = %d\n" , __func__,
						ams_tcs3430_setGainThreshold[chip->calibrationCtx.calState].high_thr,
						ams_tcs3430_setGainThreshold[chip->calibrationCtx.calState].low_thr, chip->calibrationCtx.calState, result);
					color_calibrate_result = false;
				}
			} else {
				/* cant devide by zero, maintain the default calibration scaling factor */
				result = AMS_TCS3430_FLOAT_TO_FIX;
			}
			chip->calibrationCtx.calZresult[chip->calibrationCtx.calState] = result;

			result = (chip->calibrationCtx.calIRsample / AMS_TCS3430_CAL_AVERAGE);
			if (result){
				result = (chip->calibrationCtx.calIRtarget * (currentGain *
					AMS_TCS3430_FLOAT_TO_FIX / AMS_TCS3430_GAIN_OF_GOLDEN)) / result;
				if(result > ams_tcs3430_setGainThreshold[chip->calibrationCtx.calState].high_thr||
					result < ams_tcs3430_setGainThreshold[chip->calibrationCtx.calState].low_thr ){

					hwlog_err("\n %s: ratio is out bound[%d, %d]! calIRresult[%d] = %d\n" , __func__,	
						ams_tcs3430_setGainThreshold[chip->calibrationCtx.calState].high_thr,
						ams_tcs3430_setGainThreshold[chip->calibrationCtx.calState].low_thr, chip->calibrationCtx.calState, result);
					color_calibrate_result = false;
				}
			} else {
				/* cant devide by zero, maintain the default calibration scaling factor */
				hwlog_err("\n %s: calIRresult[%d] = 0!!\n" , __func__, chip->calibrationCtx.calState);
				result = AMS_TCS3430_FLOAT_TO_FIX;
			}
			chip->calibrationCtx.calIRresult[chip->calibrationCtx.calState] = result;

			dev_info(&chip->client->dev, "osal_calHandl_als: calXresult  %d\n", chip->calibrationCtx.calXresult[chip->calibrationCtx.calState]);
			dev_info(&chip->client->dev, "osal_calHandl_als: calYresult  %d\n", chip->calibrationCtx.calYresult[chip->calibrationCtx.calState]);
			dev_info(&chip->client->dev, "osal_calHandl_als: calZresult  %d\n", chip->calibrationCtx.calZresult[chip->calibrationCtx.calState]);
			dev_info(&chip->client->dev, "osal_calHandl_als: calIRresult %d\n", chip->calibrationCtx.calIRresult[chip->calibrationCtx.calState]);

			chip->calibrationCtx.calState++;
			chip->calibrationCtx.calSampleCounter = 0;
			chip->calibrationCtx.calXsample = 0;
			chip->calibrationCtx.calYsample = 0;
			chip->calibrationCtx.calZsample = 0;
			chip->calibrationCtx.calIRsample = 0;
			if(chip->calibrationCtx.calState < CAL_STATE_GAIN_LAST){
				osal_als_enable_set(chip, AMSDRIVER_ALS_DISABLE);
				ctx->algCtx.als_data.gain = ams_tcs3430_alsGain_conversion[chip->calibrationCtx.calState];
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
		ams_tcs3430_setGain(chip->deviceCtx, AMS_TCS3430_AGAIN_DEFAULT);
		if(1 == enable_status_before_calibrate){
			osal_als_enable_set(chip, AMSDRIVER_ALS_ENABLE);
		}else{
			dev_info(&chip->client->dev, "color sensor disabled before calibrate\n");
		}
		dev_info(&chip->client->dev, "osal_calHandl_als: done\n");
	}
	return;
}

static void  osal_report_als(struct colorDriver_chip *chip)
{
	export_alsData_t outData = {0};
	uint8_t currentGainIndex = 0;
	uint32_t currentGain = 0;
	ams_tcs3430_deviceCtx_t * ctx = NULL;

	if (chip == NULL){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return;
	}
	ctx = chip->deviceCtx;

	ams_tcs3430_deviceGetAls(chip->deviceCtx, &outData);
	if(NULL != ctx){
		currentGain = ctx->algCtx.als_data.gain;
	}
	if (currentGain < ams_tcs3430_alsGain_conversion[CAL_STATE_GAIN_5]){
		currentGainIndex = ams_tcs3430_gainToReg(currentGain);
	} else {
		currentGainIndex = CAL_STATE_GAIN_5;
	}

	/* adjust the report data when the calibrate ratio is acceptable */

	outData.rawIR *= color_nv_para.calIratio[currentGainIndex];
	outData.rawIR /= AMS_TCS3430_FLOAT_TO_FIX;
	outData.rawX *= color_nv_para.calXratio[currentGainIndex];
	outData.rawX /= AMS_TCS3430_FLOAT_TO_FIX;
	outData.rawY *= color_nv_para.calYratio[currentGainIndex];
	outData.rawY /= AMS_TCS3430_FLOAT_TO_FIX;
	outData.rawZ *= color_nv_para.calZratio[currentGainIndex];
	outData.rawZ /= AMS_TCS3430_FLOAT_TO_FIX;


	report_value[0] = (int)outData.rawX;
	report_value[1] = (int)outData.rawY;
	report_value[2] = (int)outData.rawZ;
	report_value[3] = (int)outData.rawIR;

	report_value[0] *= AMS_TCS3430_GAIN_OF_GOLDEN;
	report_value[0] /= ams_tcs3430_als_gains[currentGainIndex];
	report_value[1] *= AMS_TCS3430_GAIN_OF_GOLDEN;
	report_value[1] /= ams_tcs3430_als_gains[currentGainIndex];
	report_value[2] *= AMS_TCS3430_GAIN_OF_GOLDEN;
	report_value[2] /= ams_tcs3430_als_gains[currentGainIndex];
	report_value[3] *= AMS_TCS3430_GAIN_OF_GOLDEN;
	report_value[3] /= ams_tcs3430_als_gains[currentGainIndex];

	ams_tcs3430_report_data(report_value);
	report_logcount++;
	if(report_logcount >= AMS_REPORT_LOG_COUNT_NUM){
		AMS_PORT_log_4("COLOR SENSOR tcs3430 report data %d, %d, %d, %d\n",
			report_value[0], report_value[1], report_value[2], report_value[3]);
		hwlog_info("\nAMS_Driver: tune Gain[1] and Gain[4] cal_para to Gain[16]nv: currentGain[%d]: [%d, %d, %d, %d]\n",
			ams_tcs3430_als_gains[currentGainIndex],
			color_nv_para.calXratio[currentGainIndex],
			color_nv_para.calYratio[currentGainIndex],
			color_nv_para.calZratio[currentGainIndex],
			color_nv_para.calIratio[currentGainIndex]);
		report_logcount = 0;
	}
}

#if defined(CONFIG_AMS_OPTICAL_SENSOR_ALS_XYZ)
static ssize_t osal_als_x_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	export_alsData_t outData;
	if(NULL == dev || NULL == attr || NULL == buf){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return 0;
		}
	struct colorDriver_chip *chip = dev_get_drvdata(dev);
	ams_tcs3430_deviceGetAls(chip->deviceCtx, &outData);
	return snprintf(buf, PAGE_SIZE, "%d\n", outData.rawX);
}

static ssize_t osal_als_y_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	export_alsData_t outData;
	if(NULL == dev){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return 0;
		}
	struct colorDriver_chip *chip = dev_get_drvdata(dev);
	ams_tcs3430_deviceGetAls(chip->deviceCtx, &outData);
	return snprintf(buf, PAGE_SIZE, "%d\n", outData.rawY);
}
static ssize_t osal_als_z_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	export_alsData_t outData;
	if(NULL == dev){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return 0;
		}
	struct colorDriver_chip *chip = dev_get_drvdata(dev);
	ams_tcs3430_deviceGetAls(chip->deviceCtx, &outData);
	return snprintf(buf, PAGE_SIZE, "%d\n", outData.rawZ);
}

static ssize_t osal_als_ir1_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	export_alsData_t outData;
	if(NULL == dev){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return 0;
		}
	struct colorDriver_chip *chip = dev_get_drvdata(dev);
	ams_tcs3430_deviceGetAls(chip->deviceCtx, &outData);
	return snprintf(buf, PAGE_SIZE, "%d\n", outData.rawIR);
}

static ssize_t osal_als_ir2_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	export_alsData_t outData;
	if(NULL == dev){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return 0;
		}
	struct colorDriver_chip *chip = dev_get_drvdata(dev);
	ams_tcs3430_deviceGetAls(chip->deviceCtx, &outData);
	return snprintf(buf, PAGE_SIZE, "%d\n", outData.rawIR2);
}
 #endif /* End of  XYZ */

int ams_tcs3430_setenable(bool enable)
{
	struct colorDriver_chip *chip = p_chip;

	if (enable)
		osal_als_enable_set(chip, AMSDRIVER_ALS_ENABLE);
	else
		osal_als_enable_set(chip, AMSDRIVER_ALS_DISABLE);

	hwlog_info("ams_tcs3430_setenable success\n");
	return 1;
}
EXPORT_SYMBOL_GPL(ams_tcs3430_setenable);

static ssize_t osal_als_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	if(NULL == dev){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return 0;
		}
	struct colorDriver_chip *chip = dev_get_drvdata(dev);
	ams_tcs3430_ams_mode_t mode = AMS_TCS3430_MODE_OFF;

	ams_tcs3430_deviceGetMode(chip->deviceCtx, &mode);

	if (mode & AMS_TCS3430_MODE_ALS){
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
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
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


void ams_show_calibrate(struct colorDriver_chip *chip, color_sensor_output_para * out_para)
{
	int i;
	if (NULL == out_para || NULL == chip){
		hwlog_err("ams_store_calibrate input para NULL \n");
		return;
	}

	if (chip->inCalMode == false){
		hwlog_err("ams_show_calibrate not in calibration mode \n");
	}

	//if (chip->calibrationCtx.calState == CAL_STATE_GAIN_LAST){
		out_para->result = (UINT32)report_calibrate_result;
		hwlog_info(" color_calibrate_show result = %d\n", out_para->result);
		memcpy(out_para->report_ir, chip->calibrationCtx.calIRresult, sizeof(out_para->report_ir));
		memcpy(out_para->report_x,  chip->calibrationCtx.calXresult,  sizeof(out_para->report_x));
		memcpy(out_para->report_y,  chip->calibrationCtx.calYresult,  sizeof(out_para->report_y));
		memcpy(out_para->report_z,  chip->calibrationCtx.calZresult,  sizeof(out_para->report_z));
	//}
	for(i = 0;i<CAL_STATE_GAIN_LAST; i++)
	{
		hwlog_info(" color_calibrate_show i = %d: %d,%d,%d,%d.\n", i,
		out_para->report_x[i],out_para->report_y[i],out_para->report_z[i],out_para->report_ir[i]);
		hwlog_info(" calibrationCtx i = %d: %d,%d,%d,%d.\n", i,
		chip->calibrationCtx.calXresult[i],chip->calibrationCtx.calYresult[i],
		chip->calibrationCtx.calZresult[i],chip->calibrationCtx.calIRresult[i]);

	}
	return;
}
void ams_store_calibrate(struct colorDriver_chip *chip, color_sensor_input_para *in_para)
{
	ams_tcs3430_deviceCtx_t * ctx = NULL;
	ams_tcs3430_ams_mode_t mode = 0;

	if((NULL == chip) || (NULL == in_para)){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return;
	}

	if (in_para->enable &&  chip->inCalMode){
		hwlog_err("ams_store_calibrate: Already in calibration mode.\n");
		return;
	}

	if (in_para->enable){
		ctx = chip->deviceCtx;
		dev_info(&chip->client->dev, "ams_store_calibrate: starting calibration mode\n");
		chip->calibrationCtx.calSampleCounter = 0;
		chip->calibrationCtx.calXsample = 0;
		chip->calibrationCtx.calYsample = 0;
		chip->calibrationCtx.calZsample = 0;
		chip->calibrationCtx.calIRsample = 0;
		chip->calibrationCtx.calXtarget= in_para->tar_x; /* calculate targer for gain 1x (assuming its set at 64x) */
		chip->calibrationCtx.calYtarget= in_para->tar_y;
		chip->calibrationCtx.calZtarget= in_para->tar_z;
		chip->calibrationCtx.calIRtarget= in_para->tar_ir;
		chip->calibrationCtx.calState = CAL_STATE_GAIN_1;
		if(NULL != ctx){
			ctx->algCtx.als_data.gain = ams_tcs3430_alsGain_conversion[CAL_STATE_GAIN_1];
		}
		ams_tcs3430_deviceGetMode(chip->deviceCtx, &mode);
		if ((mode & AMS_TCS3430_MODE_ALS) == AMS_TCS3430_MODE_ALS){
			enable_status_before_calibrate = 1;//enabled before calibrate
			hwlog_info("\nAMS_Driver: %s: enabled before calibrate\n", __func__);
			osal_als_enable_set(chip, AMSDRIVER_ALS_DISABLE);
			msleep(10);//sleep 10 ms to make sure disable timer
		}else{
			enable_status_before_calibrate = 0;//disabled before calibrate
			hwlog_info("\nAMS_Driver: %s: disabled before calibrate\n", __func__);
		}
		chip->inCalMode = true;
		color_calibrate_result = true;//make the calibrate_result true for calibrate again!!
		osal_als_enable_set(chip, AMSDRIVER_ALS_ENABLE);
	} else {
		dev_info(&chip->client->dev, "ams_store_calibrate: stopping calibration mode\n");
		chip->inCalMode = false;
		}
	return;
}
void ams_show_enable(struct colorDriver_chip *chip, int *state)
{
	ams_tcs3430_ams_mode_t mode = 0;

	if((NULL == chip) || (NULL == state)){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return;
		}
	
	ams_tcs3430_deviceGetMode(chip->deviceCtx, &mode);
	if (mode & AMS_TCS3430_MODE_ALS){
		*state = 1;
	} else {
		*state = 0;
	}
	
}

void ams_store_enable(struct colorDriver_chip *chip, int state)
{
	ams_tcs3430_ams_mode_t mode = 0;

	if(NULL == chip){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
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
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return rc;
	}
	return rc;
}

static int amsdriver_pltf_power_off(struct colorDriver_chip *chip)
{
	int rc = 0;
	if(NULL == chip){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return rc;
	}
	return rc;
}

static int amsdriver_power_on(struct colorDriver_chip *chip)
{
	int rc = 0;
	if(NULL == chip){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return rc;
	}
	rc = amsdriver_pltf_power_on(chip);
	if (rc){
		return rc;
	}
	dev_info(&chip->client->dev, "%s: chip was off, restoring regs\n",
			__func__);
	return ams_tcs3430_deviceInit((ams_tcs3430_deviceCtx_t*)chip->deviceCtx, chip->client);
}

#ifdef CONFIG_AMS_OPTICAL_SENSOR_ALS
static int osal_als_idev_open(struct input_dev *idev)
{
	if(NULL == idev){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return 0;
	}
	struct colorDriver_chip *chip = dev_get_drvdata(&idev->dev);
	int rc = 0;

	dev_info(&idev->dev, "%s\n", __func__);
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
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return;
	}
	int rc = 0;
	struct colorDriver_chip *chip = dev_get_drvdata(&idev->dev);
	dev_info(&idev->dev, "%s\n", __func__);

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
	ams_tcs3430_ams_mode_t mode = 0;

	if(NULL == work){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return;
	}
	struct colorDriver_chip *chip = \
		container_of(work, struct colorDriver_chip, als_work);
	if(NULL == chip){
		hwlog_err("\nAMS_Driver: %s: Pointer chip is NULL\n", __func__);
		return;
	}
	AMS_MUTEX_LOCK(&chip->lock);
	if(0 == read_nv_first_in){
		ret = get_cal_para_from_nv();
		if(!ret){
			hwlog_err("\ams_tcs3430: get_cal_para_from_nv fail \n");
		}
		read_nv_first_in = -1;// -1: do not read again.
	}
	dev_info(&chip->client->dev, "amsdriver_work\n");
	re_enable = ams_tcs3430_deviceEventHandler((ams_tcs3430_deviceCtx_t*)chip->deviceCtx, chip->inCalMode);

	if (re_enable)
	{
		ams_tcs3430_deviceCtx_t* ctx = (ams_tcs3430_deviceCtx_t*)chip->deviceCtx;
		ams_tcs3430_setField(ctx->portHndl, AMS_TCS3430_DEVREG_ENABLE, 0, AEN);
		ams_tcs3430_setField(ctx->portHndl, AMS_TCS3430_DEVREG_ENABLE, AEN, AEN);
	}

#ifdef CONFIG_AMS_OPTICAL_SENSOR_ALS

	if (ams_tcs3430_deviceGetResult((ams_tcs3430_deviceCtx_t*)chip->deviceCtx) & AMS_TCS3430_FEATURE_ALS){
		if (chip->inCalMode == false && !re_enable){
			dev_info(&chip->client->dev, "amsdriver_work: osal_report_als\n");
			osal_report_als(chip);
		} else {
			dev_info(&chip->client->dev, "amsdriver_work: calibration mode\n");
			osal_calHandl_als(chip);
		}
	}
#endif
	ams_tcs3430_deviceGetMode(chip->deviceCtx, &mode);
	if (((mode & AMS_TCS3430_MODE_ALS) == AMS_TCS3430_MODE_ALS)){
		if(chip->inCalMode == false){
			if (re_enable)
			{
				mod_timer(&chip->work_timer, jiffies + msecs_to_jiffies(106));// timer set as 106ms
				hwlog_info("timer set as 106ms \n");
			}
			else
			{
				mod_timer(&chip->work_timer, jiffies + HZ/10);// timer set as 100ms
				hwlog_info("timer set as 100ms \n");
			}
		}else{
			mod_timer(&chip->work_timer, jiffies + msecs_to_jiffies(120));//calibrate mode set timer for 120ms
			hwlog_info("in calibrate mode mod timer set as 120ms \n");
		}
	}else{
		AMS_PORT_log_1("amsdriver_work: not mod timer mode = %d\n",mode);
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
			hwlog_err("\nAMS_Driver: %s: DMD ap_color_sensor_detected fail\n", __func__);
		}
	}
}
#endif

int amsdriver_probe(struct i2c_client *client,
	const struct i2c_device_id *idp)
{
	int ret = 0;
	int i = 0;
	if(NULL == client){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return -1;
	}
	struct device *dev = &client->dev;
	static struct colorDriver_chip *chip = NULL;
	struct driver_i2c_platform_data *pdata = dev->platform_data;
	ams_tcs3430_deviceInfo_t amsDeviceInfo;
	ams_tcs3430_deviceIdentifier_e deviceId;

	/****************************************/
	/* Validate bus and device registration */
	/****************************************/
	hwlog_info(KERN_ERR "\nams_tcs3430: amsdriver_probe()\n");

	dev_info(dev, "%s: client->irq = %d\n", __func__, client->irq);
	if (!i2c_check_functionality(client->adapter,
			I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(dev, "%s: i2c smbus byte data unsupported\n", __func__);
		ret = -EOPNOTSUPP;
		goto init_failed;
	}

	chip = kzalloc(sizeof(struct colorDriver_chip), GFP_KERNEL);
	if (!chip) {
		ret = -ENOMEM;
		goto malloc_failed;
	}

	mutex_init(&chip->lock);
	chip->client = client;
	chip->pdata = pdata;
	i2c_set_clientdata(chip->client, chip);

	chip->in_suspend = 0;
	chip->inCalMode = false;
	chip->calibrationCtx.calState = 0;

	for (i = 0; i < CAL_STATE_GAIN_LAST; i++){
		chip->calibrationCtx.calXresult[i]  = AMS_TCS3430_FLOAT_TO_FIX ;
		chip->calibrationCtx.calYresult[i]  = AMS_TCS3430_FLOAT_TO_FIX ;
		chip->calibrationCtx.calZresult[i]  = AMS_TCS3430_FLOAT_TO_FIX ;
		chip->calibrationCtx.calIRresult[i] = AMS_TCS3430_FLOAT_TO_FIX ;
		color_nv_para.calXratio[i] = AMS_TCS3430_FLOAT_TO_FIX ;
		color_nv_para.calYratio[i]  = AMS_TCS3430_FLOAT_TO_FIX ;
		color_nv_para.calZratio[i]  = AMS_TCS3430_FLOAT_TO_FIX ;
		color_nv_para.calIratio[i] = AMS_TCS3430_FLOAT_TO_FIX ;
	}

#ifdef CONFIG_HUAWEI_DSM
	INIT_DELAYED_WORK(&ams_dmd_work, amsdriver_dmd_work);
#endif
	/********************************************************************/
	/* Validate the appropriate ams device is available for this driver */
	/********************************************************************/
    deviceId = ams_tcs3430_testForDevice(chip->client);
	dev_info(dev, "deviceId: %d\n", deviceId);
	hwlog_err("\nams_tcs3430: ams_tcs3430_testForDevice() %d \n", deviceId);

	if (deviceId == AMS_UNKNOWN_DEVICE) {
		dev_info(dev, "ams_tcs3430_testForDevice failed: AMS_UNKNOWN_DEVICE\n");
#ifdef CONFIG_HUAWEI_DSM
		color_devcheck_dmd_result = false;
		schedule_delayed_work(&ams_dmd_work, msecs_to_jiffies(AP_COLOR_DMD_DELAY_TIME_MS));
#endif
		goto id_failed;
	}

	dev_info(dev, "ams_tcs3430_testForDevice() ok\n");
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
	set_hw_dev_flag(DEV_I2C_AP_COLOR_SENSOR);
#endif
	ams_tcs3430_getDeviceInfo(&amsDeviceInfo);
	dev_info(dev, "ams_amsDeviceInfo() ok\n");

	chip->deviceCtx = kzalloc(amsDeviceInfo.memorySize, GFP_KERNEL);
	if (chip->deviceCtx) {
		ret = ams_tcs3430_deviceInit(chip->deviceCtx, chip->client);
		if (ret == false){
			dev_info(dev, "ams_amsDeviceInit() ok\n");
		} else {
			dev_info(dev, "ams_deviceInit failed.\n");
			goto id_failed;
		}
	} else {
		dev_info(dev, "ams_tcs3430 kzalloc failed.\n");
		goto id_failed;
	}

	/*********************/
	/* Initialize ALS    */
	/*********************/

#ifdef CONFIG_AMS_OPTICAL_SENSOR_ALS
	/* setup */
	dev_info(dev, "Setup for ALS\n");
#endif

    init_timer(&chip->work_timer);
	setup_timer(&chip->work_timer, osal_als_timerHndl, (unsigned long) chip);
    INIT_WORK(&chip->als_work, amsdriver_work);

	chip->color_show_calibrate_state = ams_show_calibrate;
	chip->color_store_calibrate_state = ams_store_calibrate;
	chip->color_enable_show_state = ams_show_enable;
	chip->color_enable_store_state = ams_store_enable;
	chip->color_sensor_getGain = ams_tcs3430_getGain;
	chip->color_sensor_setGain = ams_tcs3430_setGain;

	//dev_set_drvdata(chip->dev, chip);//add by zpl
	p_chip = chip;

	ret = color_register(chip);
	if(ret < 0){
		hwlog_err("\ams_tcs3430: color_register fail \n");
	}
	color_default_enable = ams_tcs3430_setenable;

	dev_info(dev, "Probe ok.\n");
	return 0;

	/********************************************************************************/
	/* Exit points for device functional failures (RemCon, Prox, ALS, Gesture)      */
	/* This must be unwound in the correct order, reverse from initialization above */
	/********************************************************************************/
#ifdef CONFIG_AMS_OPTICAL_SENSOR_ALS

input_a_alloc_failed:
#endif

	/********************************************************************************/
	/* Exit points for general device initialization failures                       */
	/********************************************************************************/

id_failed:
	if (chip->deviceCtx) kfree(chip->deviceCtx);
	i2c_set_clientdata(client, NULL);
malloc_failed:
	kfree(chip);

init_failed:
	dev_err(dev, "Probe failed.\n");
	return ret;
}

int amsdriver_suspend(struct device *dev)
{
	if(NULL == dev){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return -1;
	}
	struct colorDriver_chip  *chip = dev_get_drvdata(dev);
	if(NULL == chip){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return -1;
	}

	dev_info(dev, "%s\n", __func__);
	AMS_MUTEX_LOCK(&chip->lock);
	chip->in_suspend = 1;

	if (chip->wake_irq) {
		irq_set_irq_wake(chip->client->irq, 1);
	} else if (!chip->unpowered) {
		dev_info(dev, "powering off\n");
		/* TODO
		   platform power off */
	}
	AMS_MUTEX_UNLOCK(&chip->lock);

	return 0;
}

int amsdriver_resume(struct device *dev)
{
	if(NULL == dev){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return -1;
	}
	struct colorDriver_chip *chip = dev_get_drvdata(dev);
	if(NULL == chip){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
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

int amsdriver_remove(struct i2c_client *client)
{
	if(NULL == client){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
		return -1;
	}
	struct colorDriver_chip *chip = i2c_get_clientdata(client);
	if(NULL == chip){
		hwlog_err("\nAMS_Driver: %s: Pointer is NULL\n", __func__);
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
	{"ams_tcs3430", 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, amsdriver_idtable);

static const struct dev_pm_ops amsdriver_pm_ops = {
	.suspend = amsdriver_suspend,
	.resume  = amsdriver_resume,
};

static const struct of_device_id amsdriver_of_id_table[] = {
	{.compatible = "ams,tcs3430"},
	{},
};


static struct i2c_driver amsdriver_driver = {
	.driver = {
		.name = "ams_tcs3430",
		.owner = THIS_MODULE,
		.of_match_table = amsdriver_of_id_table,
	},
	.id_table = amsdriver_idtable,
	.probe = amsdriver_probe,
	.remove = amsdriver_remove,
};

static int __init amsdriver_init(void)
{
	int rc;
	hwlog_info("\nams_tcs3430: init()\n");

	rc = i2c_add_driver(&amsdriver_driver);

	printk(KERN_ERR "ams_tcs3430:  %d", rc);
	return rc;
}

static void __exit amsdriver_exit(void)
{
	hwlog_info("\nams_tcs3430: exit()\n");
	i2c_del_driver(&amsdriver_driver);
}

module_init(amsdriver_init);
module_exit(amsdriver_exit);

MODULE_AUTHOR("AMS AOS Software<cs.americas@ams.com>");
MODULE_DESCRIPTION("AMS tcs3430 ALS, XYZ color sensor driver");
MODULE_LICENSE("GPL");
