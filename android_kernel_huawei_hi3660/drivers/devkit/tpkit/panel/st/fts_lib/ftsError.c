/*

**************************************************************************
**                        STMicroelectronics 		                **
**************************************************************************
**                        marco.cali@st.com				**
**************************************************************************
*                                                                        *
*                  FTS error/info kernel log reporting			 *
*                                                                        *
**************************************************************************
**************************************************************************

*/


#include <linux/device.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/hrtimer.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/completion.h>
#include <linux/wakelock.h>

#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>

#include "../fts.h"
#include "ftsCrossCompile.h"
#include "ftsError.h"
#include "ftsIO.h"
#include "ftsTool.h"
#include "ftsCompensation.h"

extern chipInfo ftsInfo;

void logError(int force, const char *msg, ...)
{
	if (force) {
		va_list args;
		va_start(args, msg);
		vprintk(msg, args);
		va_end(args);
	}
}

int isI2cError(int error)
{
	if(((error & 0x000000FF) >= (ERROR_I2C_R & 0x000000FF)) && ((error & 0x000000FF) <= (ERROR_I2C_O & 0x000000FF)))
		return 1;
	else
		return 0;
}

int dumpErrorInfo()
{
	int ret,i;
	u8 data[ERROR_INFO_SIZE] = {0};
	u32 sign =0;

	TS_LOG_INFO("%s:Starting dump of error info...\n", __func__);
	if(ftsInfo.u16_errOffset ==INVALID_ERROR_OFFS) {
		TS_LOG_ERR("%s: Invalid error offset ERROR %02X\n", __func__, ERROR_OP_NOT_ALLOW );
		return ERROR_OP_NOT_ALLOW;
	}


	ret=readCmdU16(FTS_CMD_FRAMEBUFFER_R, ftsInfo.u16_errOffset, data, ERROR_INFO_SIZE, DUMMY_FRAMEBUFFER);
	if(ret<OK){
		TS_LOG_ERR("%s: reading data ERROR %02X\n", __func__, ret );
		return ret;
	}

	TS_LOG_ERR("%s:Error Info =\n", __func__);
	u8ToU32(data,&sign);
	if(sign!=ERROR_SIGNATURE)
		TS_LOG_ERR("%s: Wrong Error Signature! Data may be invalid!\n", __func__);
	else
		TS_LOG_ERR("%s: Error Signature OK! Data are valid!\n", __func__);

	for(i=0; i<ERROR_INFO_SIZE; i++){
		if(i%4==0){
			TS_LOG_ERR("%s: %d)\n", __func__, i/4);
		}
		TS_LOG_ERR("%02X ", data[i]);
	}
	TS_LOG_ERR("\n");

	TS_LOG_INFO("%s: dump of error info FINISHED!\n", __func__);
	return OK;
}

int errorHandler(u8 *event, int size){
	int res=OK;
	struct fts_ts_info *info=NULL;

	info = fts_get_info();

	if(info!=NULL && event!=NULL && size>1 && event[0]==EVENTID_ERROR_EVENT){
		TS_LOG_ERR("%s: Starting handling...\n",__func__);
		switch(event[1])			 //TODO: write an error log for undefinied command subtype 0xBA
		{
		case EVENT_TYPE_ESD_ERROR:	//esd
			res=fts_chip_powercycle(info);
			if (res<OK){
				TS_LOG_ERR("%s: Error performing powercycle ERROR %08X\n", __func__, res);
			}

			res=fts_system_reset();
			if (res<OK)
			{
				TS_LOG_ERR("%s: Cannot reset the device ERROR %08X\n", __func__, res);
			}
			res = (ERROR_HANDLER_STOP_PROC|res);
			break;

		case EVENT_TYPE_WATCHDOG_ERROR:	//watchdog
			dumpErrorInfo();
			res=fts_system_reset();
				if (res<OK)
				{
					TS_LOG_ERR("%s: Cannot reset the device ERROR %08X\n", __func__, res);
				}
			res = (ERROR_HANDLER_STOP_PROC|res);
			break;

		case EVENT_TYPE_CHECKSUM_ERROR: //CRC ERRORS
				switch(event[2]){
					case CRC_CONFIG_SIGNATURE:
						TS_LOG_ERR("%s: Config Signature ERROR !\n", __func__);
					break;

					case CRC_CONFIG:
						TS_LOG_ERR("%s: Config CRC ERROR !\n", __func__);
					break;

					case CRC_CX_MEMORY:
						TS_LOG_ERR("%s: CX CRC ERROR !\n", __func__);
					break;
				}
			break;

		case EVENT_TYPE_LOCKDOWN_ERROR:
			//res = (ERROR_HANDLER_STOP_PROC|res); 				//stop lockdown code routines in order to retry
			switch(event[2]){
			case 0x01:
				TS_LOG_ERR("%s: Lockdown code alredy written into the IC !\n", __func__);
				break;

			case 0x02:
			case 0x22:
				TS_LOG_ERR("%s: Lockdown CRC check fail during a WRITE !\n", __func__);
				break;

			case 0x03:
				TS_LOG_ERR("%s: Lockdown WRITE command format wrong !\n", __func__);
				break;

			case 0x04:
			case 0x24:
				TS_LOG_ERR("%s: Lockdown Memory Corrupted! Please contact ST for support !\n", __func__);
				break;

			case 0x11:
				TS_LOG_ERR("%s: NO Lockdown code to READ into the IC !\n", __func__);
				break;

			case 0x12:
				TS_LOG_ERR("%s: Lockdown code data corrupted !\n", __func__);
				break;

			case 0x13:
				TS_LOG_ERR("%s: Lockdown READ command format wrong !\n", __func__);
				break;

			case 0x21:
				TS_LOG_ERR("%s: Exceeded maximum number of Lockdown code REWRITE into the IC !\n", __func__);
				break;

			case 0x23:
				TS_LOG_ERR("%s: Lockdown CRC check fail during a REWRITE !\n", __func__);
				break;
			case 0x31:
				if(0 == event[3])
					TS_LOG_ERR( "%s :lockdown write already exceed writing limit: 0x%X times\n", __func__,event[4]);
				break;
			case 0x33:
				TS_LOG_ERR("%s:lockdown write Invalid command\n", __func__);
				break;
			case 0x41:
				TS_LOG_ERR("%s:lockdown write the type of Data Not Found\n", __func__);
				break;
			case 0x42:
				TS_LOG_ERR("%s:lockdown Read Memory Corrupt:\n", __func__);
				break;
			default:
				TS_LOG_ERR("%s:No valid error type for LOCKDOWN ERROR! \n", __func__);
			}
			break;

		default:
			TS_LOG_ERR("%s : No Action taken!\n", __func__);
		break;

		}
		TS_LOG_ERR("%s : handling Finished! res = %08X\n", __func__, res);
		return res;
	}else{
		TS_LOG_ERR("%s : event Null or not correct size! ERROR %08X\n", __func__, ERROR_OP_NOT_ALLOW);
		return ERROR_OP_NOT_ALLOW;
	}
}
