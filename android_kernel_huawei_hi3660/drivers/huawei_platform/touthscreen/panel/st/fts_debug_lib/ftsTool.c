

#include "ftsCrossCompile.h"
#include "ftsError.h"
#include "ftsHardware.h"
#include "ftsIO.h"
#include "ftsSoftware.h"
#include "ftsTime.h"
#include "ftsTool.h"

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
#include <linux/init.h>
#include <linux/pm.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/power_supply.h>
#include <linux/firmware.h>
#include <../../../huawei_touchscreen_chips.h>

int readB2(u16 address, u8* outBuf, int len) 
{
	int remaining = len;
	int toRead = 0;
	int retry = 0;
	int ret;
	int event_to_search[1];
	u8* readEvent = (u8*)kmalloc(FIFO_EVENT_SIZE*sizeof(u8), GFP_KERNEL);

	if(readEvent == NULL){
		TS_LOG_ERR("%s readB2: ERROR %02X\n",__func__, ERROR_ALLOC);
		return ERROR_ALLOC;
	}

	//logError(1,"%s readB2: Prima disable\n",__func__);
	ret = disableInterrupt();
	if (ret >= 0) {
		u8 cmd[4] = { FTS_CMD_REQU_FW_CONF, 0x00, 0x00, (u8)len };
		u16ToU8_be(address, &cmd[1]);

		TS_LOG_INFO("%s:Comand B2 = %02X,%02X,%02X,%02X\n",__func__, cmd[0], cmd[1], cmd[2], cmd[3]);

		do{
			remaining = len;
			if(writeCmd(cmd, 4)<0) {
				TS_LOG_ERR("%s readB2: ERROR %02X\n",__func__, ERROR_I2C_W);
				kfree(readEvent);
				return ERROR_I2C_W;
			}								//ask to the FW the data
			TS_LOG_INFO("%s Comand to FW sent! \n", __func__);

			event_to_search[0] = (int)EVENTID_FW_CONFIGURATION;
			while (remaining > 0 ) {

				if (remaining > B2_DATA_BYTES) {
					toRead = B2_DATA_BYTES;
					remaining -= B2_DATA_BYTES;
				}
				else {
					toRead = remaining;
					remaining = 0;
				}

				ret=pollForEvent(event_to_search, 1, readEvent,GENERAL_TIMEOUT);
				if ( ret >= 0) {			//start the polling for reading the reply
					memcpy(outBuf, &readEvent[3], toRead);
					retry = 0;
					outBuf += toRead;

				}
				else {
					retry+=1;
					break;
				}

			}
		
		}while(retry < B2_RETRY && retry!=0);
		
		kfree(readEvent);
		if (retry == B2_RETRY){
			TS_LOG_ERR("%s readB2: ERROR %02X\n",__func__, ERROR_TIMEOUT);
			return ERROR_TIMEOUT;
		}
		TS_LOG_INFO("%s B2 read %d bytes\n", __func__, len);

		/*ret=enableInterrupt();
		if(ret<0){
			logError(1,"%s readB2: ERROR %02X \n",__func__, ERROR_ENABLE_INTER);
			return ERROR_ENABLE_INTER;
		}*/
		return OK;
	}	else {
		//enableInterrupt();				//this call is done just for be sure that interrupt are not disabled
		TS_LOG_ERR("%s readB2: ERROR %02X\n",__func__,ERROR_DISABLE_INTER);
		kfree(readEvent);
		return ERROR_DISABLE_INTER;
	}

}

int readB2U16( u16 address, u8* outBuf, int byteToRead)
{

	int remaining = byteToRead;
	int toRead = 0;
	int ret;

	u8* buff = (u8*)kmalloc((B2_CHUNK + 1)*sizeof(u8), GFP_KERNEL);
	if(buff==NULL){
		TS_LOG_ERR("%s readB2U16: ERROR %02X \n",__func__, ERROR_ALLOC);
		return ERROR_ALLOC;
	}

	while (remaining > 0)
	{
		if (remaining >= B2_CHUNK)
		{
			toRead = B2_CHUNK;
			remaining -= B2_CHUNK;
		}
		else
		{
			toRead = remaining;
			remaining = 0;
		}

		ret=readB2(address, buff, toRead);
		if (ret < 0) {
			kfree(buff);
			return ret;
		}
		memcpy(outBuf, buff , toRead);
		

		address += toRead;

		outBuf += toRead;

	}

	kfree(buff);
	return OK;
}

/*char* timestamp() {
	char s[1000];
	char* result;

	time_t t = time(0);
	struct tm * p = localtime(&t);
	size_t count;

	count = strftime(s, 1000, "[%a %d %b %G, %R] ", p);
	//printf("%d", count);
	result = (char *)malloc(count*sizeof(char));
	strncpy(result, s, count);
	//printf("%s %s", s, result);
	return result;
}*/

char* printHex(char* label, u8* buff, int count) 
{
	int i, offset;
	char *result;

	offset = strlen(label);
	result = (char*)kmalloc(((offset + 3 * count) + 1)*sizeof(char), GFP_KERNEL);
	if(NULL == result) {
		TS_LOG_ERR("kmalloc result buffer failed\n");
		return NULL;
	}
	//char result[(offset + 3 * count) + 1];
	strcpy(result, label);
	//printf("%s\n", result);

	for (i = 0; i < count; i++) {
		//printf("%02X ", buff[i]);
		snprintf(&result[offset + i * 3], 4, "%02X ", buff[i]);
		//printf("%s\n", result);
	}
	strcat(result, "\n");
	return result;
}

int pollForEvent(int * event_to_search, int event_bytes, u8* readData, int time_to_wait) 
{
	int i, find, retry, count_err;
	int time_to_count;
	u8 cmd[1] = { FIFO_CMD_READONE };
	//char * temp;

	find = 0;
	retry = 0;
	count_err = 0;
	time_to_count = time_to_wait / TIMEOUT_RESOLUTION;

	while (find != 1 && retry < time_to_count && readCmd(cmd, 1, readData, FIFO_EVENT_SIZE) >= 0) {
		//Log of errors
		if (readData[0] == EVENTID_ERROR_EVENT) {
			TS_LOG_DEBUG("%s ERROR EVENT = %02X %02X %02X %02X %02X %02X %02X %02X\n", __func__,
				readData[0], readData[1],readData[2],readData[3],readData[4],
				readData[5],readData[6],readData[7]);
			count_err++;
		} else {
			if (readData[0] != EVENTID_NO_EVENT) {
				TS_LOG_DEBUG("%s READ EVENT = %02X %02X %02X %02X %02X %02X %02X %02X\n", __func__,
					readData[0], readData[1],readData[2],readData[3],readData[4],
					readData[5],readData[6],readData[7]);
			}
		}

		find = 1;

		for (i = 0; i < event_bytes; i++) {
			if (event_to_search[i] != -1 && (int)readData[i] != event_to_search[i]) {
				TS_LOG_DEBUG("%s READ EVENT = %02X %02X %02X %02X %02X %02X %02X %02X\n", __func__,
					readData[0], readData[1],readData[2],readData[3],readData[4],
					readData[5],readData[6],readData[7]);
				find = 0;
				break;
			}
		}

		retry++;
		mdelay(TIMEOUT_RESOLUTION);
	}

	if ((retry >= time_to_count) && find!=1) {
		TS_LOG_ERR("%s pollForEvent: ERROR %02X (ERROR timeout)\n",__func__, ERROR_TIMEOUT);
		return ERROR_TIMEOUT;
	} else if (find == 1) {
#if 0
		temp = printHex("FOUND EVENT = ", readData, FIFO_EVENT_SIZE);
		TS_LOG_INFO("%s %s",__func__,temp);
		kfree(temp);
		TS_LOG_INFO("%s Event found in %d ms! Number of errors found = %d \n", __func__, retry * TIMEOUT_RESOLUTION,count_err);
#endif
		return count_err;
	} else {
		TS_LOG_ERR("%s pollForEvent: ERROR %02X (ERROR read result)\n",__func__, ERROR_I2C_R);
		return ERROR_I2C_R;
	}
}

int flushFIFO(void) 
{
	u8 cmd = FIFO_CMD_FLUSH;												//flush the FIFO
	if (writeCmd(&cmd, 1)<0) {
		TS_LOG_ERR("%s flushFIFO: ERROR %02X \n", __func__, ERROR_I2C_W);
		return ERROR_I2C_W;
	}

	TS_LOG_INFO("%s FIFO flushed! \n", __func__);
	return OK;

}

int disableInterrupt(void) 
{
	u8 cmd[4] = { FTS_CMD_HW_REG_W, 0x00, 0x00, IER_DISABLE };				//disable interrupt 
	u16ToU8_be(IER_ADDR, &cmd[1]);

	if (writeCmd(cmd, 4) >= 0) {
		cmd[0] = FIFO_CMD_FLUSH;											//flush the FIFO
		if(writeCmd(cmd, 1)<0) {
			TS_LOG_ERR("%s disableInterrupt 1: ERROR %02X \n",__func__, ERROR_I2C_W);
			return ERROR_I2C_W;		
		}
	}
	else{
		TS_LOG_ERR("%s disableInterrupt 2: ERROR %02X\n",__func__, ERROR_I2C_W);
		return ERROR_I2C_W;
	}
	TS_LOG_INFO("%s Interrupt Disabled and FIFO flushed! \n", __func__);
	return OK;
}

int enableInterrupt(void) 
{
	u8 cmd[4] = { FTS_CMD_HW_REG_W, 0x00, 0x00, IER_ENABLE };				//enable interrupt
	u16ToU8_be(IER_ADDR, &cmd[1]);
    if (writeCmd(cmd, 4)<0){
		TS_LOG_ERR("%s enableInterrupt: ERROR %02X\n",__func__,ERROR_I2C_W);
		return ERROR_I2C_W;
	}
	TS_LOG_DEBUG("%s Interrupt Enabled!\n", __func__);
	return OK;
}

int u8ToU16n(u8* src, int src_length, u16* dst) 
{
	int i, j;

	if (src_length % 2 != 0) {
		return -1;
	}
	else {
		j = 0;
		dst = (u16*)kmalloc((src_length / 2)*sizeof(u16), GFP_KERNEL);
		if(NULL == dst){
			TS_LOG_ERR("dst alloc failed\n");
			return ERROR_ALLOC;
		}
		for (i = 0; i < src_length; i += 2) {
			dst[j]= ((src[i+1] & 0x00FF) << 8) + (src[i] & 0x00FF);
			j++;
		}
	}

	return (src_length / 2);
}

int u8ToU16(u8* src, u16* dst) 
{
	*dst = (u16)(((src[1] & 0x00FF) << 8) + (src[0] & 0x00FF));
	return 0;
}

int u8ToU16_le(u8* src, u16* dst) 
{
	*dst = (u16)(((src[0] & 0x00FF) << 8) + (src[1] & 0x00FF));
	return 0;
}

int u16ToU8n(u16* src, int src_length, u8* dst) 
{
	int i, j;
	dst = (u8*)kmalloc((2 * src_length)*sizeof(u8), GFP_KERNEL);
	if(NULL == dst) {
		TS_LOG_ERR("dst alloc failed\n");
		return ERROR_ALLOC;
	}

	j = 0;
	for (i = 0; i < src_length; i++) {
		dst[j] = (u8) (src[i] & 0xFF00)>>8;
		dst[j+1] = (u8) (src[i] & 0x00FF);
		j += 2;
	}

	return src_length * 2;

}

int u16ToU8(u16 src, u8* dst) 
{
	dst[0] = (u8)((src & 0xFF00) >> 8);
	dst[1] = (u8)(src & 0x00FF);
	return 0;
}

int u16ToU8_be(u16 src, u8* dst) 
{
	dst[0] = (u8)((src & 0xFF00) >> 8);
	dst[1] = (u8)(src & 0x00FF);
	return 0;
}

int u16ToU8_le(u16 src, u8* dst) 
{
	dst[1] = (u8)((src & 0xFF00) >> 8);
	dst[0] = (u8)(src & 0x00FF);
	return 0;
}

int attempt_function(int(*code)(void), unsigned long wait_before_retry, int retry_count) 
{
	int result;
	int count = 0;
	
	do {
		result = code();
		count++;
		mdelay(wait_before_retry);
	} while (count < retry_count && result < 0);


	if (count == retry_count)
		return (result | ERROR_TIMEOUT);
	else
		return result;

}

int system_reset(void)
{
	u8 readData[FIFO_EVENT_SIZE];
	int event_to_search;
	int res=-1;
	int i;
	u8 cmd[4] = { FTS_CMD_HW_REG_W, 0x00, 0x00, SYSTEM_RESET_VALUE };
	event_to_search = (int)EVENTID_CONTROLLER_READY;

	u16ToU8_be(SYSTEM_RESET_ADDRESS, &cmd[1]);

	TS_LOG_DEBUG("%s System resetting...\n", __func__);

	for (i=0; i<SYSTEM_RESET_RETRY && res<0; i++){
		res = writeCmd(cmd, 4);
		if(res<0){
			TS_LOG_ERR("%s system_reset: ERROR %02X\n",__func__, ERROR_I2C_W);
			//return (res|ERROR_SYSTEM_RESET_FAIL);
		}else{
			res = pollForEvent(&event_to_search, 1, readData, GENERAL_TIMEOUT);
			if (res < 0) {
				TS_LOG_ERR("%s system_reset: ERROR %02X\n",__func__, res);
				//return (res | ERROR_SYSTEM_RESET_FAIL);
			}
		}	
	}
	if(res<0){
		TS_LOG_ERR("%s system_reset...failed after 3 attempts: ERROR %02X\n",__func__, (res | ERROR_SYSTEM_RESET_FAIL));
		return (res | ERROR_SYSTEM_RESET_FAIL);
	}else{
		TS_LOG_DEBUG("%s System reset DONE!\n", __func__);
		return OK;
	}
}

int senseOn(void) 
{
	int ret;
	u8 cmd[1] = { FTS_CMD_MS_MT_SENSE_ON };
	
	ret = writeCmd(cmd, 1);
	if (ret < 0) {
		TS_LOG_ERR("%s senseOn: ERROR %02X\n", __func__, ERROR_SENSE_ON_FAIL);
		return (ret|ERROR_SENSE_ON_FAIL);
	}
	TS_LOG_DEBUG("%s senseOn: SENSE ON\n", __func__);
	return OK;

}

int senseOff(void) 
{
	int ret;
	u8 cmd[1] = { FTS_CMD_MS_MT_SENSE_OFF };
	
	ret = writeCmd(cmd, 1);
	if (ret < 0) {
		TS_LOG_ERR("%s senseOFF: ERROR %02X\n", __func__, ERROR_SENSE_OFF_FAIL);
		return (ret | ERROR_SENSE_OFF_FAIL);
	}

	TS_LOG_INFO("%s senseOff: SENSE OFF\n", __func__);
	return OK;

}

int cleanUp(int enableTouch) 
{
	int res;

	TS_LOG_DEBUG("%s: system reset...\n", __func__);
	res = system_reset();
	if (res < 0) return res;
	//logError(0, "%s cleanUp: flush FIFO...\n", __func__);
	//res = flushFIFO();
	//if (res < 0) return res;
	if (enableTouch) {
		TS_LOG_DEBUG("%s: enabling interrupts...\n", __func__);
		res = enableInterrupt();
		if (res < 0) return res;
		TS_LOG_DEBUG("%s: enabling touches...\n", __func__);
		res = senseOn();
		if (res < 0) return res;
	}
	return OK;

}

short** array1dTo2d_short(short* data, int size, int columns) {

	int i;
	short** matrix = (short**)kmalloc(((int)(size / columns))*sizeof(short *), GFP_KERNEL);
	if (NULL == matrix) {
		TS_LOG_ERR("matrix alloc failed\n");
		return NULL;
	}

	for (i = 0; i < (int)(size / columns); i++) {
		matrix[i] = (short*)kmalloc(columns*sizeof(short), GFP_KERNEL);
		if (matrix[i]) {
			TS_LOG_ERR("matrix[%d] alloc failed\n", i);
			return NULL;
		}
	}

	for (i = 0; i< size; i++){
		matrix[i / columns][i % columns] = data[i];
	}

	return matrix;
}

u8** array1dTo2d_u8(u8* data, int size, int columns) {

	int i;
	u8** matrix = (u8**)kmalloc(((int)(size / columns))*sizeof(u8 *), GFP_KERNEL);
	if (NULL == matrix) {
		TS_LOG_ERR("matrix alloc failed\n");
		return NULL;
	}
	for (i = 0; i < (int)(size / columns); i++){
		matrix[i] = (u8*)kmalloc(columns*sizeof(u8), GFP_KERNEL);
		if (NULL == matrix[i]) {
			TS_LOG_ERR("matrix[%d] alloc failed\n", i);
			return NULL;
		}
	}

	for (i = 0; i< size; i++){
		matrix[i / columns][i % columns] = data[i];
	}

	return matrix;
}

void print_frame_short(char *label, short **matrix, int row, int column) 
{
	int i, j;
	TS_LOG_INFO("%s %s \n", __func__, label);
	for (i = 0; i < row; i++) {
		TS_LOG_INFO("%s ",__func__);
		for (j = 0; j < column; j++) {
			printk("%d ", matrix[i][j]);
		}
		TS_LOG_INFO("\n");
	}
}

void print_frame_u8(char *label, u8 **matrix, int row, int column)
{
	int i, j;
	TS_LOG_INFO("%s %s \n", __func__, label);
		for (i = 0; i < row; i++) {
			TS_LOG_INFO("%s ",__func__);
			for (j = 0; j < column; j++) {
				printk("%d ", matrix[i][j]);
			}
			TS_LOG_INFO("\n");
		}
}

void print_frame_u32(char *label, u32 **matrix, int row, int column) 
{
	int i, j;
	TS_LOG_INFO("%s %s \n", __func__, label);
	for (i = 0; i < row; i++) {
		TS_LOG_INFO("%s ",__func__);
		for (j = 0; j < column; j++) {
			printk("%d ", matrix[i][j]);
		}
		TS_LOG_INFO("\n");
	}
}


void print_frame_int(char *label, int **matrix, int row, int column) 
{
	int i, j;
	TS_LOG_INFO("%s %s \n", __func__, label);
	for (i = 0; i < row; i++) {
		TS_LOG_INFO("%s ",__func__);
		for (j = 0; j < column; j++) {
			printk("%d ", matrix[i][j]);
		}
		TS_LOG_INFO("\n");
	}
}
