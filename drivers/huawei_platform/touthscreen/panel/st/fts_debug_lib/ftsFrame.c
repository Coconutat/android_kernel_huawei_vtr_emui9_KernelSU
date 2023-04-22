

#include "ftsCrossCompile.h"
#include "ftsError.h"
#include "ftsFrame.h"
#include "ftsHardware.h"
#include "ftsIO.h"
#include "ftsSoftware.h"
#include "ftsTool.h"
#include "ftsTime.h"

#include <linux/init.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <stdarg.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/serio.h>
#include <linux/time.h>
#include <linux/pm.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/power_supply.h>
#include <linux/firmware.h>
#include <linux/regulator/consumer.h>
#include <linux/of_gpio.h>
//#include <linux/sec_sysfs.h>
#include <../../../huawei_touchscreen_chips.h>

static int sense_len = 0, force_len = 0;

int getOffsetFrame(u16 address, u16 *offset) 
{
	
	//offset = (u16*)malloc(sizeof(u16));
	u8 data[2] = {0};
	u8 cmd = { FTS_CMD_FRAMEBUFFER_R };
	//u16 address;
	
	if (readCmdU16(cmd, address, data, OFFSET_LENGTH, DUMMY_FRAMEBUFFER) < 0) {
		TS_LOG_ERR("%s getOffsetFrame: ERROR %02X\n", __func__, ERROR_I2C_R);
		return ERROR_I2C_R;
	}
	else {
		u8ToU16(data, offset);
		TS_LOG_INFO("%s Offest = %02X %02X\n", __func__, data[0], data[1]);
		return OK;
	}
}

int getChannelsLength(void)
{
	int ret;

	u8* data = (u8*)kmalloc(2*sizeof(u8), GFP_KERNEL);
	if(data==NULL){
		TS_LOG_ERR("%s getChannelsLength: ERROR %02X\n",__func__, ERROR_ALLOC);
		return ERROR_ALLOC;
	}

	ret = readB2(ADDR_SENSE_LEN, data, 2);
	if(ret < 0){
		TS_LOG_ERR("%s getChannelsLength: ERROR %02X\n",__func__, ERROR_READ_B2);
		sense_len = 27;
		force_len = 15;
		goto out;
	}

	sense_len = (int)data[0];
	force_len = (int)data[1];

out:
	TS_LOG_INFO("%s Force_len = %d   Sense_Len = %d \n", __func__, force_len, sense_len);
	kfree(data);
	return OK;
}

int getFrameData(u16 address, int size, short **frame) 
{
	int i, j, ret;
	u8* data = (u8*)kmalloc(size*sizeof(u8), GFP_KERNEL);
	if(data==NULL){
		TS_LOG_ERR("%s getFrameData: ERROR %02X\n",__func__, ERROR_ALLOC);
		return ERROR_ALLOC;
	}
	
	ret= readCmdU16(FTS_CMD_FRAMEBUFFER_R, address, data, size, DUMMY_FRAMEBUFFER);
	if(ret<0){
		TS_LOG_ERR("%s getFrameData: ERROR %02X\n",__func__, ERROR_I2C_R);
		kfree(data);
		return ERROR_I2C_R;
	}
	j = 0;
	for (i = 0; i < size; i += 2) {
		(*frame)[j] = (short)((data[i + 1] << 8) + data[i]);
		j++;
	}
	kfree(data);
	return OK;											
}

int getMSFrame(u16 type, short **frame, int keep_first_row) 
{
	u16 offset;
	int size, ret;	

	if (getSenseLen() == 0 || getForceLen() == 0) {
		ret=getChannelsLength();
		if(ret<0){
			TS_LOG_ERR("%s getMSFrame: ERROR %02X\n",__func__,ERROR_CH_LEN);
			return (ret|ERROR_CH_LEN);
		}
	}

	ret = getOffsetFrame(type, &offset);
	if (ret<0) {
		TS_LOG_ERR( "%s getMSFrame: ERROR %02X\n", __func__, ERROR_GET_OFFSET);
		return (ret | ERROR_GET_OFFSET);
	}
	
	switch (type) {
		case ADDR_RAW_TOUCH:				
		case ADDR_FILTER_TOUCH:					
		case ADDR_NORM_TOUCH:					
	    case ADDR_CALIB_TOUCH:
			if(keep_first_row ==1)
				size = ((force_len+1)*sense_len);
			else {
				size = ((force_len)*sense_len);
				offset+= (sense_len * BYTES_PER_NODE);
			}
			break;

		default:
			TS_LOG_ERR("%s getMSFrame: ERROR % 02X\n", __func__, ERROR_OP_NOT_ALLOW);
			return ERROR_OP_NOT_ALLOW;
	}
	
	*frame = (short*)kmalloc(size*sizeof(short), GFP_KERNEL);
	if(*frame==NULL){
		TS_LOG_ERR("%s getMSFrame: ERROR %02X\n",__func__, ERROR_ALLOC);
		return ERROR_ALLOC;
	}

	ret = getFrameData(offset, size*BYTES_PER_NODE, frame);
	if(ret<0){
		TS_LOG_ERR("%s getMSFrame: ERROR %02X\n",__func__, ERROR_GET_FRAME_DATA);
	 	return (ret|ERROR_GET_FRAME_DATA);	
	}
	// if i want to access one node i should compute the offset like offset = i*columns + j = > frame[i, j]	

	TS_LOG_INFO("%s Frame acquired! \n", __func__);
	return size;																		//return the number of data put inside frame

}

int getSSFrame(u16 type, short **frame) 
{
	u16 offset;
	int size, ret;

	if (getSenseLen() == 0 || getForceLen() == 0) {
		ret = getChannelsLength();
		if (ret<0) {
			TS_LOG_ERR("%s getSSFrame: ERROR %02X\n", __func__, ERROR_CH_LEN);
			return (ret | ERROR_CH_LEN);
		}
	}

	switch (type) {
	case ADDR_RAW_HOVER_FORCE:
	case ADDR_FILTER_HOVER_FORCE:
	case ADDR_NORM_HOVER_FORCE:
	case ADDR_CALIB_HOVER_FORCE:
	case ADDR_RAW_PRX_FORCE:
	case ADDR_FILTER_PRX_FORCE:
	case ADDR_NORM_PRX_FORCE:
	case ADDR_CALIB_PRX_FORCE:
		size = ((force_len)* 1);
		break;

	case ADDR_RAW_HOVER_SENSE:
	case ADDR_FILTER_HOVER_SENSE:
	case ADDR_NORM_HOVER_SENSE:
	case ADDR_CALIB_HOVER_SENSE:
	case ADDR_RAW_PRX_SENSE:
	case ADDR_FILTER_PRX_SENSE:
	case ADDR_NORM_PRX_SENSE:
	case ADDR_CALIB_PRX_SENSE:
		size = ((1)*sense_len);
		break;

	default:
		TS_LOG_ERR("%s getSSFrame: ERROR % 02X\n", __func__, ERROR_OP_NOT_ALLOW);
		return ERROR_OP_NOT_ALLOW;

	}

	ret = getOffsetFrame(type, &offset);
	if (ret<0) {
		TS_LOG_ERR("%s getSSFrame: ERROR %02X\n", __func__, ERROR_GET_OFFSET);
		return (ret | ERROR_GET_OFFSET);
	}

	*frame = (short*)kmalloc(size*sizeof(short), GFP_KERNEL);
	if (*frame == NULL) {
		TS_LOG_ERR("%s getSSFrame: ERROR %02X\n", __func__, ERROR_ALLOC);
		return ERROR_ALLOC;
	}

	ret = getFrameData(offset, size*BYTES_PER_NODE, frame);
	if (ret<0) {
		TS_LOG_ERR("%s getSSFrame: ERROR %02X\n", __func__, ERROR_GET_FRAME_DATA);
		return (ret | ERROR_GET_FRAME_DATA);
	}
	// if i want to access one node i should compute the offset like offset = i*columns + j = > frame[i, j]	

	TS_LOG_INFO("%s Frame acquired! \n", __func__);
	return size;

}

int getNmsFrame(u16 type, short ***frames, int *size, int keep_first_row, int fs, int n) 
{
	int i;
	StopWatch global, local;// structure for computing the time elapsed during the total execution or the single iteration
	int temp;

	*frames = (short **)kmalloc(n*sizeof(short *), GFP_KERNEL);
	//sizes = (int *)malloc(n*sizeof(int));
	
	if(*frames==NULL){
		TS_LOG_ERR("%s getNmsFrame: ERROR %02X\n",__func__, ERROR_ALLOC);
		return ERROR_ALLOC;
	}

	fs = (1*1000 / fs) ;													//convert the sample frequency[Hz] in ms
	//int sizes[n];
																			
	startStopWatch(&global);
	for (i = 0; i < n; i++) {
		startStopWatch(&local);
		
		*size = getMSFrame(type, ((*frames)+i), keep_first_row);
		if (*size < 0) {
			TS_LOG_ERR("%s getNFrame: getFrame failed\n",__func__);
			return *size;
		}
		
		stopStopWatch(&local);
		temp = elapsedMillisecond(&local);
		TS_LOG_INFO("%s Iteration %d performed in %d ms... the process wait for %ld ms\n\n", __func__, i, temp, (unsigned long)(fs - temp));

		if (temp < fs)
			mdelay((unsigned long)(fs - temp));
	}
	
	stopStopWatch(&global);
	temp = elapsedMillisecond(&global);
	TS_LOG_INFO("%s Global Iteration performed in %d ms \n", __func__, temp);
	temp /= n;
	TS_LOG_INFO("%s Mean Iteration performed in %d ms \n", __func__, temp);
	return (1000 / (temp));
}

int getSenseLen(void) 
{
	int ret;
	if(sense_len!=0)
		return sense_len;
	else {
		ret = getChannelsLength();
		if (ret < 0)
			return ret;
		else
			return sense_len;
	}
}

int getForceLen(void) 
{
	int ret;
	if (force_len != 0)
		return force_len;
	else {
		ret = getChannelsLength();
		if (ret < 0)
			return ret;
		else
			return force_len;
	}
}
