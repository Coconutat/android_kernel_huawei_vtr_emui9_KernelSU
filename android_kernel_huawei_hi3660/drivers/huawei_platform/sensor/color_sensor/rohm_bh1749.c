/*
 * Input Driver Module
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

#include "rohm_bh1749.h"
//#include "color_sensor.h"

#define HWLOG_TAG color_sensor
HWLOG_REGIST();

/****************************************************************************
 *                     OSAL Device Com Interface
 ****************************************************************************/
static struct class *color_class;
static bool rohm_bh1749_getDeviceInfo(rohm_bh1749_deviceInfo_t * info);
static void rohm_bh1749_deviceInit(rohm_bh1749_deviceCtx_t * ctx, ROHM_PORT_portHndl * portHndl);
static bool rohm_bh1749_deviceEventHandler(rohm_bh1749_deviceCtx_t * ctx, bool inCalMode);
static UINT8 rohm_bh1749_testForDevice(ROHM_PORT_portHndl * portHndl);
static int report_value[ROHM_REPORT_DATA_LEN] = {0};
static struct colorDriver_chip *p_chip = NULL;
extern int (*color_default_enable)(bool enable);
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
static struct delayed_work rohm_dmd_work;
static uint8_t report_logcount = 0;

#if defined(CONFIG_ROHM_OPTICAL_SENSOR_ALS)
typedef struct{
	uint32_t rawX;
	uint32_t rawY;
	uint32_t rawZ;
	uint32_t rawIR;
	uint32_t rawIR2;
}export_alsData_t;

#define ROHM_BH1749_GAIN_OF_GOLDEN             ( 32)
#define ROHM_REPORT_LOG_COUNT_NUM                (20)
#define ROHM_BH1749_GAIN_SCALE	(1000)
#define LEFT_SHIFT_8  (8)
#define DATA_TRANSFER_COFF (32*120)
#define ROHM_POLL_TIME (150)

#define AND_FF (0xFF)
static UINT32 const rohm_bh1749_alsGain_conversion[] = {
	1 * ROHM_BH1749_GAIN_SCALE,
	32 * ROHM_BH1749_GAIN_SCALE
};

static UINT8 const rohm_bh1749_als_gains[] = {
	1,
	32
};

int ROHM_PORT_BH1749_getByte(ROHM_PORT_portHndl * handle, uint8_t reg, uint8_t * data, uint8_t len){
    int ret = ERR_PARA;
    if ((handle == NULL) || (data == NULL )){
	hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
	return ERR_PARA;
    }

    ret = i2c_smbus_read_i2c_block_data(handle, reg, len, data);
    if (ret < 0)
	    hwlog_err("%s: failed at address %x (%d bytes)\n",
		    __func__, reg, len);

    return ret;
}

int ROHM_PORT_BH1749_setByte(ROHM_PORT_portHndl * handle, uint8_t reg, uint8_t* data, uint8_t len){
	int ret = ERR_PARA;
	if ((handle == NULL) || (data == NULL) ){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return ERR_PARA;
	}

	ret = i2c_smbus_write_i2c_block_data(handle, reg, len, data);
	if (ret < 0)
		hwlog_err("%s: failed at address %x (%d bytes)\n",
			__func__, reg, len);
	return ret;
}

static int rohm_bh1749_getByte(ROHM_PORT_portHndl * portHndl, uint8_t reg, UINT8 * readData)
{
	int read_count = 0;
	UINT8 length = 1;

	if (portHndl == NULL || readData == NULL){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return read_count;
	}

	read_count = ROHM_PORT_BH1749_getByte(portHndl,
				reg,
				readData,
				length);

	hwlog_debug("I2C reg getByte = 0x%02x, data: 0x%02x\n", reg, (UINT8)readData[0]);

	return read_count;
}

static int rohm_bh1749_setByte(ROHM_PORT_portHndl * portHndl, uint8_t reg, UINT8 data)
{
	int write_count = 0;
	UINT8 length = 1;

	if (portHndl == NULL ){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return write_count;
	}

	write_count = ROHM_PORT_BH1749_setByte(portHndl,
				reg,
				&data,
				length);
	hwlog_info("I2C reg setByte = 0x%02x, data: 0x%02x\n", reg, (UINT8)data);
	return write_count;
}

static int rohm_bh1749_getBuf(ROHM_PORT_portHndl * portHndl, uint8_t reg, UINT8 * readData, UINT8 length)
{
	UINT8 read_count = 0;

	if ((portHndl == NULL) || (readData == NULL) ){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return read_count;
	}

	read_count = ROHM_PORT_BH1749_getByte(portHndl,
				reg,
				readData,
				length);

	//PRINT_INFO("I2C reg getBuf  = 0x%x, len= %d, data: 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", rohm_bh1749_deviceRegisterDefinition[reg].address, length,
	//	(UINT8)readData[0], (UINT8)readData[1], (UINT8)readData[2], (UINT8)readData[3], (UINT8)readData[4], (UINT8)readData[5], (UINT8)readData[6], (UINT8)readData[7] );
	return read_count;
}

static int rohm_bh1749_report_data(int value[])
{
	hwlog_debug("rohm_bh1749_report_data\n");
	return ap_color_report(value, ROHM_REPORT_DATA_LEN*sizeof(int));
}
static int rohm_bh1749_setWord(ROHM_PORT_portHndl * portHndl, uint8_t reg, UINT16 data)
{
	int write_count = 0;
	UINT8 length = sizeof(UINT16);
	UINT8 buffer[sizeof(UINT16)] = {0};

	if (portHndl == NULL){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return write_count;
	}

	buffer[0] = ((data >> ROHM_ENDIAN_1) & AND_FF);//0xff for ROHM_ENDIAN_1 need part
	buffer[1] = ((data >> ROHM_ENDIAN_2) & AND_FF);//0xff for ROHM_ENDIAN_2 need part

	write_count = ROHM_PORT_BH1749_setByte(portHndl,
				reg,
				&buffer[0],
				length);
	return write_count;
}

#if 0
static UINT8 rohm_bh1749_getField(ROHM_PORT_portHndl * portHndl, uint8_t reg, UINT8 * readData, rohm_bh1749_regMask_t mask)
{
	UINT8 read_count = 0;
	UINT8 length = 1;

	if ((portHndl == NULL) || (readData == NULL) ){
		hwlog_err("\nROHM_Driver: %s: Pointer is NULL\n", __func__);
		return read_count;
	}

	read_count = ROHM_PORT_BH1749_getByte(portHndl,
	                            reg,
	                            readData,
	                            length);

	*readData &= mask;

	return read_count;
}

static UINT8 rohm_bh1749_setField(ROHM_PORT_portHndl * portHndl, uint8_t reg, UINT8 data, rohm_bh1749_regMask_t mask)
{
	UINT8 write_count = 1;
	UINT8 length = 1;
	UINT8 original_data = 0;
	UINT8 new_data = 0;

	if (portHndl == NULL){
		hwlog_err("\nROHM_Driver: %s: Pointer is NULL\n", __func__);
		return 0;
	}

	rohm_bh1749_getByte(portHndl,
	                    reg,
	                    &original_data);

	new_data = original_data & ~mask;
	new_data |= (data & mask);

	if (new_data != original_data){
	    write_count = rohm_bh1749_setByte(portHndl,
	                    reg,
	                    new_data);
	}

	return write_count;
}
#endif

static INT32 rohm_bh1749_setGain(rohm_bh1749_deviceCtx_t * ctx, uint32_t gain){
	UINT8 cfg1_reg_data = 0;
	INT32 ret = 0;

	if (ctx == NULL){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return -1;
	}

	hwlog_info("rohm_bh1749_setGain: %d\n", gain);

	rohm_bh1749_getByte(ctx->portHndl, ROHM_BH1749_MODECONTROL1, &cfg1_reg_data);
	if(GAIN_X1 == gain)
	{
		cfg1_reg_data = (cfg1_reg_data & RGB_GAIN_MASK) | RGB_GAIN_X1;
		ctx->algCtx.als_data.gain =  GAIN_X1 *  ROHM_BH1749_GAIN_SCALE;
	}else if(GAIN_X32 == gain)
	{
		cfg1_reg_data = (cfg1_reg_data & RGB_GAIN_MASK) | RGB_GAIN_X32;
		ctx->algCtx.als_data.gain =  GAIN_X32 * ROHM_BH1749_GAIN_SCALE ;
	}
	else
	{
		hwlog_err("Invalid gain\n");
	}
	rohm_bh1749_setByte(ctx->portHndl, ROHM_BH1749_MODECONTROL1, cfg1_reg_data);

	return (ret);
}

static UINT8 rohm_bh1749_testForDevice(ROHM_PORT_portHndl * portHndl){
	UINT8 chipId = 0;

	if (portHndl == NULL){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return ROHM_UNKNOWN_DEVICE;
	}

	rohm_bh1749_getByte(portHndl, ROHM_BH1749_MANUFACT_ID, &chipId);
	return chipId;
}
#define ROHM_GAIN_SIZE 4
static INT32 rohm_bh1749_getGain(rohm_bh1749_deviceCtx_t * ctx){

	UINT8 cfg1_reg_data = 0;
	INT32 gain = 0;
        unsigned char again[ROHM_GAIN_SIZE] = {0, 1, 0, 32};//intalize gain original val

	if (ctx == NULL){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return -1;
	}

	rohm_bh1749_getByte(ctx->portHndl, ROHM_BH1749_MODECONTROL1, &cfg1_reg_data);
	gain = again[RGB_GAIN_VALUE(cfg1_reg_data)];

	return gain;
}



static INT32 rohm_bh1749_getIrGain(rohm_bh1749_deviceCtx_t * ctx){
	UINT8 cfg1_reg_data = 0;
	INT32 ir_gain = 0;
        unsigned char again[ROHM_GAIN_SIZE] = {0, 1, 0, 32};//intalize ir gain original val

	if (ctx == NULL){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return -1;
	}

	rohm_bh1749_getByte(ctx->portHndl, ROHM_BH1749_MODECONTROL1, &cfg1_reg_data);
	ir_gain = again[IR_GAIN_VALUE(cfg1_reg_data)];

	return ir_gain;
}

static UINT8 rohm_bh1749_gainToReg(UINT32 x){
	UINT8 i =0;

	for (i = sizeof(rohm_bh1749_alsGain_conversion)/sizeof(UINT32)-1; i != 0; i--) {
	    	if (x >= rohm_bh1749_alsGain_conversion[i]) break;
	}
	hwlog_info("rohm_bh1749_gainToReg: %d %d\n", x, i);
	return (i);
}

static INT32 rohm_bh1749_setIrGain(rohm_bh1749_deviceCtx_t * ctx, uint32_t gain){
	UINT8 cfg1_reg_data = 0;
	INT32 ret = 0;

	if (ctx == NULL){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return -1;
	}

	hwlog_info("rohm_bh1749_setIrGain: %d\n", gain);
	rohm_bh1749_getByte(ctx->portHndl, ROHM_BH1749_MODECONTROL1, &cfg1_reg_data);

	if(GAIN_X1 == gain)
	{
		cfg1_reg_data = (cfg1_reg_data & IR_GAIN_MASK) | IR_GAIN_X1;
		ctx->algCtx.als_data.gain_ir =  GAIN_X1 * ROHM_BH1749_GAIN_SCALE;
	}else if(GAIN_X32 == gain)
	{
		cfg1_reg_data = (cfg1_reg_data & IR_GAIN_MASK) | IR_GAIN_X32;
		ctx->algCtx.als_data.gain_ir =  GAIN_X32 * ROHM_BH1749_GAIN_SCALE;
	}
	else
	{
		hwlog_err("Invalid ir_gain\n");
	}

	rohm_bh1749_setByte(ctx->portHndl, ROHM_BH1749_MODECONTROL1, cfg1_reg_data);

	return (ret);
}


static INT32 rohm_bh1749_setAtimeInMs(rohm_bh1749_deviceCtx_t * ctx, int atime_ms){
	INT32 ret = 0;
	UINT8 atime_reg_data = 0;

	if (ctx == NULL){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return -1;
	}

	rohm_bh1749_getByte(ctx->portHndl, ROHM_BH1749_MODECONTROL1, &atime_reg_data);

	if(TIME_120MS == atime_ms)
	{
		atime_reg_data = (atime_reg_data & TIME_GAIN_MASK) | MEASURE_120MS;
		ctx->algCtx.als_data.atime_ms =  TIME_120MS;
	}else if(TIME_240MS == atime_ms)
	{
		atime_reg_data = (atime_reg_data & TIME_GAIN_MASK) | MEASURE_240MS;
		ctx->algCtx.als_data.atime_ms =  TIME_240MS;
	}
	else if(TIME_35MS == atime_ms)
	{
		atime_reg_data = (atime_reg_data & TIME_GAIN_MASK) | MEASURE_35MS;
		ctx->algCtx.als_data.atime_ms =  TIME_35MS;
	}
	else
	{
		hwlog_err("Invalid ir_gain\n");
	}

	ret = rohm_bh1749_setByte(ctx->portHndl, ROHM_BH1749_MODECONTROL1, atime_reg_data);

	return (ret);
}

static void rohm_bh1749_resetAllRegisters(rohm_bh1749_deviceCtx_t * ctx, ROHM_PORT_portHndl * portHndl){
	UINT8 tmp = 0;

	if (portHndl == NULL || ctx == NULL){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return;
	}

	hwlog_info("_bh1749_resetAllRegisters\n");

	//reset
	tmp = (UINT8)SW_RESET|INT_RESET;
	rohm_bh1749_setByte(portHndl, ROHM_BH1749_SYSTEMCONTROL, tmp);

	//set 32X default
	rohm_bh1749_setGain(ctx, GAIN_X32);
	rohm_bh1749_setIrGain(ctx, GAIN_X32);

	//set 35ms default
	rohm_bh1749_setAtimeInMs(ctx, TIME_120MS);
}

static bool  rohm_bh1749_saturation_check(rohm_bh1749_deviceCtx_t * ctx, raw_data_arg_s *current_raw)
{
	if(current_raw == NULL){
		hwlog_err("NULL pointer!\n");
		return false;
	}
	if (current_raw->red > ROHM_BH1749_HIGH_LEVEL ||
		current_raw->green > ROHM_BH1749_HIGH_LEVEL ||
		current_raw->blue > ROHM_BH1749_HIGH_LEVEL)
	{
		return true;
	}
	return false;
}

static bool rohm_bh1749_insufficience_check(rohm_bh1749_deviceCtx_t * ctx, raw_data_arg_s *current_raw)
{
	if(current_raw == NULL){
		hwlog_err("NULL pointer!\n");
		return false;
	}
	if (current_raw->red < ROHM_BH1749_LOW_LEVEL ||
		current_raw->green < ROHM_BH1749_LOW_LEVEL ||
		current_raw->blue < ROHM_BH1749_LOW_LEVEL)
	{
		return true;
	}
	return false;
}

static bool  rohm_bh1749_saturation_ir_check(rohm_bh1749_deviceCtx_t * ctx, raw_data_arg_s *current_raw)
{
	if(current_raw == NULL){
		hwlog_err("NULL pointer!\n");
		return false;
	}
	if (current_raw->ir > ROHM_BH1749_HIGH_LEVEL)
	{
		return true;
	}
	return false;
}

static bool rohm_bh1749_insufficience_ir_check(rohm_bh1749_deviceCtx_t * ctx, raw_data_arg_s *current_raw)
{
	if(current_raw == NULL){
		hwlog_err("NULL pointer!\n");
		return false;
	}
	if (current_raw->ir < ROHM_BH1749_LOW_LEVEL)
	{
		return true;
	}
	return false;
}

static bool rohm_bh1749_handleALS(rohm_bh1749_deviceCtx_t * ctx, bool inCalMode){
	UINT8 adc_data[ROHM_BH1749_ADC_BYTES] = {0};
        raw_data_arg_s rgb_rawdata = {0};
	uint8_t valid_data = 0;
	bool re_enable = false;

	if (ctx == NULL){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return false;
	}

	//read valid status
	rohm_bh1749_getByte(ctx->portHndl, ROHM_BH1749_MODECONTROL2, &valid_data);
	hwlog_debug("ROHM_Driver: %s: ROHM_BH1749_MODECONTROL2=0x%x \n", __func__, valid_data);
	if(valid_data & RGBC_VALID_HIGH)
	{
		/* read ADC data */
		rohm_bh1749_getBuf(ctx->portHndl, ROHM_BH1749_RED_DATA, adc_data, ROHM_BH1749_ADC_BYTES);
		rgb_rawdata.red      = ((unsigned short) adc_data[1])<<LEFT_SHIFT_8 | adc_data[0];
		rgb_rawdata.green = ((unsigned short) adc_data[3])<<LEFT_SHIFT_8 | adc_data[2];
		rgb_rawdata.blue   = ((unsigned short) adc_data[5])<<LEFT_SHIFT_8 | adc_data[4];

		rohm_bh1749_getBuf(ctx->portHndl, ROHM_BH1749_IR_DATA, adc_data, ROHM_BH1749_ADC_IR_BYTES);
		rgb_rawdata.ir = ((unsigned short) adc_data[1])<<LEFT_SHIFT_8 | adc_data[0];
		hwlog_debug("rohm_bh1749_handleALS: r=%d , g = %d, b = %d ir = %d\n", rgb_rawdata.red, rgb_rawdata.green,rgb_rawdata.blue,rgb_rawdata.ir);

		ctx->algCtx.als_data.datasetArray.x   = rgb_rawdata.red;
		ctx->algCtx.als_data.datasetArray.y   = rgb_rawdata.green;
		ctx->algCtx.als_data.datasetArray.z   = rgb_rawdata.blue;
		ctx->algCtx.als_data.datasetArray.ir1 = rgb_rawdata.ir;
		ctx->algCtx.als_data.datasetArray.ir2 = rgb_rawdata.ir;
	}
	else
	{
		hwlog_err("ROHM_Driver:  Invalid  status !!!! \n");
		return true;
	}

	if (inCalMode)
	{
		ctx->updateAvailable |= ROHM_BH1749_FEATURE_ALS;
		hwlog_warn("rohm_bh1749_handleALS: ATIME = %d, AGAIN = %d, inCalMode = %d\n", \
			(UINT16)ctx->algCtx.als_data.atime_ms, \
			(UINT32)ctx->algCtx.als_data.gain, \
			inCalMode);

		return false;
	}

	/* Adjust rgb gain setting */
	if (rohm_bh1749_saturation_check(ctx, &rgb_rawdata)) {
		if(ctx->algCtx.als_data.gain == (GAIN_X32 * ROHM_BH1749_GAIN_SCALE))
		{
			rohm_bh1749_setGain(ctx, GAIN_X1);//Set Gain to 1x
			re_enable = true;
		}

	}
	else if (rohm_bh1749_insufficience_check(ctx, &rgb_rawdata) )
	{
		if(ctx->algCtx.als_data.gain == (GAIN_X1 * ROHM_BH1749_GAIN_SCALE))
		{
			rohm_bh1749_setGain(ctx, GAIN_X32);//Set Gain to 32x
			re_enable = true;
		}
	}

	/* Adjust ir gain setting */
	if (rohm_bh1749_saturation_ir_check(ctx, &rgb_rawdata)) {

		if(ctx->algCtx.als_data.gain_ir == (GAIN_X32 * ROHM_BH1749_GAIN_SCALE))
		{
			rohm_bh1749_setIrGain(ctx, GAIN_X1 );//Set ir Gain to 1x
			re_enable = true;
		}
	}
	else if (rohm_bh1749_insufficience_ir_check(ctx, &rgb_rawdata) )
	{
		if(ctx->algCtx.als_data.gain_ir == (GAIN_X1 * ROHM_BH1749_GAIN_SCALE))
		{
			rohm_bh1749_setIrGain(ctx, GAIN_X32 );//Set ir Gain to 32x
			re_enable = true;
		}
	}

	//set available every time, because timer is more than 50ms, our mesaure time is 35ms
	ctx->updateAvailable |= ROHM_BH1749_FEATURE_ALS;

	hwlog_debug("rohm_bh1749_handleALS: ATIME = %d, RGB_GAIN = %d, IR_GAIN = %d, inCalMode = %d\n", \
		(UINT16)ctx->algCtx.als_data.atime_ms, \
		(UINT32)ctx->algCtx.als_data.gain, \
		(UINT32)ctx->algCtx.als_data.gain_ir, \
		inCalMode);

	return re_enable;
}


/* --------------------------------------------------------------------*/
/* Called by the OSAL interrupt service routine                        */
/* --------------------------------------------------------------------*/
static bool rohm_bh1749_deviceEventHandler(rohm_bh1749_deviceCtx_t * ctx, bool inCalMode)
{
	bool ret = false;

	if (ctx == NULL){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return false;
	}

	ret = rohm_bh1749_handleALS(ctx, inCalMode);
	return ret;
}

static bool rohm_bh1749_deviceGetMode(rohm_bh1749_deviceCtx_t * ctx, rohm_bh1749_rohm_mode_t *mode)
{
	if ((ctx == NULL) || (mode == NULL)){
		hwlog_err("%s: Pointer is NULL\n", __func__);
		return true;
	}

	*mode = ctx->mode;
	return false;
}

static bool rohm_bh1749_deviceSetMode(rohm_bh1749_deviceCtx_t * ctx, rohm_bh1749_rohm_mode_t mode)
{
	if (ctx == NULL){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return true;
	}

	ctx->mode = mode;
	return false;
}

static uint32_t rohm_bh1749_deviceGetResult(rohm_bh1749_deviceCtx_t * ctx)
{
	if (ctx == NULL){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return -1;
	}
	return ctx->updateAvailable;
}

static bool rohm_bh1749_deviceGetAls(rohm_bh1749_deviceCtx_t * ctx, export_alsData_t * exportData)
{
	if ((ctx == NULL) || (exportData == NULL)){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return true;
	}
	ctx->updateAvailable &= ~(ROHM_BH1749_FEATURE_ALS);

	exportData->rawX = ctx->algCtx.als_data.datasetArray.x;
	exportData->rawY = ctx->algCtx.als_data.datasetArray.y;
	exportData->rawZ = ctx->algCtx.als_data.datasetArray.z;
	exportData->rawIR = ctx->algCtx.als_data.datasetArray.ir1;
	exportData->rawIR2 = ctx->algCtx.als_data.datasetArray.ir1;

	return false;
}


static void rohm_bh1749_deviceInit(rohm_bh1749_deviceCtx_t * ctx, ROHM_PORT_portHndl * portHndl)
{
	if (portHndl == NULL || ctx == NULL){
		hwlog_err("\nROHM_Driver: %s: Pointer is NULL\n", __func__);
		return ;
	}

	memset(ctx, 0, sizeof(rohm_bh1749_deviceCtx_t));
	ctx->portHndl = portHndl;
	ctx->mode = ROHM_BH1749_MODE_OFF;
	ctx->deviceId = rohm_bh1749_testForDevice(ctx->portHndl);
	rohm_bh1749_resetAllRegisters(ctx, portHndl);
}

static bool rohm_bh1749_getDeviceInfo(rohm_bh1749_deviceInfo_t * info)
{
	if (info == NULL){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return true;
	}
	memset(info, 0, sizeof(rohm_bh1749_deviceInfo_t));
	info->memorySize =  sizeof(rohm_bh1749_deviceCtx_t);
	return false;
}

static void osal_als_timerHndl(unsigned long data)
{
	struct colorDriver_chip *chip = (struct colorDriver_chip*) data;

	if (chip == NULL){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return;
	}
	schedule_work(&chip->als_work);
}

static ssize_t osal_als_enable_set(struct colorDriver_chip *chip, uint8_t valueToSet)
{
	ssize_t rc = 0;
	rohm_bh1749_rohm_mode_t mode = 0;

	if (chip == NULL){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return true;
	}

	rohm_bh1749_deviceGetMode(chip->deviceCtx, &mode);
	if(valueToSet){
		rohm_bh1749_setByte(chip->client, ROHM_BH1749_MODECONTROL2, RGBC_EN_ON);
		mod_timer(&chip->work_timer, jiffies + msecs_to_jiffies(ROHM_POLL_TIME));//first enable set the timer as 150ms
		rohm_bh1749_deviceSetMode(chip->deviceCtx, ROHM_BH1749_MODE_ALS);
        }
	else
	{
		rohm_bh1749_setByte(chip->client, ROHM_BH1749_MODECONTROL2, RGBC_EN_OFF);
		rohm_bh1749_deviceSetMode(chip->deviceCtx, ROHM_BH1749_MODE_OFF);
	}

        hwlog_info("osal_als_enable_set = %d\n",valueToSet);
	return 0;
}

static int get_cal_para_from_nv(void)
{
	int i = 0, ret = 0;

	ret = read_color_calibrate_data_from_nv(RGBAP_CALI_DATA_NV_NUM, RGBAP_CALI_DATA_SIZE, "RGBAP", &color_nv_para);
	if(ret < 0){
		hwlog_err("ROHM_Driver: %s: fail,use default para!!\n", __func__);
		for (i = 0; i < CAL_STATE_GAIN_LAST; i++){
			hwlog_err("ROHM_Driver: get_cal_para_from_nv: gain[%d]: [%d, %d, %d, %d]\n", i,
			color_nv_para.calXratio[i], color_nv_para.calYratio[i], color_nv_para.calZratio[i], color_nv_para.calIratio[i]);
		}
		return 0;
	}
	for (i = 0; i < CAL_STATE_GAIN_LAST; i++){
		hwlog_warn("ROHM_Driver: get_cal_para_from_nv: gain[%d]: [%d, %d, %d, %d]\n", i,
		color_nv_para.calXratio[i], color_nv_para.calYratio[i], color_nv_para.calZratio[i], color_nv_para.calIratio[i]);

		if(!color_nv_para.calXratio[i]||!color_nv_para.calYratio[i]
			||!color_nv_para.calZratio[i]||!color_nv_para.calIratio[i]){
			color_nv_para.calXratio[i] = ROHM_BH1749_FLOAT_TO_FIX ;
			color_nv_para.calYratio[i]  = ROHM_BH1749_FLOAT_TO_FIX ;
			color_nv_para.calZratio[i]  = ROHM_BH1749_FLOAT_TO_FIX ;
			color_nv_para.calIratio[i] = ROHM_BH1749_FLOAT_TO_FIX ;
		}/* old project is 100, current is 10000  */
		else if (color_nv_para.calXratio[i]>=FLOAT_TO_FIX_LOW/ROHM_BH1749_CAL_THR
				&&color_nv_para.calXratio[i]<=FLOAT_TO_FIX_LOW*ROHM_BH1749_CAL_THR
				&&color_nv_para.calYratio[i]>=FLOAT_TO_FIX_LOW/ROHM_BH1749_CAL_THR
				&&color_nv_para.calYratio[i]<=FLOAT_TO_FIX_LOW*ROHM_BH1749_CAL_THR
				&&color_nv_para.calZratio[i]>=FLOAT_TO_FIX_LOW/ROHM_BH1749_CAL_THR
				&&color_nv_para.calZratio[i]<=FLOAT_TO_FIX_LOW*ROHM_BH1749_CAL_THR
				&&color_nv_para.calIratio[i]>=FLOAT_TO_FIX_LOW/ROHM_BH1749_CAL_THR
				&&color_nv_para.calIratio[i]<=FLOAT_TO_FIX_LOW*ROHM_BH1749_CAL_THR){
			color_nv_para.calXratio[i] *= (ROHM_BH1749_FLOAT_TO_FIX/FLOAT_TO_FIX_LOW);
			color_nv_para.calYratio[i] *= (ROHM_BH1749_FLOAT_TO_FIX/FLOAT_TO_FIX_LOW);
			color_nv_para.calZratio[i] *= (ROHM_BH1749_FLOAT_TO_FIX/FLOAT_TO_FIX_LOW);
			color_nv_para.calIratio[i] *= (ROHM_BH1749_FLOAT_TO_FIX/FLOAT_TO_FIX_LOW);
			hwlog_info("ROHM_Driver: low_level nv calibrate data\n");
		}
	}

	//copy calibration coefficient 32X to 1X as ROHM suggestion
	color_nv_para.calXratio[CAL_STATE_GAIN_1] = color_nv_para.calXratio[CAL_STATE_GAIN_2] ;
	color_nv_para.calYratio[CAL_STATE_GAIN_1] = color_nv_para.calYratio[CAL_STATE_GAIN_2] ;
	color_nv_para.calZratio[CAL_STATE_GAIN_1] = color_nv_para.calZratio[CAL_STATE_GAIN_2] ;
	color_nv_para.calIratio[CAL_STATE_GAIN_1] = color_nv_para.calIratio[CAL_STATE_GAIN_2] ;

	return 1;
}

static int save_cal_para_to_nv(struct colorDriver_chip *chip)
{
	int i = 0, ret = 0;
	if (chip == NULL){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
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
		hwlog_warn("ROHM_Driver: save_cal_para_to_nv: gain[%d]: [%d, %d, %d, %d]\n", i,
		color_nv_para.calXratio[i], color_nv_para.calYratio[i], color_nv_para.calZratio[i], color_nv_para.calIratio[i]);

	}

	ret = write_color_calibrate_data_to_nv(RGBAP_CALI_DATA_NV_NUM, RGBAP_CALI_DATA_SIZE, "RGBAP", &color_nv_para);
	if(ret < 0){
		hwlog_err("ROHM_Driver: %s: fail\n", __func__);
	}
	return 1;
}

static void osal_calHandl_als(struct colorDriver_chip *chip){
	export_alsData_t outData;
	rohm_bh1749_deviceCtx_t * ctx = NULL;
	uint32_t currentGain = 0;
	uint32_t result = 0;; /* remember, this is for fixed point and can cause lower performance */

	if (chip == NULL){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return;
	}
	if (chip->deviceCtx == NULL){
		hwlog_err("ROHM_Driver: %s: deviceCtx is NULL\n", __func__);
		return;
	}

	ctx = chip->deviceCtx;
	currentGain = (ctx->algCtx.als_data.gain / ROHM_BH1749_GAIN_SCALE);

	rohm_bh1749_deviceGetAls(chip->deviceCtx, &outData);

	hwlog_info("osal_calHandl_als: state %d\n", chip->calibrationCtx.calState);
	hwlog_info("osal_calHandl_als: count %d\n", chip->calibrationCtx.calSampleCounter);
	if((0 == ctx->algCtx.als_data.gain)||(0 == ctx->algCtx.als_data.atime_ms)||(0 == ctx->algCtx.als_data.gain_ir)){
		hwlog_err("%s, gain atime gain_ir err para\n", __func__);
		return;
	}
	if (chip->calibrationCtx.calState != CAL_STATE_GAIN_3){
		chip->calibrationCtx.calSampleCounter++;
		chip->calibrationCtx.calXsample += outData.rawX;
		chip->calibrationCtx.calYsample += outData.rawY;
		chip->calibrationCtx.calZsample += outData.rawZ;
		chip->calibrationCtx.calIRsample += outData.rawIR;

		if(chip->calibrationCtx.calSampleCounter >= ROHM_BH1749_CAL_AVERAGE){
			//X
			hwlog_info("rohm calibrate calXsample = %d, calXtarget = %d, currentGain = %d\n ",\
				chip->calibrationCtx.calXsample, chip->calibrationCtx.calXtarget, currentGain);
			result = (chip->calibrationCtx.calXsample / ROHM_BH1749_CAL_AVERAGE);
			if (result){
				result = result * DATA_TRANSFER_COFF / (ctx->algCtx.als_data.gain / ROHM_BH1749_GAIN_SCALE) / ctx->algCtx.als_data.atime_ms;  //rohm algorithm
				if(0 == result){
					color_calibrate_result = false;
					return;
				}
				result = chip->calibrationCtx.calXtarget * ROHM_BH1749_FLOAT_TO_FIX / result;
				if(result > HIGH_THRESHOLD ||
					result < LOW_THRESHOLD ){
					hwlog_err("%s: ratio is out bound[%d, %d]! calXresult[%d] = %d\n" , __func__,
						HIGH_THRESHOLD,
						LOW_THRESHOLD, chip->calibrationCtx.calState, result);
					color_calibrate_result = false;
				}
			} else {
				/* cant devide by zero, maintain the default calibration scaling factor */
				result = ROHM_BH1749_FLOAT_TO_FIX;
			}
			chip->calibrationCtx.calXresult[chip->calibrationCtx.calState] = result;

			//Y
			hwlog_info("rohm calibrate calYsample = %d, calYtarget = %d, currentGain = %d\n ",\
				chip->calibrationCtx.calYsample, chip->calibrationCtx.calYtarget, currentGain);
			result = (chip->calibrationCtx.calYsample / ROHM_BH1749_CAL_AVERAGE);
			if (result){
				result = result * DATA_TRANSFER_COFF / (ctx->algCtx.als_data.gain / ROHM_BH1749_GAIN_SCALE)  /  ctx->algCtx.als_data.atime_ms;
				if(0 == result){
					color_calibrate_result = false;
					return;
				}
				result = chip->calibrationCtx.calYtarget * ROHM_BH1749_FLOAT_TO_FIX / result;
				if(result > HIGH_THRESHOLD
					|| result <  LOW_THRESHOLD ){
					hwlog_err("%s: ratio is out bound[%d, %d]! calYresult[%d] = %d\n" , __func__,
						HIGH_THRESHOLD,
						LOW_THRESHOLD, chip->calibrationCtx.calState, result);
					color_calibrate_result = false;
				}
			} else {
				/* cant devide by zero, maintain the default calibration scaling factor */
				result = ROHM_BH1749_FLOAT_TO_FIX;
			}
			chip->calibrationCtx.calYresult[chip->calibrationCtx.calState] = result;

			//Z
			hwlog_info("rohm calibrate calZsample = %d, calZtarget = %d, currentGain = %d\n ",\
				chip->calibrationCtx.calZsample, chip->calibrationCtx.calZtarget, currentGain);
			result = (chip->calibrationCtx.calZsample / ROHM_BH1749_CAL_AVERAGE);
			if (result){
				result = result * DATA_TRANSFER_COFF / (ctx->algCtx.als_data.gain / ROHM_BH1749_GAIN_SCALE)  /  ctx->algCtx.als_data.atime_ms;
				if(0 == result){
					color_calibrate_result = false;
					return;
				}
				result = chip->calibrationCtx.calZtarget * ROHM_BH1749_FLOAT_TO_FIX / result;
				if(result > HIGH_THRESHOLD
					|| result < LOW_THRESHOLD ){

					hwlog_err("%s: ratio is out bound[%d, %d]! calZresult[%d] = %d\n" , __func__,
						HIGH_THRESHOLD,
						LOW_THRESHOLD, chip->calibrationCtx.calState, result);
					color_calibrate_result = false;
				}
			} else {
				/* cant devide by zero, maintain the default calibration scaling factor */
				result = ROHM_BH1749_FLOAT_TO_FIX;
			}
			chip->calibrationCtx.calZresult[chip->calibrationCtx.calState] = result;

			//IR
			hwlog_info("rohm calibrate calIRsample = %d, calIRtarget = %d, currentGain = %d\n ",\
				chip->calibrationCtx.calIRsample, chip->calibrationCtx.calIRtarget, currentGain);
			result = (chip->calibrationCtx.calIRsample / ROHM_BH1749_CAL_AVERAGE);
			if (result){
				result = result * DATA_TRANSFER_COFF / (ctx->algCtx.als_data.gain_ir / ROHM_BH1749_GAIN_SCALE)  /  ctx->algCtx.als_data.atime_ms;
				if(0 == result){
					color_calibrate_result = false;
					return;
				}
				result = chip->calibrationCtx.calIRtarget * ROHM_BH1749_FLOAT_TO_FIX / result;
				if(result > HIGH_THRESHOLD||
					result < LOW_THRESHOLD){

					hwlog_err("%s: ratio is out bound[%d, %d]! calIRresult[%d] = %d\n" , __func__,
						HIGH_THRESHOLD,
						LOW_THRESHOLD, chip->calibrationCtx.calState, result);
					color_calibrate_result = false;
				}
			} else {
				/* cant devide by zero, maintain the default calibration scaling factor */
				hwlog_err("%s: calIRresult[%d] = 0!!\n" , __func__, chip->calibrationCtx.calState);
				result = ROHM_BH1749_FLOAT_TO_FIX;
			}
			chip->calibrationCtx.calIRresult[chip->calibrationCtx.calState] = result;

			hwlog_warn("osal_calHandl_als: calXresult  %d\n", chip->calibrationCtx.calXresult[chip->calibrationCtx.calState]);
			hwlog_warn("osal_calHandl_als: calYresult  %d\n", chip->calibrationCtx.calYresult[chip->calibrationCtx.calState]);
			hwlog_warn("osal_calHandl_als: calZresult  %d\n", chip->calibrationCtx.calZresult[chip->calibrationCtx.calState]);
			hwlog_warn("osal_calHandl_als: calIRresult %d\n", chip->calibrationCtx.calIRresult[chip->calibrationCtx.calState]);

			chip->calibrationCtx.calState++;
			chip->calibrationCtx.calSampleCounter = 0;
			chip->calibrationCtx.calXsample = 0;
			chip->calibrationCtx.calYsample = 0;
			chip->calibrationCtx.calZsample = 0;
			chip->calibrationCtx.calIRsample = 0;
			if(chip->calibrationCtx.calState < CAL_STATE_GAIN_3){
				osal_als_enable_set(chip, ROHMDRIVER_ALS_DISABLE);
				ctx->algCtx.als_data.gain = rohm_bh1749_alsGain_conversion[chip->calibrationCtx.calState];
				ctx->algCtx.als_data.gain_ir = rohm_bh1749_alsGain_conversion[chip->calibrationCtx.calState];
				rohm_bh1749_setGain(chip->deviceCtx, ctx->algCtx.als_data.gain /ROHM_BH1749_GAIN_SCALE);
				rohm_bh1749_setIrGain(chip->deviceCtx, ctx->algCtx.als_data.gain_ir /ROHM_BH1749_GAIN_SCALE);
				osal_als_enable_set(chip, ROHMDRIVER_ALS_ENABLE);
			}
		}
	}
	else {
		if(true == color_calibrate_result){
			save_cal_para_to_nv(chip);
			report_calibrate_result = true;
		}else {
			hwlog_err("color_calibrate_result fail\n");
			report_calibrate_result = false;
		}
		chip->inCalMode = false;
		osal_als_enable_set(chip, ROHMDRIVER_ALS_DISABLE);
		rohm_bh1749_setGain(chip->deviceCtx, GAIN_X32);
		rohm_bh1749_setIrGain(chip->deviceCtx, GAIN_X32);
		if(1 == enable_status_before_calibrate){
			osal_als_enable_set(chip, ROHMDRIVER_ALS_ENABLE);
		}else{
			hwlog_info("color sensor disabled before calibrate\n");
		}
		hwlog_info("osal_calHandl_als: done\n");
	}
	return;
}

static void  osal_report_als(struct colorDriver_chip *chip)
{
	export_alsData_t outData = {0};
	uint8_t currentGainIndex = 0;
	uint32_t currentGain = 0;
	rohm_bh1749_deviceCtx_t * ctx = NULL;

	if (chip == NULL){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return;
	}

	ctx = chip->deviceCtx;
	rohm_bh1749_deviceGetAls(chip->deviceCtx, &outData);
	hwlog_debug(" bh1749 report raw data: x = %d, y = %d, z = %d, ir = %d\n",
		outData.rawX, outData.rawY, outData.rawZ, outData.rawIR);

	if(NULL != ctx){
		currentGain = ctx->algCtx.als_data.gain;
	}

	if (currentGain < rohm_bh1749_alsGain_conversion[CAL_STATE_GAIN_2]){
		currentGainIndex = rohm_bh1749_gainToReg(currentGain);
	} else {
		currentGainIndex = CAL_STATE_GAIN_2;
	}

	outData.rawIR *= color_nv_para.calIratio[currentGainIndex];
	outData.rawIR /= ROHM_BH1749_FLOAT_TO_FIX;
	outData.rawX *= color_nv_para.calXratio[currentGainIndex];
	outData.rawX /= ROHM_BH1749_FLOAT_TO_FIX;
	outData.rawY *= color_nv_para.calYratio[currentGainIndex];
	outData.rawY /= ROHM_BH1749_FLOAT_TO_FIX;
	outData.rawZ *= color_nv_para.calZratio[currentGainIndex];
	outData.rawZ /= ROHM_BH1749_FLOAT_TO_FIX;

	hwlog_debug(" bh1749 report  data before calibration: x = %d, y = %d, z = %d, ir = %d\n",
		outData.rawX, outData.rawY, outData.rawZ, outData.rawIR);
	if((ctx->algCtx.als_data.gain == 0) || (ctx->algCtx.als_data.gain_ir == 0) || (ctx->algCtx.als_data.atime_ms == 0)) {
        	return;
        }
	//rohm raw data transform
	outData.rawX = outData.rawX * DATA_TRANSFER_COFF / (ctx->algCtx.als_data.gain / ROHM_BH1749_GAIN_SCALE)  /  ctx->algCtx.als_data.atime_ms;
	outData.rawY = outData.rawY * DATA_TRANSFER_COFF / (ctx->algCtx.als_data.gain / ROHM_BH1749_GAIN_SCALE)  /  ctx->algCtx.als_data.atime_ms;
	outData.rawZ = outData.rawZ * DATA_TRANSFER_COFF / (ctx->algCtx.als_data.gain / ROHM_BH1749_GAIN_SCALE)  /  ctx->algCtx.als_data.atime_ms;
	outData.rawIR = outData.rawIR * DATA_TRANSFER_COFF / (ctx->algCtx.als_data.gain_ir / ROHM_BH1749_GAIN_SCALE)  /  ctx->algCtx.als_data.atime_ms;

	/* adjust the report data when the calibrate ratio is acceptable */
	hwlog_debug(" bh1749 report  data after rohm transform: x = %d, y = %d, z = %d, ir = %d\n",
		outData.rawX, outData.rawY, outData.rawZ, outData.rawIR);

	//copy to report_value
	report_value[0] = (int)outData.rawX;
	report_value[1] = (int)outData.rawY;
	report_value[2] = (int)outData.rawZ;
	report_value[3] = (int)outData.rawIR;

	//report to up level
	rohm_bh1749_report_data(report_value);

#if 1
	report_logcount++;
	if(report_logcount >= ROHM_REPORT_LOG_COUNT_NUM){
		hwlog_info("bh1749 report data %d, %d, %d, %d\n",
			report_value[0], report_value[1], report_value[2], report_value[3]);
		hwlog_info("bh1749: tune als_gains[%d], calXratio[%d], calYratio[%d], calZratio[%d], calIratio[%d]\n, atime[%d], rgb_gain[%d], ir_gain[%d]\n",
			rohm_bh1749_als_gains[currentGainIndex],
			color_nv_para.calXratio[currentGainIndex],
			color_nv_para.calYratio[currentGainIndex],
			color_nv_para.calZratio[currentGainIndex],
			color_nv_para.calIratio[currentGainIndex],
			(UINT16)ctx->algCtx.als_data.atime_ms,
			(UINT32)ctx->algCtx.als_data.gain,
			(UINT32)ctx->algCtx.als_data.gain_ir);
			report_logcount = 0;
	}
#endif
}

#if defined(CONFIG_ROHM_OPTICAL_SENSOR_ALS_XYZ)
static ssize_t osal_als_x_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	export_alsData_t outData;
	if(NULL == dev || NULL == attr || NULL == buf){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return 0;
	}
	struct colorDriver_chip *chip = dev_get_drvdata(dev);
	rohm_bh1749_deviceGetAls(chip->deviceCtx, &outData);
	return snprintf(buf, PAGE_SIZE, "%d\n", outData.rawX);
}

static ssize_t osal_als_y_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	export_alsData_t outData;
	if(NULL == dev || NULL == attr || NULL == buf){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return 0;
	}
	struct colorDriver_chip *chip = dev_get_drvdata(dev);
	rohm_bh1749_deviceGetAls(chip->deviceCtx, &outData);
	return snprintf(buf, PAGE_SIZE, "%d\n", outData.rawY);
}
static ssize_t osal_als_z_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	export_alsData_t outData;
	if(NULL == dev || NULL == attr || NULL == buf){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return 0;
	}
	struct colorDriver_chip *chip = dev_get_drvdata(dev);
	rohm_bh1749_deviceGetAls(chip->deviceCtx, &outData);
	return snprintf(buf, PAGE_SIZE, "%d\n", outData.rawZ);
}

static ssize_t osal_als_ir1_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	export_alsData_t outData;
	if(NULL == dev || NULL == attr || NULL == buf){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return 0;
	}
	struct colorDriver_chip *chip = dev_get_drvdata(dev);
	rohm_bh1749_deviceGetAls(chip->deviceCtx, &outData);
	return snprintf(buf, PAGE_SIZE, "%d\n", outData.rawIR);
}

static ssize_t osal_als_ir2_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	export_alsData_t outData;
	if(NULL == dev || NULL == attr || NULL == buf){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return 0;
	}
	struct colorDriver_chip *chip = dev_get_drvdata(dev);
	rohm_bh1749_deviceGetAls(chip->deviceCtx, &outData);
	return snprintf(buf, PAGE_SIZE, "%d\n", outData.rawIR2);
}
 #endif /* End of  XYZ */

int rohm_bh1749_setenable(bool enable)
{
	struct colorDriver_chip *chip = p_chip;

	if(chip == NULL){
		return 1;
	}
	if (enable)
		osal_als_enable_set(chip, ROHMDRIVER_ALS_ENABLE);
	else
		osal_als_enable_set(chip, ROHMDRIVER_ALS_DISABLE);

	//hwlog_info("rohm_bh1749_setenable success\n");
	return 1;
}
EXPORT_SYMBOL_GPL(rohm_bh1749_setenable);

static ssize_t osal_als_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	if(NULL == dev || NULL == attr || NULL == buf){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return 0;
	}
	struct colorDriver_chip *chip = dev_get_drvdata(dev);
	rohm_bh1749_rohm_mode_t mode = ROHM_BH1749_MODE_OFF;

	rohm_bh1749_deviceGetMode(chip->deviceCtx, &mode);
	if (mode & ROHM_BH1749_MODE_ALS){
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
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return 0;
	}
	struct colorDriver_chip *chip = dev_get_drvdata(dev);
	bool value;
	if (strtobool(buf, &value))
		return -EINVAL;

	if (value)
		osal_als_enable_set(chip, ROHMDRIVER_ALS_ENABLE);
	else
		osal_als_enable_set(chip, ROHMDRIVER_ALS_DISABLE);

	return size;
}


void rohm_show_calibrate(struct colorDriver_chip *chip, color_sensor_output_para * out_para)
{
	int i = 0;
	if (NULL == out_para || NULL == chip){
		hwlog_err("rohm_store_calibrate input para NULL \n");
		return;
	}

	if (chip->inCalMode == false){
		hwlog_err("rohm_show_calibrate not in calibration mode \n");
	}
	//if (chip->calibrationCtx.calState == CAL_STATE_GAIN_LAST){
	out_para->result = (UINT32)report_calibrate_result;
	hwlog_warn(" color_calibrate_show result = %d\n", out_para->result);
	memcpy(out_para->report_ir, chip->calibrationCtx.calIRresult, sizeof(out_para->report_ir));
	memcpy(out_para->report_x,  chip->calibrationCtx.calXresult,  sizeof(out_para->report_x));
	memcpy(out_para->report_y,  chip->calibrationCtx.calYresult,  sizeof(out_para->report_y));
	memcpy(out_para->report_z,  chip->calibrationCtx.calZresult,  sizeof(out_para->report_z));
	//}
	for(i = 0;i<CAL_STATE_GAIN_LAST; i++)
	{
		hwlog_warn(" color_calibrate_show i = %d: %d,%d,%d,%d.\n", i,
		out_para->report_x[i],out_para->report_y[i],out_para->report_z[i],out_para->report_ir[i]);
		hwlog_warn(" calibrationCtx i = %d: %d,%d,%d,%d.\n", i,
		chip->calibrationCtx.calXresult[i],chip->calibrationCtx.calYresult[i],
		chip->calibrationCtx.calZresult[i],chip->calibrationCtx.calIRresult[i]);
	}

	return;
}
void rohm_store_calibrate(struct colorDriver_chip *chip, color_sensor_input_para *in_para)
{
	rohm_bh1749_deviceCtx_t * ctx = NULL;
	rohm_bh1749_rohm_mode_t mode = 0;

	if((NULL == chip)||(in_para == NULL)){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return;
	}

	if (in_para->enable &&  chip->inCalMode){
		hwlog_err("rohm_store_calibrate: Already in calibration mode.\n");
		return;
	}

	if (in_para->enable){
		ctx = chip->deviceCtx;
		hwlog_warn("rohm_store_calibrate: starting calibration mode\n");
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
		hwlog_warn("rohm_store_calibrate: target cali x = %d, y = %d, z = %d, ir = %d in calibration mode.\n", in_para->tar_x,in_para->tar_y,in_para->tar_z,in_para->tar_ir);
		if(NULL != ctx){
			ctx->algCtx.als_data.gain = rohm_bh1749_alsGain_conversion[CAL_STATE_GAIN_1];
			ctx->algCtx.als_data.gain_ir = rohm_bh1749_alsGain_conversion[CAL_STATE_GAIN_1];
		}

		rohm_bh1749_deviceGetMode(chip->deviceCtx, &mode);
		if ((mode & ROHM_BH1749_MODE_ALS) == ROHM_BH1749_MODE_ALS){
			enable_status_before_calibrate = 1;//enabled before calibrate
			hwlog_warn("ROHM_Driver: %s: enabled before calibrate\n", __func__);
			osal_als_enable_set(chip, ROHMDRIVER_ALS_DISABLE);
			msleep(10);//sleep 10 ms to make sure disable timer
		}else{
			enable_status_before_calibrate = 0;//disabled before calibrate
			hwlog_warn("ROHM_Driver: %s: disabled before calibrate\n", __func__);
		}
		chip->inCalMode = true;
		color_calibrate_result = true;//make the calibrate_result true for calibrate again!!
		rohm_bh1749_setGain(chip->deviceCtx, ctx->algCtx.als_data.gain /ROHM_BH1749_GAIN_SCALE);
		rohm_bh1749_setIrGain(chip->deviceCtx, ctx->algCtx.als_data.gain_ir /ROHM_BH1749_GAIN_SCALE);
		osal_als_enable_set(chip, ROHMDRIVER_ALS_ENABLE);
	} else {
		hwlog_warn("rohm_store_calibrate: stopping calibration mode\n");
		chip->inCalMode = false;
	}

	return;
}
void rohm_show_enable(struct colorDriver_chip *chip, int *state)
{
	rohm_bh1749_rohm_mode_t mode = 0;
	if((NULL == chip) || (NULL == state)){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return;
	}

	rohm_bh1749_deviceGetMode(chip->deviceCtx, &mode);
	if (mode & ROHM_BH1749_MODE_ALS){
		*state = 1;
	} else {
		*state = 0;
	}
}

void rohm_store_enable(struct colorDriver_chip *chip, int state)
{
	rohm_bh1749_rohm_mode_t mode = 0;

	if(NULL == chip){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return;
	}
	if (state)
		osal_als_enable_set(chip, ROHMDRIVER_ALS_ENABLE);
	else
		osal_als_enable_set(chip, ROHMDRIVER_ALS_DISABLE);
}
#endif


/****************************************************************************
 *                     OSAL Linux Input Driver
 ****************************************************************************/
static int rohmdriver_pltf_power_on(struct colorDriver_chip *chip)
{
	int rc = 0;
	if(NULL == chip){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return rc;
	}
	return rc;
}

static int rohmdriver_pltf_power_off(struct colorDriver_chip *chip)
{
	int rc = 0;
	if(NULL == chip){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return rc;
	}
	return rc;
}

static int rohmdriver_power_on(struct colorDriver_chip *chip)
{
	int rc = 0;
	if(NULL == chip){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return rc;
	}
	rc = rohmdriver_pltf_power_on(chip);
	if (rc){
		return rc;
	}
	hwlog_err("%s: chip was off, restoring regs\n",__func__);
	rohm_bh1749_deviceInit(chip->deviceCtx, chip->client);
	return 0;
}

#ifdef CONFIG_ROHM_OPTICAL_SENSOR_ALS
static int osal_als_idev_open(struct input_dev *idev)
{
	if(NULL == idev){
		hwlog_err("\nROHM_Driver: %s: Pointer is NULL\n", __func__);
		return 0;
	}
	struct colorDriver_chip *chip = dev_get_drvdata(&idev->dev);
	if(NULL == chip){
		hwlog_err("\nROHM_Driver: %s: Pointer is NULL\n", __func__);
		return 0;
	}

	int rc = 0;

	dev_info(&idev->dev, "%s\n", __func__);
	ROHM_MUTEX_LOCK(&chip->lock);
	if (chip->unpowered) {
		rc = rohmdriver_power_on(chip);
		if (rc)
			goto chip_on_err;
	}

	rc = osal_als_enable_set(chip, ROHMDRIVER_ALS_ENABLE);
	if (rc)
		rohmdriver_pltf_power_off(chip);
chip_on_err:
	ROHM_MUTEX_UNLOCK(&chip->lock);
	return 0;
}

static void osal_als_idev_close(struct input_dev *idev)
{
	if(NULL == idev){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return;
	}
	int rc = 0;
	struct colorDriver_chip *chip = dev_get_drvdata(&idev->dev);
	if(NULL == chip){
		hwlog_err("\nROHM_Driver: %s: Pointer is NULL\n", __func__);
		return 0;
	}

	hwlog_info("%s\n", __func__);

	ROHM_MUTEX_LOCK(&chip->lock);
	rc = osal_als_enable_set(chip, ROHMDRIVER_ALS_DISABLE);
	if (rc){
		rohmdriver_pltf_power_off(chip);
	}
	ROHM_MUTEX_UNLOCK(&chip->lock);
}
#endif

static void rohmdriver_work(struct work_struct *work)
{
	int ret = 0;
	bool re_enable = false;
	rohm_bh1749_rohm_mode_t mode = 0;

	if(NULL == work){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return;
	}
    	struct colorDriver_chip *chip = container_of(work, struct colorDriver_chip, als_work);
	if(NULL == chip){
		hwlog_err("ROHM_Driver: %s: Pointer chip is NULL\n", __func__);
		return;
	}

	ROHM_MUTEX_LOCK(&chip->lock);

	if(0 == read_nv_first_in){
		ret = get_cal_para_from_nv();
		if(!ret){
			hwlog_err("\rohm_bh1749: get_cal_para_from_nv fail \n");
		}
		read_nv_first_in = -1;// -1: do not read again.
	}

	hwlog_debug("rohmdriver_work\n");

	re_enable = rohm_bh1749_deviceEventHandler((rohm_bh1749_deviceCtx_t*)chip->deviceCtx, chip->inCalMode);
	hwlog_debug("rohm_bh1749:  re_enable  =%d \n", re_enable);

#ifdef CONFIG_ROHM_OPTICAL_SENSOR_ALS
	if(rohm_bh1749_deviceGetResult((rohm_bh1749_deviceCtx_t*)chip->deviceCtx) & ROHM_BH1749_FEATURE_ALS){
		if (chip->inCalMode == false ){
			hwlog_debug("rohmdriver_work: osal_report_als\n");
			osal_report_als(chip);
		} else {
			hwlog_warn("rohmdriver_work: calibration mode\n");
			osal_calHandl_als(chip);
		}
	}
#endif

	rohm_bh1749_deviceGetMode(chip->deviceCtx, &mode);
	//hwlog_info("enable status: current_mode  = %d\n", mode);
	if(mode){
		mod_timer(&chip->work_timer, jiffies + msecs_to_jiffies(ROHM_POLL_TIME));// timer set as 150ms
	}

bypass:
	ROHM_MUTEX_UNLOCK(&chip->lock);
}

#ifdef CONFIG_HUAWEI_DSM
static void rohmdriver_dmd_work(void)
{
	if (!dsm_client_ocuppy(shb_dclient)) {
		if (color_devcheck_dmd_result == false){
			dsm_client_record(shb_dclient, "ap_color_sensor_detected fail\n");
			dsm_client_notify(shb_dclient, DSM_AP_ERR_COLORSENSOR_DETECT);
			hwlog_err("ROHM_Driver: %s: DMD ap_color_sensor_detected fail\n", __func__);
		}
	}
}
#endif

int rohmdriver_probe(struct i2c_client *client,
	const struct i2c_device_id *idp)
{
	int ret = 0;
	int i = 0;
	UINT8 deviceId = 0;
	rohm_bh1749_deviceInfo_t rohmDeviceInfo;

	if(NULL == client){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return -1;
	}
	struct device *dev = &client->dev;
	if(NULL == dev){
		hwlog_err("ROHM_Driver: %s: dev Pointer is NULL\n", __func__);
		return -1;
	}

	static struct colorDriver_chip *chip;
	struct driver_i2c_platform_data *pdata = dev->platform_data;

	/****************************************/
	/* Validate bus and device registration */
	/****************************************/
	hwlog_warn("rohm_bh1749: rohmdriver_probe!\n");

	hwlog_info("%s: client->irq = %d\n", __func__, client->irq);
	if (!i2c_check_functionality(client->adapter,
			I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(dev, "%s: i2c smbus byte data unsupported\n", __func__);
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
		chip->calibrationCtx.calXresult[i]  = ROHM_BH1749_FLOAT_TO_FIX ;
		chip->calibrationCtx.calYresult[i]  = ROHM_BH1749_FLOAT_TO_FIX ;
		chip->calibrationCtx.calZresult[i]  = ROHM_BH1749_FLOAT_TO_FIX ;
		chip->calibrationCtx.calIRresult[i] = ROHM_BH1749_FLOAT_TO_FIX ;
		color_nv_para.calXratio[i] = ROHM_BH1749_FLOAT_TO_FIX ;
		color_nv_para.calYratio[i]  = ROHM_BH1749_FLOAT_TO_FIX ;
		color_nv_para.calZratio[i]  = ROHM_BH1749_FLOAT_TO_FIX ;
		color_nv_para.calIratio[i] = ROHM_BH1749_FLOAT_TO_FIX ;
	}

#ifdef CONFIG_HUAWEI_DSM
	INIT_DELAYED_WORK(&rohm_dmd_work, rohmdriver_dmd_work);
#endif
	/********************************************************************/
	/* Validate the appropriate rohm device is available for this driver */
	/********************************************************************/
	deviceId = rohm_bh1749_testForDevice(chip->client);
	hwlog_info("rohm_bh1749 deviceId: %d\n", deviceId);

	if (deviceId == ROHM_BH1749_ID) {
		hwlog_info("rohm_bh1749_testForDevice success\n");
	}else{
#ifdef CONFIG_HUAWEI_DSM
		color_devcheck_dmd_result = false;
		schedule_delayed_work(&rohm_dmd_work, msecs_to_jiffies(AP_COLOR_DMD_DELAY_TIME_MS));
#endif
		hwlog_err( "rohm_bh1749_testForDevice FAILED: ROHM_UNKNOWN_DEVICE\n");
		goto  malloc_failed;
	}

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
	set_hw_dev_flag(DEV_I2C_AP_COLOR_SENSOR);
#endif

	rohm_bh1749_getDeviceInfo(&rohmDeviceInfo);
	chip->deviceCtx = kzalloc(rohmDeviceInfo.memorySize, GFP_KERNEL);
	if (chip->deviceCtx) {
		rohm_bh1749_deviceInit(chip->deviceCtx, chip->client);
	}else {
		hwlog_err("rohm bh1749 kzalloc failed.\n");
		ret = -ENOMEM;
		goto malloc_failed;
	}

	/*********************/
	/* Initialize ALS    */
	/*********************/

#ifdef CONFIG_ROHM_OPTICAL_SENSOR_ALS
	/* setup */
	hwlog_info("Setup for ALS\n");
#endif

        init_timer(&chip->work_timer);
	setup_timer(&chip->work_timer, osal_als_timerHndl, (unsigned long) chip);
        INIT_WORK(&chip->als_work, rohmdriver_work);

	chip->color_show_calibrate_state = rohm_show_calibrate;
	chip->color_store_calibrate_state = rohm_store_calibrate;
	chip->color_enable_show_state = rohm_show_enable;
	chip->color_enable_store_state = rohm_store_enable;
	chip->color_sensor_getGain = rohm_bh1749_getGain;
	chip->color_sensor_setGain = rohm_bh1749_setGain;

	p_chip = chip;
	ret = color_register(chip);
	if(ret < 0){
		hwlog_err("rohm_bh1749: color_register fail \n");
	}
	color_default_enable = rohm_bh1749_setenable;
	hwlog_info("rohm Probe ok.\n");
	return 0;

id_failed:
	if (chip->deviceCtx) kfree(chip->deviceCtx);
	i2c_set_clientdata(client, NULL);
malloc_failed:
	kfree(chip);
init_failed:
	hwlog_err("rohm Probe failed.\n");
	return ret;
}

int rohmdriver_suspend(struct device *dev)
{
	if(NULL == dev){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return -1;
	}
	struct colorDriver_chip  *chip = dev_get_drvdata(dev);
	if(chip == NULL){
		hwlog_err("ROHM_Driver: %s: chip Pointer is NULL\n", __func__);
		return -1;
	}
	hwlog_info("%s\n", __func__);
	ROHM_MUTEX_LOCK(&chip->lock);
	chip->in_suspend = 1;

	if (chip->wake_irq) {
		irq_set_irq_wake(chip->client->irq, 1);
	} else if (!chip->unpowered) {
		hwlog_info("powering off\n");
		/* TODO
		   platform power off */
	}
	ROHM_MUTEX_UNLOCK(&chip->lock);
	return 0;
}

int rohmdriver_resume(struct device *dev)
{
	if(NULL == dev){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return -1;
	}
	struct colorDriver_chip *chip = dev_get_drvdata(dev);

	return 0;
}

int rohmdriver_remove(struct i2c_client *client)
{
	if(NULL == client){
		hwlog_err("ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return -1;
	}
	struct colorDriver_chip *chip = i2c_get_clientdata(client);
	if(NULL == chip){
		hwlog_err("\ROHM_Driver: %s: Pointer is NULL\n", __func__);
		return -1;
	}

	/* TODO
	   platform teardown */
	i2c_set_clientdata(client, NULL);
	if(chip->deviceCtx){
		kfree(chip->deviceCtx);
	}

	if(chip){
		kfree(chip);
	}

	return 0;
}

static struct i2c_device_id rohmdriver_idtable[] = {
	{"rohm_bh1749", 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, rohmdriver_idtable);

static const struct dev_pm_ops rohmdriver_pm_ops = {
	.suspend = rohmdriver_suspend,
	.resume  = rohmdriver_resume,
};

static const struct of_device_id rohmdriver_of_id_table[] = {
	{.compatible = "rohm,bh1749"},
	{},
};


static struct i2c_driver rohmdriver_driver = {
	.driver = {
		.name = "rohm_bh1749",
		.owner = THIS_MODULE,
		.of_match_table = rohmdriver_of_id_table,
	},
	.id_table = rohmdriver_idtable,
	.probe = rohmdriver_probe,
	.remove = rohmdriver_remove,
};

static int __init rohmdriver_init(void)
{
	int rc;
	hwlog_info("rohm_rohm bh1749: init()\n");

	rc = i2c_add_driver(&rohmdriver_driver);

	hwlog_info("rohm_bh1749: %d", rc);
	return rc;
}

static void __exit rohmdriver_exit(void)
{
	hwlog_info("rohm_rohm bh1749: exit()\n");
	i2c_del_driver(&rohmdriver_driver);
}

module_init(rohmdriver_init);
module_exit(rohmdriver_exit);

MODULE_AUTHOR("hujianglei@huawei.com");
MODULE_DESCRIPTION("huawei, rohm color sensor driver");
MODULE_LICENSE("GPL");
