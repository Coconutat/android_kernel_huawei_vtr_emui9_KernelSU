



#include "ftsSoftware.h"
#include "ftsCrossCompile.h"

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
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/spi/spidev.h>
#include <../../../huawei_touchscreen_chips.h>

static struct i2c_client *client = NULL;

static u16 I2CSAD;


#include "ftsError.h"
#include "ftsHardware.h"
#include "ftsIO.h"

int openChannel(struct i2c_client *clt)
{
	client = clt;
	I2CSAD = clt->addr;
	TS_LOG_INFO("%s openChannel: SAD: %02X \n",__func__, I2CSAD);
	return OK;
}

struct device * getDev(){
	if(client!=NULL)
		return &(client->dev);
	else 
		return NULL;
}

int readCmd(u8* cmd, int cmdLength, u8* outBuf, int byteToRead) {
	int ret;
	struct i2c_msg                I2CMsg[2];

	//write msg
	I2CMsg[0].addr = (__u16)I2CSAD;
	I2CMsg[0].flags = (__u16)0;
	I2CMsg[0].len = (__u16)cmdLength;
	I2CMsg[0].buf = (__u8*)cmd;

	//read msg
	I2CMsg[1].addr = (__u16)I2CSAD;
	I2CMsg[1].flags = I2C_M_RD;
	I2CMsg[1].len = byteToRead;
	I2CMsg[1].buf = (__u8*)outBuf;

	if (client == NULL) return ERROR_I2C_O;
	ret = i2c_transfer(client->adapter, I2CMsg,2);
	if (ret < 0) return ERROR_I2C_R;
	return OK;
}

int writeCmd(u8 *cmd, int cmdLength)
{
	int ret = 0;
	struct i2c_msg                I2CMsg[2];
	
	I2CMsg[0].addr = (__u16)I2CSAD;
	I2CMsg[0].flags = (__u16)0;
	I2CMsg[0].len = (__u16)cmdLength;
	I2CMsg[0].buf = (__u8*)cmd;

	if (client == NULL) return ERROR_I2C_O;
	ret = i2c_transfer(client->adapter, I2CMsg, 1);
	if (ret < 0) return ERROR_I2C_W;

	return OK;
}

int writeReadCmd(u8 *writeCmd1, int writeCmdLength, u8 *readCmd1, int readCmdLength, u8 *outBuf, int byteToRead)
{
	int ret;
	struct i2c_msg                I2CMsg[3];
	

	//write msg
	I2CMsg[0].addr = (__u16)I2CSAD;
	I2CMsg[0].flags = (__u16)0;
	I2CMsg[0].len = (__u16)writeCmdLength;
	I2CMsg[0].buf = (__u8*)writeCmd1;

	//write msg
	I2CMsg[1].addr = (__u16)I2CSAD;
	I2CMsg[1].flags = (__u16)0;
	I2CMsg[1].len = (__u16)readCmdLength;
	I2CMsg[1].buf = (__u8*)readCmd1;

	//read msg
	I2CMsg[2].addr = (__u16)I2CSAD;
	I2CMsg[2].flags = I2C_M_RD;
	I2CMsg[2].len = byteToRead;
	I2CMsg[2].buf = (__u8*)outBuf;

	if (client == NULL) return ERROR_I2C_O;
	ret = i2c_transfer(client->adapter, I2CMsg, 3);


	if (ret < 0){
		TS_LOG_ERR("%s writeCmdU32: ERROR %02X\n", __func__, ERROR_I2C_WR);
	    return ERROR_I2C_WR;
	}
	return OK;
}

int readCmdU16(u8 cmd, u16 address, u8* outBuf, int byteToRead, int hasDummyByte)
{

	int remaining = byteToRead;
	int toRead = 0;
	u8 rCmd[3] = { cmd, 0x00, 0x00 };

	u8* buff = (u8*)kmalloc((READ_CHUNK + 1)*sizeof(u8), GFP_KERNEL);
	if(buff==NULL){
		TS_LOG_ERR("%s readCmdU16: ERROR %02X \n",__func__, ERROR_ALLOC);
		return ERROR_ALLOC;
	}

	while (remaining > 0)
	{
		if (remaining >= READ_CHUNK)
		{
			toRead = READ_CHUNK;
			remaining -= READ_CHUNK;
		}
		else
		{
			toRead = remaining;
			remaining = 0;
		}

		rCmd[1] = (u8)((address & 0xFF00) >> 8);
		rCmd[2] = (u8)(address & 0xFF);

		if (hasDummyByte)
		{
			if (readCmd(rCmd, 3, buff, toRead + 1) < 0){
				TS_LOG_ERR("%s readCmdU16: ERROR %02X \n", __func__, ERROR_I2C_R);
				kfree(buff);
				return ERROR_I2C_R;
			} 
			memcpy(outBuf, buff + 1, toRead);
		}
		else
		{
			if(readCmd(rCmd, 3, buff, toRead)<0) return ERROR_I2C_R;
			memcpy(outBuf, buff, toRead);
		}

		address += toRead;
		outBuf += toRead;
	}
	kfree(buff);

	return OK;
}

int writeCmdU16(u8 WriteCmd, u16 address, u8* dataToWrite, int byteToWrite)
{

	int remaining = byteToWrite;
	int toWrite = 0;

	u8* buff = (u8*)kmalloc((WRITE_CHUNK + 3)*sizeof(u8), GFP_KERNEL);
	if(buff==NULL){
		TS_LOG_ERR("%s writeCmdU16: ERROR %02X \n",__func__, ERROR_ALLOC);
		return ERROR_ALLOC;
	}

	buff[0] = WriteCmd;


	while (remaining > 0)
	{
		if (remaining >= WRITE_CHUNK)
		{
			toWrite = WRITE_CHUNK;
			remaining -= WRITE_CHUNK;
		}
		else
		{
			toWrite = remaining;
			remaining = 0;
		}

		buff[1] = (u8)((address & 0xFF00) >> 8);
		buff[2] = (u8)(address & 0xFF);
		memcpy(buff + 3, dataToWrite, toWrite);
		if(writeCmd(buff, 3 + toWrite)<0){
			TS_LOG_ERR("%s writeCmdU16: ERROR %d\n", __func__, ERROR_I2C_W);
			kfree(buff);
			return ERROR_I2C_W;
		}
		address += toWrite;
		dataToWrite += toWrite;

	}

	kfree(buff);
	return OK;
}

int writeCmdU32(u8 writeCmd1, u8 writeCmd2, u32 address, u8* dataToWrite, int byteToWrite)
{

	int remaining = byteToWrite;
	int toWrite = 0;

	u8 buff1[3] = { writeCmd1, 0x00, 0x00 };
	u8* buff2 = (u8*)kmalloc((WRITE_CHUNK + 3)*sizeof(u8), GFP_KERNEL);
	if(buff2==NULL){
		TS_LOG_ERR("%s writeCmdU32: ERROR %02X \n",__func__, ERROR_ALLOC);
		return ERROR_ALLOC;
	}
	buff2[0] = writeCmd2;


	while (remaining > 0)
	{
		if (remaining >= WRITE_CHUNK)
		{
			toWrite = WRITE_CHUNK;
			remaining -= WRITE_CHUNK;
		}
		else
		{
			toWrite = remaining;
			remaining = 0;
		}

		buff1[1] = (u8)((address & 0xFF000000) >> 24);
		buff1[2] = (u8)((address & 0x00FF0000) >> 16);
		buff2[1] = (u8)((address & 0x0000FF00) >> 8);
		buff2[2] = (u8)(address & 0xFF);
		memcpy(buff2 + 3, dataToWrite, toWrite);

		if(writeCmd(buff1, 3) < 0){
			TS_LOG_ERR("%s writeCmdU32: ERROR %02X\n", __func__, ERROR_I2C_W);
			kfree(buff2);
			return ERROR_I2C_W;
		}
		if(writeCmd(buff2, 3 + toWrite) < 0){
			TS_LOG_ERR("%s writeCmdU32: ERROR %02X\n", __func__, ERROR_I2C_W);
			kfree(buff2);
			return ERROR_I2C_W;
		}

		address += toWrite;
		dataToWrite += toWrite;

	}
	kfree(buff2);
	return OK;
}


int writeReadCmdU32(u8 wCmd, u8 rCmd, u32 address, u8* outBuf, int byteToRead, int hasDummyByte)
{

	int remaining = byteToRead;
	int toRead = 0;
	u8 reaCmd[3];
	u8 wriCmd[3];

	u8* buff = (u8*)kmalloc((READ_CHUNK + 1)*sizeof(u8), GFP_KERNEL);
	if(buff==NULL){
		TS_LOG_ERR("%s writereadCmd32: ERROR %02X\n",__func__, ERROR_ALLOC);
		return ERROR_ALLOC;
	}

	reaCmd[0] = rCmd;
	wriCmd[0] = wCmd;

	while (remaining > 0)
	{
		if (remaining >= READ_CHUNK)
		{
			toRead = READ_CHUNK;
			remaining -= READ_CHUNK;
		}
		else
		{
			toRead = remaining;
			remaining = 0;
		}

		wriCmd[1] = (u8)((address & 0xFF000000) >> 24);
		wriCmd[2] = (u8)((address & 0x00FF0000) >> 16);

		reaCmd[1] = (u8)((address & 0x0000FF00) >> 8);
		reaCmd[2] = (u8)(address & 0x000000FF);

		if (hasDummyByte)
		{
			if(writeReadCmd(wriCmd, 3, reaCmd, 3, buff, toRead + 1)<0) {
				TS_LOG_ERR("%s writeCmdU32: ERROR %02X \n", __func__, ERROR_I2C_WR);
				kfree(buff);
				return ERROR_I2C_WR;
			}
			memcpy(outBuf, buff + 1, toRead);
		}
		else
		{
			if(writeReadCmd(wriCmd, 3, reaCmd, 3, buff, toRead)<0){
				TS_LOG_ERR("%s writeCmdU32: ERROR %02X\n", __func__, ERROR_I2C_WR);
				kfree(buff);
				return ERROR_I2C_WR;
			}
			memcpy(outBuf, buff, toRead);
		}

		//printf("%02X%02X%02X%02X%02X%02X %02X\n", writeCmd[0], writeCmd[1], writeCmd[2], readCmd[0], readCmd[1], readCmd[2], WriteCmd);   

		address += toRead;

		outBuf += toRead;

	}
	kfree(buff);
	return OK;
}
