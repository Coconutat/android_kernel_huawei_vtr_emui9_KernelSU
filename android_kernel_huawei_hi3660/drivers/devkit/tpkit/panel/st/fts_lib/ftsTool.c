/*

**************************************************************************
**                        STMicroelectronics 		                **
**************************************************************************
**                        marco.cali@st.com				**
**************************************************************************
*                                                                        *
*                     FTS Utility Functions				 *
*                                                                        *
**************************************************************************
**************************************************************************

*/

#include "ftsCompensation.h"
#include "ftsCrossCompile.h"
#include "ftsError.h"
#include "ftsHardware.h"
#include "ftsIO.h"
#include "ftsSoftware.h"
#include "ftsTime.h"
#include "ftsTool.h"
#include "../fts.h"

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
#include <linux/gpio.h>

#define NUM_OF_ROW 16

static int reset_gpio=GPIO_NOT_DEFINED;
static int system_resetted_up = 0;
static int system_resetted_down = 0;

extern chipInfo ftsInfo;
extern int fts_warm_boot(void);

int readB2(u16 address, u8* outBuf, int len) {
	int remaining = len;
	int toRead = 0;
	int retry = 0;
	int ret;
	int event_to_search[3];
	char *temp = NULL;
	u8* init_outBuf = outBuf;
	u16 init_addr = address;
	u8 readEvent[FIFO_EVENT_SIZE] = {0};
	u8 cmd[4] = { FTS_CMD_REQU_FW_CONF, 0x00, 0x00, (u8)len };

	if(readEvent==NULL){
		TS_LOG_ERR("%s readB2: ERROR %02X\n", __func__, ERROR_ALLOC);
		return ERROR_ALLOC;
	}

	u16ToU8_be(address, &cmd[1]);
	temp = printHex("Command B2 = ", cmd, 4);
	if(temp!=NULL)
		TS_LOG_INFO("%s", temp);
	kfree(temp);
	do{
		remaining = len;
		ret = fts_writeFwCmd(cmd, 4);
		if(ret<0) {
			TS_LOG_ERR("%s readB2: ERROR %02X\n", __func__, ERROR_I2C_W);
			return ret;
		}	//ask to the FW the data
		TS_LOG_INFO("%s Command to FW sent!\n", __func__);
		event_to_search[0] = (int)EVENTID_FW_CONFIGURATION;

		while (remaining > OK ) {
			event_to_search[1]= (int)((address & 0xFF00)>>8);
			event_to_search[2]= (int) (address & 0x00FF);
				if (remaining > B2_DATA_BYTES) {
					toRead = B2_DATA_BYTES;
					remaining -= B2_DATA_BYTES;
				} else {
					toRead = remaining;
					remaining = 0;
				}

			ret = pollForEvent(event_to_search, 3, readEvent,GENERAL_TIMEOUT);
			if ( ret >= OK) {			//start the polling for reading the reply
				memcpy(outBuf, &readEvent[3], toRead);
				retry = 0;
				outBuf += toRead;

			} else {
				retry+=1;
				break;
			}
			address+=B2_DATA_BYTES;
		}
		TS_LOG_INFO("%s readB2: B2 failed... attempt = %d\n", __func__, retry);
		outBuf = init_outBuf;
		address = init_addr;
	} while(retry < B2_RETRY && retry!=0);

	if (retry == B2_RETRY){
		TS_LOG_ERR("%s readB2: ERROR %02X\n", __func__, ERROR_TIMEOUT);
		return ERROR_TIMEOUT;
	}
	TS_LOG_INFO("%s B2 read %d bytes\n", __func__, len);
	return OK;
}


int readB2U16( u16 address, u8* outBuf, int byteToRead)
{
	int remaining = byteToRead;
	int toRead = 0;
	int ret;

	u8* buff = (u8*)kmalloc((B2_CHUNK + 1)*sizeof(u8), GFP_KERNEL);
	if(buff==NULL){
		TS_LOG_ERR("%s readB2U16: ERROR %02X\n", __func__, ERROR_ALLOC);
		return ERROR_ALLOC;
	}

	while (remaining > 0) {
		if (remaining >= B2_CHUNK) {
			toRead = B2_CHUNK;
			remaining -= B2_CHUNK;
		} else {
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


int releaseInformation()
{
	int ret;
	u8 cmd[1] = { FTS_CMD_RELEASE_INFO };
	int event_to_search[1];
	u8 readEvent[FIFO_EVENT_SIZE];

	event_to_search[0]=(int)EVENTID_RELEASE_INFO;

	TS_LOG_INFO("%s releaseInformation started... Chip INFO:\n", __func__);

	ret = fts_writeFwCmd(cmd, 1);
	if (ret < OK) {
		TS_LOG_ERR("%s releaseInformation: ERROR %02X\n", __func__, ret);
		return ret;
	}

	ret=pollForEvent(event_to_search, 1, &readEvent[0], RELEASE_INFO_TIMEOUT); //start the polling for reading the reply
	if ( ret < OK) {
		TS_LOG_ERR("%s releaseInformation: ERROR %02X\n", __func__, ret);
		return ret;
	}

	TS_LOG_INFO("%s:releaseInformation: Finished!\n", __func__);
	return OK;
}

int calculateCRC8(u8* u8_srcBuff, int size, u8 *crc)
{
	u8 u8_remainder;
	u8 bit;
	int i=0;
	u8_remainder = 0x00;

	TS_LOG_INFO("%s: Start CRC computing...\n",__func__);
	if(size!=0 && u8_srcBuff!=NULL){
		// Perform modulo-2 division, a byte at a time.
		for ( i = 0; i < size; i++) {//Bring the next byte into the remainder.
			u8_remainder ^= u8_srcBuff[i]; //Perform modulo-2 division, a bit at a time.
			for (bit = 8; bit > 0; --bit) {		//Try to divide the current data bit.
				if (u8_remainder & (0x1 << 7)){
					u8_remainder = (u8_remainder << 1) ^ 0x9B;
				} else {
					u8_remainder = (u8_remainder << 1);
				}
			}
		} // The final remainder is the CRC result.
		*crc = u8_remainder;
		TS_LOG_INFO("%s: CRC value = %02X\n",__func__, *crc);
		return OK;
	}else{
		TS_LOG_ERR("%s: Arguments passed not valid! Data pointer = NULL or size = 0 (%d) ERROR %08X\n",
					__func__, size, ERROR_OP_NOT_ALLOW);
		return ERROR_OP_NOT_ALLOW;
	}

	return OK;
}

#define  PRINT_HEX_EACH_ITEM_LEN  3
#define  PRINT_HEX_MAX_LEN    256
char* printHex(char* label, u8* buff, int count)
{
	int i, offset;
	char *result=NULL;
	if(!label || !buff ){
		TS_LOG_ERR("%s pointer is NULL.\n",__func__);
		return -EINVAL;
	}
	offset = strlen(label);
	if(offset >PRINT_HEX_MAX_LEN ){
		offset = PRINT_HEX_MAX_LEN;
		TS_LOG_ERR("%s offset exceed max length.\n",__func__);
	}
	result = (char*)kzalloc(((offset + PRINT_HEX_EACH_ITEM_LEN * count) + PRINT_HEX_EACH_ITEM_LEN)*sizeof(char), GFP_KERNEL);
	if (result != NULL) {
		strncpy(result, label,offset);

		for (i = 0; i < count; i++) {
			snprintf(&result[offset + i * PRINT_HEX_EACH_ITEM_LEN], 4, "%02X ", buff[i]);
		}
		strncat(result, "\n",PRINT_HEX_EACH_ITEM_LEN);
	}
	return result;
}

int pollForEvent(int * event_to_search, int event_bytes, u8* readData, int time_to_wait)
{
	int i, find, retry, count_err;
	int time_to_count;
	int err_handling=OK;
	StopWatch clock;

	u8 cmd[1] = { FIFO_CMD_READONE };
	char* temp=NULL;

	find = 0;
	retry = 0;
	count_err = 0;
	time_to_count = time_to_wait / TIMEOUT_RESOLUTION;

	startStopWatch(&clock);
	while (find != 1 && retry < time_to_count && fts_readCmd(cmd, 1, readData, FIFO_EVENT_SIZE) >= 0) {
		if (readData[0] == EVENTID_ERROR_EVENT) {
			temp = printHex("ERROR EVENT = ", readData, FIFO_EVENT_SIZE);
			if(temp!=NULL)
				TS_LOG_ERR("%s\n",temp);
			kfree(temp);
			count_err++;
			err_handling=errorHandler(readData, FIFO_EVENT_SIZE);
			if((err_handling&0xF0FF0000)==ERROR_HANDLER_STOP_PROC){
				TS_LOG_ERR("%s pollForEvent: forced to be stopped! ERROR %08X\n", __func__, err_handling);
				return err_handling;
			}
		} else {
			if (readData[0] != EVENTID_NO_EVENT) {
				temp = printHex("READ EVENT = ", readData, FIFO_EVENT_SIZE);
				if(temp!=NULL)
					TS_LOG_ERR("%s",temp);
				kfree(temp);
			}
			if (readData[0] == EVENTID_CONTROL_READY && event_to_search[0]!=EVENTID_CONTROL_READY){
				TS_LOG_ERR("%s pollForEvent: Unmanned Controller Ready Event! Setting reset flags...\n", __func__);
				setSystemResettedUp(1);
				setSystemResettedDown(1);
			}
		}

		find = 1;
		for (i = 0; i < event_bytes; i++) {
			if (event_to_search[i] != -1 && (int)readData[i] != event_to_search[i]) {
				find = 0;
				break;
			}
		}

		retry++;
		mdelay(TIMEOUT_RESOLUTION);
	}
	stopStopWatch(&clock);
	if ((retry >= time_to_count) && find!=1) {
		TS_LOG_ERR("%s pollForEvent: ERROR %02X\n", __func__, ERROR_TIMEOUT);
		return ERROR_TIMEOUT;
	}
	else if (find == 1) {
		temp = printHex("FOUND EVENT = ", readData, FIFO_EVENT_SIZE);
		if(temp!=NULL)
			TS_LOG_INFO("%s",temp);
		kfree(temp);
		TS_LOG_INFO("%s Event found in %d ms (%d iterations)! Number of errors found = %d\n", __func__, elapsedMillisecond(&clock), retry, count_err);
		return count_err;
	} else {
		TS_LOG_ERR("%s pollForEvent: ERROR %02X\n",__func__,ERROR_I2C_R);
		return ERROR_I2C_R;
	}
}

int flushFIFO()
{
	u8 cmd = FIFO_CMD_FLUSH; //flush the FIFO
	if (fts_writeCmd(&cmd, 1)<0) {
		TS_LOG_ERR("%s flushFIFO: ERROR %02X\n", __func__, ERROR_I2C_W);
		return ERROR_I2C_W;
	}

	TS_LOG_INFO("%s FIFO flushed!\n", __func__);
	return OK;

}

int fts_disableInterrupt()
{
	u8 cmd[4] = { FTS_CMD_HW_REG_W, 0x00, 0x00, IER_DISABLE }; //disable interrupt
	u16ToU8_be(IER_ADDR, &cmd[1]);

	if (fts_writeCmd(cmd, 4) < OK) {
		TS_LOG_ERR("%s fts_disableInterrupt: ERROR %02X\n", __func__, ERROR_I2C_W);
		return ERROR_I2C_W;
	}
	TS_LOG_INFO("%s:Interrupt Disabled!\n", __func__);
	return OK;
}


int fts_enableInterrupt() {
	u8 cmd[4] = { FTS_CMD_HW_REG_W, 0x00, 0x00, IER_ENABLE };				//enable interrupt
	u16ToU8_be(IER_ADDR, &cmd[1]);
	if (fts_writeCmd(cmd, 4)<0){
		TS_LOG_ERR("%s fts_enableInterrupt: ERROR %02X\n", __func__, ERROR_I2C_W);
		return ERROR_I2C_W;
	}
	TS_LOG_INFO("%s Interrupt Enabled!\n", __func__);
	return OK;
}

int u8ToU16n(u8* src, int src_length, u16* dst) {
	int i, j;

	if (src_length % 2 != 0) {
		return -1;
	} else {
		j = 0;
		dst = (u16*)kmalloc((src_length / 2)*sizeof(u16), GFP_KERNEL);
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

int u8ToU16_le(u8* src, u16* dst) {
	*dst = (u16)(((src[0] & 0x00FF) << 8) + (src[1] & 0x00FF));
	return 0;
}

int u16ToU8n(u16* src, int src_length, u8* dst)
{
	int i, j;
	dst = (u8*)kmalloc((2 * src_length)*sizeof(u8), GFP_KERNEL);
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

int u8ToU32(u8* src, u32* dst)
{
	*dst = (u32)(((src[3] & 0xFF) << 24) + ((src[2] & 0xFF) << 16) + ((src[1] & 0xFF) << 8) + (src[0] & 0xFF));
	return 0;
}

int u32ToU8(u32 src, u8* dst)
{
	dst[3] = (u8)((src & 0xFF000000) >> 24);
	dst[2] = (u8)((src & 0x00FF0000) >> 16);
	dst[1] = (u8)((src & 0x0000FF00) >> 8);
	dst[0] = (u8)(src & 0x000000FF);
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

void setResetGpio(int gpio)
{
	reset_gpio=gpio;
	TS_LOG_ERR("%s setResetGpio: reset_gpio = %d\n", __func__, reset_gpio);
}

int fts_system_reset()
{
	u8 readData[FIFO_EVENT_SIZE];
	int event_to_search;
	int res=-1;
	int i;
	u8 cmd[4] = { FTS_CMD_HW_REG_W, 0x00, 0x00, SYSTEM_RESET_VALUE };
	event_to_search = (int)EVENTID_CONTROL_READY;

	u16ToU8_be(SYSTEM_RESET_ADDRESS, &cmd[1]);
	TS_LOG_INFO("%s System resetting...\n", __func__);
	for (i=0; i<SYSTEM_RESET_RETRY && res<0; i++){
		if(reset_gpio==GPIO_NOT_DEFINED) {
#ifndef FTM3_CHIP
			res |= fts_warm_boot();
#endif
			res = fts_writeCmd(cmd, 4);
		} else {
			gpio_set_value(reset_gpio, 0);
			mdelay(10);
			gpio_set_value(reset_gpio, 1);
			res=OK;
		}
		if(res<OK) {
			TS_LOG_ERR("%s fts_system_reset: ERROR %02X\n", __func__, ERROR_I2C_W);
		} else {
			res = pollForEvent(&event_to_search, 1, readData, GENERAL_TIMEOUT);
			if ( res < OK) {
				TS_LOG_ERR("%s fts_system_reset: ERROR %02X\n", __func__, res);
			}
		}
	}
	if(res<OK) {
		TS_LOG_ERR("%s fts_system_reset...failed after 3 attempts: ERROR %02X\n", __func__, (res | ERROR_SYSTEM_RESET_FAIL));
		return (res | ERROR_SYSTEM_RESET_FAIL);
	} else {
		TS_LOG_INFO("%s System reset DONE!\n", __func__);
		system_resetted_down = 1;
		system_resetted_up = 1;
		return OK;
	}

}

int isSystemResettedDown()
{
	return system_resetted_down;
}

int isSystemResettedUp()
{
	return system_resetted_up;
}

void setSystemResettedDown(int val)
{
	system_resetted_down = val;
}

void setSystemResettedUp(int val)
{
	system_resetted_up = val;
}

int senseOn()
{
	int ret;
	u8 cmd[1] = { FTS_CMD_MS_MT_SENSE_ON };

	ret = fts_writeFwCmd(cmd, 1);
	if (ret < OK) {
		TS_LOG_ERR("%s senseOn: ERROR %02X\n", __func__, ERROR_SENSE_ON_FAIL);
		return (ret|ERROR_SENSE_ON_FAIL);
	}

	TS_LOG_INFO("%s senseOn: SENSE ON\n", __func__);
	return OK;
}

int senseOff()
{
	int ret;
	u8 cmd[1] = { FTS_CMD_MS_MT_SENSE_OFF };

	ret = fts_writeFwCmd(cmd, 1);
	if (ret < OK) {
		TS_LOG_ERR("%s senseOff: ERROR %02X\n", __func__, ERROR_SENSE_OFF_FAIL);
		return (ret | ERROR_SENSE_OFF_FAIL);
	}

	TS_LOG_INFO("%s senseOff: SENSE OFF\n", __func__);
	return OK;

}

int keyOn()
{
	int ret;
	u8 cmd[1] = { FTS_CMD_MS_KEY_ON };

	ret = fts_writeFwCmd(cmd, 1);
	if (ret < OK) {
		TS_LOG_ERR("%s keyOn: ERROR %02X\n", __func__, ERROR_SENSE_ON_FAIL);
		return (ret | ERROR_SENSE_ON_FAIL);
	}

	TS_LOG_INFO("%s keyOn: KEY ON\n", __func__);
	return OK;

}

int keyOff()
{
	int ret;
	u8 cmd[1] = { FTS_CMD_MS_KEY_OFF };

	ret = fts_writeFwCmd(cmd, 1);
	if (ret < OK) {
		TS_LOG_ERR("%s keyOff: ERROR %02X\n", __func__, ERROR_SENSE_OFF_FAIL);
		return (ret | ERROR_SENSE_OFF_FAIL);
	}

	TS_LOG_INFO("%s keyOff: KEY OFF\n", __func__);
	return OK;

}

int suspensionOn()
{
	int ret;
	u8 cmd[1] = { FTS_CMD_SUSPENSION_ON };

	ret = fts_writeFwCmd(cmd, 1);
	if (ret < OK) {
		TS_LOG_ERR("%s suspensionOn: ERROR %02X\n", __func__, ERROR_SENSE_ON_FAIL);
		return (ret | ERROR_SENSE_ON_FAIL);
	}

	TS_LOG_INFO("%s OK\n", __func__);
	return OK;

}

int suspensionOff()
{
	int ret;
	u8 cmd[1] = { FTS_CMD_SUSPENSION_OFF };

	ret = fts_writeFwCmd(cmd, 1);
	if (ret < OK) {
		TS_LOG_ERR("%s suspensionOff: ERROR %02X\n", __func__, ERROR_SENSE_OFF_FAIL);
		return (ret | ERROR_SENSE_OFF_FAIL);
	}

	TS_LOG_INFO("%s OFF OK\n", __func__);
	return OK;

}

int cleanUp(int enableTouch)
{
	int res;

	TS_LOG_INFO("%s cleanUp: system reset...\n", __func__);
	res = fts_system_reset();
	if (res < OK) return res;
	if (enableTouch) {
		TS_LOG_INFO("%s cleanUp: enabling touches...\n", __func__);
		res = senseOn();
		if (res < OK) return res;
		TS_LOG_INFO("%s cleanUp: enabling interrupts...\n", __func__);
		res = fts_enableInterrupt();
		if (res < OK) return res;
	}
	return OK;
}

int checkEcho(u8 *cmd, int size)
{
	int ret,i;
	int event_to_search[size+1];
	u8 readData[FIFO_EVENT_SIZE];

	if((ftsInfo.u32_echoEn & 0x00000001) != ECHO_ENABLED){
		TS_LOG_DEBUG("%s ECHO Not Enabled!\n", __func__);
		return OK;
	}
	if(size<1 ) {
		TS_LOG_ERR("%s checkEcho: Error Size = %d not valid! or ECHO not Enabled! ERROR %08X\n",
		 		__func__, size, ERROR_OP_NOT_ALLOW);
		return ERROR_OP_NOT_ALLOW;
	} else {
		if((size+2)>FIFO_EVENT_SIZE)
			size = FIFO_EVENT_SIZE-2;	//Echo event EC xx xx xx xx xx xx fifo_status therefore for command with more than 6 bytes will echo only the first 6

		event_to_search[0] = EVENTID_ECHO;
		for(i=1; i<=size; i++) {
			event_to_search[i] = cmd[i-1];
		}
		ret = pollForEvent(event_to_search, size+1, readData, GENERAL_TIMEOUT);
		if ( ret < OK) {
				TS_LOG_ERR("%s checkEcho: Echo Event not found! ERROR %02X\n", __func__, ret);
				return (ret | ERROR_CHECK_ECHO_FAIL);
		}

		TS_LOG_INFO("%s ECHO OK!\n", __func__);
		return OK;
	}

}

int featureEnableDisable(int on_off,u32 feature)
{
	int ret;
	u8 cmd[5];

	if(on_off == FEAT_ENABLE) {
		cmd[0] = FTS_CMD_FEATURE_ENABLE;
		TS_LOG_INFO("%s featureEnableDisable: Enabling feature %08X ...\n", __func__, feature);
	} else {
		cmd[0] = FTS_CMD_FEATURE_DISABLE;
		TS_LOG_INFO("%s featureEnableDisable: Disabling feature %08X ...\n", __func__, feature);
	}

	u32ToU8(feature,&cmd[1]);
	ret = fts_writeCmd(cmd, 5);	//not use writeFwCmd because this function can be called also during interrupt enable and should be fast
	if (ret < OK) {
		TS_LOG_ERR("%s featureEnableDisable: ERROR %02X\n", __func__, ret );
		return (ret | ERROR_FEATURE_ENABLE_DISABLE);
	}

	TS_LOG_INFO("%s featureEnableDisable: DONE!\n", __func__);
	return OK;

}

int writeNoiseParameters(u8 *noise)
{
	int ret,i;
	u8 cmd[2+NOISE_PARAMETERS_SIZE];
	u8 readData[FIFO_EVENT_SIZE];
	int event_to_search[2] = {EVENTID_NOISE_WRITE, NOISE_PARAMETERS};

	TS_LOG_INFO("%s: Writing noise parameters to the IC ...\n", __func__);

	ret = fts_disableInterrupt();
	if (ret < OK) {
		TS_LOG_ERR("%s: ERROR %08X\n", __func__, ret );
		ret = (ret | ERROR_NOISE_PARAMETERS);
		goto ERROR;
	}

	cmd[0] = FTS_CMD_NOISE_WRITE;
	cmd[1] = NOISE_PARAMETERS;

	for(i=0; i<NOISE_PARAMETERS_SIZE; i++){
		cmd[2+i] = noise[i];
	}
	TS_LOG_DEBUG("%s: Noise parameters = 0x%02X  0x%02X  0x%02X  0x%02X\n",
			__func__, noise[0], noise[1], noise[2], noise[3]);

	ret = fts_writeCmd(cmd, NOISE_PARAMETERS_SIZE+2);			//not use writeFwCmd because this function should be fast
	if (ret < OK) {
		TS_LOG_ERR("%s: impossible write command... ERROR %02X\n", __func__, ret );
		ret = (ret | ERROR_NOISE_PARAMETERS);
		goto ERROR;
	}


	ret = pollForEvent(event_to_search, 2, readData, GENERAL_TIMEOUT);
	if (ret < OK) {
		TS_LOG_ERR("%s: polling FIFO ERROR %02X\n", __func__, ret );
		ret = (ret | ERROR_NOISE_PARAMETERS);
		goto ERROR;
	}

	if(readData[2]!=0x00){
		TS_LOG_ERR("%s: Event check FAIL! %02X != 0x00 ERROR %02X\n",
		 	__func__, readData[2], ERROR_NOISE_PARAMETERS);
		ret= ERROR_NOISE_PARAMETERS;
		goto ERROR;
	}

	ret = OK;
ERROR:
	ret = fts_enableInterrupt();			//ensure that the interrupt are always renabled when exit from funct
	if (ret < OK) {
		TS_LOG_ERR("%s: ERROR %02X\n", __func__, ret );
		return (ret | ERROR_NOISE_PARAMETERS);
	}
	return ret;
}


int readNoiseParameters(u8 *noise){
	int ret,i;
	u8 cmd[2];
	u8 readData[FIFO_EVENT_SIZE];
	int event_to_search[2] = {EVENTID_NOISE_READ, NOISE_PARAMETERS};

	TS_LOG_INFO("%s: Reading noise parameters from the IC ...\n", __func__);

	ret = fts_disableInterrupt();
	if (ret < OK) {
		TS_LOG_ERR("%s readNoiseParameters: ERROR %02X\n", __func__, ret );
		ret = (ret | ERROR_NOISE_PARAMETERS);
		goto ERROR;
	}

	cmd[0] = FTS_CMD_NOISE_READ;
	cmd[1] = NOISE_PARAMETERS;


	ret = fts_writeCmd(cmd, 2);			//not use writeFwCmd should be fast
	if (ret < OK) {
		TS_LOG_ERR("%s: impossible write command... ERROR %02X\n", __func__, ret );
		ret = (ret | ERROR_NOISE_PARAMETERS);
		goto ERROR;
	}


	ret = pollForEvent(event_to_search, 2, readData, GENERAL_TIMEOUT);
	if (ret < OK) {
		TS_LOG_ERR("%s: polling FIFO ERROR %02X\n", __func__, ret );
		ret = (ret | ERROR_NOISE_PARAMETERS);
		goto ERROR;
	}

	for(i=0; i<NOISE_PARAMETERS_SIZE; i++){
		noise[i] = readData[2+i];
		TS_LOG_INFO("%02X ",noise[i]);
	}
	TS_LOG_INFO("%s: Noise parameters = 0x%02X  0x%02X  0x%02X  0x%02X\n",
			__func__, noise[0], noise[1], noise[2], noise[3]);
	ret = OK;
ERROR:
	ret = fts_enableInterrupt();			//ensure that the interrupt are always renabled when exit from funct
	if (ret < OK) {
		TS_LOG_ERR("%s: ERROR %02X\n", __func__, ret);
		return (ret | ERROR_NOISE_PARAMETERS);
	}
	return ret;
}

short** array1dTo2d_short(short* data, int size, int columns)
{

	int i;
	short** matrix = (short**)kmalloc(((int)(size / columns))*sizeof(short *), GFP_KERNEL);
	if (matrix != NULL) {
		for (i = 0; i < (int)(size / columns); i++) {
			matrix[i] = (short*)kmalloc(columns*sizeof(short), GFP_KERNEL);
		}

		for (i = 0; i < size; i++) {
			matrix[i / columns][i % columns] = data[i];
		}
	}

	return matrix;
}

u8** array1dTo2d_u8(u8* data, int size, int columns)
{

	int i;
	u8** matrix = (u8**)kmalloc(((int)(size / columns))*sizeof(u8 *), GFP_KERNEL);
	if (matrix != NULL) {
		for (i = 0; i < (int)(size / columns); i++) {
			matrix[i] = (u8*)kmalloc(columns*sizeof(u8), GFP_KERNEL);
		}

		for (i = 0; i < size; i++) {
			matrix[i / columns][i % columns] = data[i];
		}
	}

	return matrix;
}

void print_frame_short(char *label, short **matrix, int row, int column)
{
	int i, j;
	TS_LOG_INFO("%s\n", label);
	for (i = 0; i < row; i++) {
		for (j = 0; j < column; j++) {
			printk("%d ", matrix[i][j]);
		}
		TS_LOG_INFO("\n");
		kfree(matrix[i]);
	}
	kfree(matrix);
}

void print_frame_u8(char *label, u8 **matrix, int row, int column)
{
	int i, j;
	TS_LOG_INFO("%s\n", label);
		for (i = 0; i < row; i++) {
			for (j = 0; j < column; j++) {
				printk("%d ", matrix[i][j]);
			}
			printk("\n");
			kfree(matrix[i]);
		}
	kfree(matrix);
}

void print_frame_u32(char *label, u32 **matrix, int row, int column)
{
	int i, j;
	TS_LOG_INFO("%s\n", label);
	for (i = 0; i < row; i++) {
		for (j = 0; j < column; j++) {
			printk("%d ", matrix[i][j]);
		}
		TS_LOG_INFO("\n");
		kfree(matrix[i]);
	}
	kfree(matrix);
}


void print_frame_int(char *label, int **matrix, int row, int column)
{
	int i, j;
	TS_LOG_INFO("%s\n", label);
	for (i = 0; i < row; i++) {
		for (j = 0; j < column; j++) {
			printk("%d ", matrix[i][j]);
		}
		TS_LOG_INFO("\n");
		kfree(matrix[i]);
	}
	kfree(matrix);
}

int writeLockDownInfo(u8 *data, int size)
{
	int ret,i,toWrite, retry=0, offset =size;
	u8 cmd[2+LOCKDOWN_CODE_WRITE_CHUNK]={0xCA, 0x00};
	u8 crc=0,write_count=0;
	int event_to_search[2] = {EVENTID_STATUS_UPDATE, EVENT_TYPE_LOCKDOWN_WRITE_SPEC};
	u8 readEvent[FIFO_EVENT_SIZE];
	char *temp =NULL;


	TS_LOG_INFO("%s: Writing Lockdown code into the IC ...\n", __func__);

	ret = fts_disableInterrupt();
	if (ret < OK) {
		TS_LOG_ERR("%s: ERROR %08X\n", __func__, ret );
		ret = (ret | ERROR_LOCKDOWN_CODE);
		goto ERROR;
	}

	if(size>LOCKDOWN_CODE_MAX_SIZE){
		TS_LOG_ERR("%s: Lockdown data to write too big! %d>%d  ERROR %08X\n", __func__,size, LOCKDOWN_CODE_MAX_SIZE, ret );
		ret = (ERROR_OP_NOT_ALLOW | ERROR_LOCKDOWN_CODE);
		goto ERROR;
	}


	temp = printHex("Lockdown Code = ",data, size);
	if(temp!=NULL){
		TS_LOG_INFO("%s: %s",  __func__, temp);
		kfree(temp);
	}

	for(retry=0; retry<LOCKDOWN_CODE_RETRY; retry++){

		TS_LOG_INFO("%s: Filling FW buffer ...\n", __func__);
		i=0;
		offset = size;
		cmd[0] = 0xCA;
		while(offset>0){
			if(offset>LOCKDOWN_CODE_WRITE_CHUNK){
				toWrite = LOCKDOWN_CODE_WRITE_CHUNK;
			}else{
				toWrite = offset;
			}

			memcpy(&cmd[2], &data[i], toWrite);
			cmd[1] = i;

			temp = printHex("Commmand = ",cmd,  2+toWrite);
			if(temp!=NULL){
				TS_LOG_INFO("%s: %s",  __func__, temp);
				kfree(temp);
			}

			ret = fts_writeFwCmd(cmd, 2+toWrite);
			if (ret < OK) {
				TS_LOG_ERR("%s: Unable to write Lockdown data at %d iteration.. ERROR %08X\n", __func__,i, ret);
				ret = (ret | ERROR_LOCKDOWN_CODE);
				continue;
			}

			i+=toWrite; 		//update the offset
			offset -=toWrite;

		}

		TS_LOG_INFO("%s: Compute 8bit CRC...\n",  __func__);
		ret = calculateCRC8(data,size,&crc);
		if (ret < OK) {
				TS_LOG_ERR("%s: Unable to compute CRC.. ERROR %08X\n", __func__, ret);
				ret = (ret | ERROR_LOCKDOWN_CODE);
				continue;
		}
		cmd[0] = 0xCD;
		cmd[1] = 0x01;
		cmd[2] = (u8)size;
		cmd[3] = crc;
		logError(0,"%s: Write Lockdown data...\n",__func__);
		temp = printHex("Commmand = ",cmd,  4);
			if(temp!=NULL){
				TS_LOG_INFO("%s: %s",  __func__, temp);
				kfree(temp);
			}

		ret = fts_writeFwCmd(cmd, 4);
		if (ret < OK) {
			TS_LOG_ERR("%s: Unable to send Lockdown data write command... ERROR %08X\n", __func__, ret);
			ret = (ret | ERROR_LOCKDOWN_CODE);
			continue;
		}

		ret = pollForEvent(event_to_search, 2, &readEvent[0], GENERAL_TIMEOUT); //start the polling for reading the reply
		if(ret>=OK){
			write_count = readEvent[2];
			TS_LOG_INFO("%s: Lockdown Code write DONE! it's already write %d times\n", __func__,write_count);
			ret = write_count;
			break;
		}else{
			TS_LOG_ERR("%s: Can not find lockdown code write reply event! ERROR %08X\n",  __func__, ret);
		}
	}

ERROR:
	if (fts_enableInterrupt() < OK) {
		TS_LOG_ERR("%s: Error while re-enabling the interrupt!\n",  __func__);

	}
	return ret;

}

int readLockDownInfo(u8 data_type, u8 *lockData, int buffer_size,
	int *real_size)
{
	int ret;
	int retry = 0;
	int toRead = 0;
	int byteToRead;
	int event_to_search[3] = {EVENTID_LOCKDOWN_INFO_READ,-1,0x00};
	u8 readEvent[FIFO_EVENT_SIZE];
	char *temp =NULL;
	u8 cmd_lock[]={0xCD,0x02,0x00};/* read command */

	TS_LOG_INFO("%s: Reading Lockdown code from the IC, type = %d\n",
			__func__, data_type);

	cmd_lock[2] = data_type; /* byte2 - type */

	ret = fts_disableInterrupt();
	if (ret < OK) {
		TS_LOG_ERR("%s: ERROR %08X\n", __func__, ret );
		ret = (ret | ERROR_LOCKDOWN_CODE);
		goto ERROR;
	}

	for(retry=0; retry<LOCKDOWN_CODE_RETRY; retry++){
		event_to_search[2] = 0x00;
		ret = fts_writeFwCmd(cmd_lock, 3);
		if (ret < OK) {
			TS_LOG_ERR("%s: Unable to send Lockdown data write command... ERROR %08X\n", __func__, ret);
			ret = (ret | ERROR_LOCKDOWN_CODE);
			continue;
		}

		ret = pollForEvent(event_to_search, 3, &readEvent[0], GENERAL_TIMEOUT); //start the polling for reading the reply
		if (ret < OK) {
			TS_LOG_ERR("%s: Can not find first lockdown code read reply event! ERROR %08X\n",
				__func__, ret);
			goto ERROR;

		}

		byteToRead = readEvent[1];
		*real_size = byteToRead;
		TS_LOG_INFO("%s: Lockdown Code size = %d\n", __func__, *real_size);
		if (*real_size > buffer_size) {
			TS_LOG_ERR("%s:read len out of buff size, buffer_sise=%d, read_size=%d\n",
					__func__, buffer_size, *real_size);
			goto ERROR;
		}
		while(byteToRead>0){
			if((readEvent[1]-readEvent[2])>LOCKDOWN_CODE_READ_CHUNK)
				toRead = LOCKDOWN_CODE_READ_CHUNK;
			else
				toRead = readEvent[1]-readEvent[2];
			byteToRead-=toRead;
			memcpy(&lockData[readEvent[2]],&readEvent[3],toRead);
			event_to_search[2] += toRead;
			if(byteToRead>0){
				ret = pollForEvent(event_to_search, 3, &readEvent[0], GENERAL_TIMEOUT); //start the polling for reading the reply
				if(ret<OK){
					TS_LOG_ERR("%s: Can not find lockdown code read reply event with offset %02X ! ERROR %08X\n",
								 __func__, event_to_search[2], ret);
					ret = (ERROR_ALLOC | ERROR_LOCKDOWN_CODE);
					break;
				}
			}
		}
		if(byteToRead!=0){
			TS_LOG_ERR("%s: Read Lockdown code FAIL! ERROR %08X\n", __func__, ret);
			continue;
		} else {
			TS_LOG_INFO("%s: Lockdown Code read DONE!\n", __func__);
			ret = OK;
			temp = printHex("Lockdown Code = ", lockData, *real_size);
			if(temp!=NULL){
				TS_LOG_INFO("%s: %s", __func__, temp);
				kfree(temp);
			}
			break;
		}
	}
ERROR:
	if (fts_enableInterrupt() < OK) {
		TS_LOG_ERR("%s: Error while re-enabling the interrupt!\n", __func__);
	}
	return ret;
}

void short_to_infobuf(int * src ,short * des,int size , int seek)
{
	int i = 0;
	if(src == NULL ||des == NULL)
	{
		TS_LOG_ERR("[%s]NULL point error  return \n",__func__);
		return;
	}
	for(i = 0; i<size ;i++)
		src[i+seek] = des[i];

	for(i = 0 ; i < size; i++)
	{
		if (i% NUM_OF_ROW  == 0)
		{
			printk("[TS_KIT]:");
		}
		printk("\t%6d ", des[i]);
		if (i% NUM_OF_ROW  == NUM_OF_ROW -1)
		{
			printk("\n");
		}
	}
	printk("\n");
}
void uchar_to_infobuf(int * src , unsigned char * des,int size , int seek)
{
	int i = 0;
	if(src == NULL ||des == NULL)
	{
		TS_LOG_ERR("[%s]NULL point error  return \n",__func__);
		return;
	}
	for(i = 0; i<size ;i++)
		src[i+seek] = des[i];

	for(i = 0 ; i < size; i++)
	{
		if (i% NUM_OF_ROW  == 0)
		{
			printk("[TS_KIT]:");
		}
		printk("\t%6d ", des[i]);
		if (i% NUM_OF_ROW  == NUM_OF_ROW -1)
		{
			printk("\n");
		}
	}
	printk("\n");
}

unsigned short forcekeyvalue(unsigned char f_ix1, unsigned char ix2_fm)
{
	TS_LOG_INFO("forcekeyvalue = %d \n", f_ix1 * 2 + ix2_fm);
	return f_ix1 * 2 + ix2_fm;
}
