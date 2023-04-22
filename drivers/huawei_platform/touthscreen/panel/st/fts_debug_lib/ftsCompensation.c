

#include "ftsCrossCompile.h"
#include "ftsCompensation.h"
#include "ftsError.h"
#include "ftsFrame.h"
#include "ftsHardware.h"
#include "ftsIO.h"
#include "ftsSoftware.h"
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

int requestCompensationData(u16 type)
{
	int retry = 0;
	int ret;
	u16 answer;
	int event_to_search[1];
	u8 readEvent[FIFO_EVENT_SIZE];

	u8 cmd[3] = { FTS_CMD_REQU_COMP_DATA, 0x00, 0x00 };							// B8 is the comand for asking compensation data
	event_to_search[0] = (int)EVENTID_COMP_DATA_READ;

	ret=disableInterrupt();
	if(ret<0) {
		TS_LOG_ERR("%s requestCompensationData: ERROR %02X\n",__func__,ERROR_DISABLE_INTER);
		return (ret|ERROR_DISABLE_INTER);
	}

	u16ToU8(type, &cmd[1]);

	//TS_LOG_INFO("%s %s", __func__, printHex("Comand = ", cmd, 3));
	//writeCmd(cmd, 3);										//send the request to the chip to load in memory the Compensation Data

	while (retry < COMP_DATA_READ_RETRY) {
		ret=writeCmd(cmd, 3);										//send the request to the chip to load in memory the Compensation Data
		TS_LOG_INFO("%s Comand = %02X,%02X,%02X\n", __func__, cmd[0],cmd[1],cmd[2]);
		if(ret<0){
			TS_LOG_ERR("%s requestCompensationData:  ERROR %02X\n",__func__, ERROR_I2C_W);
			return ERROR_I2C_W;
		}
		ret=pollForEvent(event_to_search, 1, readEvent,TIMEOUT_REQU_COMP_DATA);
		if (ret < 0) {
			//TODO: choose a policy for handling timeout or error during the polling of FIFO
			//if needed is possible to implement a swith case to log he specific error
			TS_LOG_INFO("%s Event did not Found at %d attemp! or error inside pollForEvent\n", __func__, retry+1);
			//enableInterrupt();
			//return -1;
			retry += 1;
		} else {
			retry = 0;
			break;
		}
	}
	
	/*ret=enableInterrupt();
	if (ret < 0) {
		TS_LOG_ERR("%s requestCompensationData: ERROR %02X\n", __func__, ERROR_ENABLE_INTER);
		return (ret|ERROR_ENABLE_INTER);
	}*/

	if (retry == COMP_DATA_READ_RETRY){
		 TS_LOG_ERR("%s requestCompensationData: ERROR %02X\n",__func__, ERROR_TIMEOUT);
		 return ERROR_TIMEOUT;
	}

	u8ToU16_le(&readEvent[1], &answer);

	if(answer == type)
		return OK;
	else {
		TS_LOG_ERR("%s The event found has a different type of Compensation data ERROR %02X \n", __func__, ERROR_DIFF_COMP_TYPE);
		return ERROR_DIFF_COMP_TYPE;
	}
		
}

int readCompensationDataHeader(u16 type, DataHeader *header, u16 *address) 
{

	u16 offset;
	u16 answer;
	u8 data[COMP_DATA_HEADER] = {0};

	if (getOffsetFrame(ADDR_COMP_DATA, &offset) < 0) {
		TS_LOG_ERR("%s  readCompensationDataHeader: ERROR %02X \n", __func__, ERROR_GET_OFFSET);
		return ERROR_GET_OFFSET;
	}

	if (readCmdU16(FTS_CMD_FRAMEBUFFER_R, offset, data, COMP_DATA_HEADER, DUMMY_FRAMEBUFFER) < 0) {
		TS_LOG_ERR("%s  readCompensationDataHeader: ERROR %02X \n", __func__, ERROR_I2C_R);
		return ERROR_I2C_R;
	}
	TS_LOG_INFO("%s Read Data Header done! \n",__func__);

	if (data[0] != HEADER_SIGNATURE) {
		TS_LOG_ERR("%s readCompensationDataHeader: ERROR %02X The Header Signature was wrong!  %02X != %02X \n", __func__, ERROR_WRONG_COMP_SIGN, data[0],HEADER_SIGNATURE);
		return ERROR_WRONG_COMP_SIGN;
	}

	
	u8ToU16_le(&data[1], &answer);

	
	if (answer != type) {
		TS_LOG_ERR("%s readCompensationDataHeader:  ERROR %02X\n", __func__, ERROR_DIFF_COMP_TYPE);
		return ERROR_DIFF_COMP_TYPE;
	}

	TS_LOG_INFO("%s Type of Compensation data OK! \n", __func__);

	
	header->force_node = (int)data[4];
	header->sense_node = (int)data[5];

	*address = offset + COMP_DATA_HEADER;

	return OK;

}

int readMutualSenseGlobalData(u16 *address, MutualSenseData *global)
{

	u8 data[COMP_DATA_GLOBAL] = {0};

	TS_LOG_DEBUG("%s Address for Global data= %02X \n", __func__, *address);

	if (readCmdU16(FTS_CMD_FRAMEBUFFER_R, *address, data, COMP_DATA_GLOBAL, DUMMY_FRAMEBUFFER) < 0) {
		TS_LOG_ERR("%s readMutualSenseGlobalData: ERROR %02X\n", __func__, ERROR_I2C_R);
		return ERROR_I2C_R;
	}
	TS_LOG_DEBUG("%s Global data Read !\n", __func__);

	global->tuning_ver = data[0];
	global->cx1 = data[1];

	TS_LOG_INFO("%s tuning_ver = %d   CX1 = %d \n", __func__, global->tuning_ver, global->cx1);

	*address += COMP_DATA_GLOBAL;
	return OK;

}

int readMutualSenseNodeData(u16 address, MutualSenseData *node) 
{

	int size = node->header.force_node*node->header.sense_node;
	
	TS_LOG_DEBUG("%s Address for Node data = %02X \n", __func__, address);

	node->node_data = (u8*)kmalloc(size*(sizeof(u8)), GFP_KERNEL);
	if (node->node_data == NULL) {
		TS_LOG_ERR("%s readMutualSenseNodeData: ERROR %02X \n", __func__, ERROR_ALLOC);
		return ERROR_ALLOC;
	}

	TS_LOG_DEBUG("%s Node Data to read %d bytes \n", __func__, size);
	
	if (readCmdU16(FTS_CMD_FRAMEBUFFER_R, address, node->node_data, size, DUMMY_FRAMEBUFFER) < 0) {
		TS_LOG_ERR("%s readMutualSenseNodeData: ERROR %02X \n", __func__, ERROR_I2C_R);
		return ERROR_I2C_R;
	}
	node->node_data_size = size;
	
	TS_LOG_INFO("%s Read node data ok! \n",__func__);

	return size;

}

int readMutualSenseCompensationData(u16 type, MutualSenseData *data)
{
	int ret;
	u16 address;

	if (!(type == MS_TOUCH_ACTIVE || type == MS_TOUCH_LOW_POWER || type == MS_TOUCH_ULTRA_LOW_POWER || type == MS_KEY)) {
		TS_LOG_ERR("%s readMutualSenseCompensationData: Choose a MS type of compensation data ERROR %02X\n", __func__, ERROR_OP_NOT_ALLOW);
		return ERROR_OP_NOT_ALLOW;
	}

	ret = requestCompensationData(type);
	if (ret < 0) {
		TS_LOG_ERR("%s readMutualSenseCompensationData: ERROR %02X\n", __func__, ERROR_REQU_COMP_DATA);
		return (ret|ERROR_REQU_COMP_DATA);
	}

	ret = readCompensationDataHeader(type, &(data->header), &address);
	if ( ret < 0) {
		TS_LOG_ERR("%s readMutualSenseCompensationData: ERROR %02X\n", __func__, ERROR_COMP_DATA_HEADER);
		return (ret|ERROR_COMP_DATA_HEADER);
	}

	ret = readMutualSenseGlobalData(&address, data);
	if (ret < 0) {
		TS_LOG_ERR("%s readMutualSenseCompensationData: ERROR %02X \n", __func__, ERROR_COMP_DATA_GLOBAL);
		return (ret|ERROR_COMP_DATA_GLOBAL);
	}

	ret = readMutualSenseNodeData(address, data);
	if ( ret < 0) {
		TS_LOG_ERR("%s readMutualSenseCompensationData: ERROR %02X\n", __func__, ERROR_COMP_DATA_NODE);
		return (ret|ERROR_COMP_DATA_NODE);
	}

	return OK;

}


int readSelfSenseGlobalData(u16 *address, SelfSenseData *global) 
{
	
	u8 data[COMP_DATA_GLOBAL] = {0};
	
	TS_LOG_INFO("%s Address for Global data= %02X \n", __func__, *address);
	
	if (readCmdU16(FTS_CMD_FRAMEBUFFER_R, *address, data, COMP_DATA_GLOBAL, DUMMY_FRAMEBUFFER) < 0) {
		TS_LOG_ERR("%s readSelfSenseGlobalData: ERROR %02X \n", __func__, ERROR_I2C_R);
		return ERROR_I2C_R;
	}
	
	TS_LOG_INFO("%s Global data Read !\n", __func__);

	global->tuning_ver = data[0];
	global->f_ix1 = data[1];
	global->s_ix1 = data[2];
	global->f_cx1 = data[3];
	global->s_cx1 = data[4];
	global->f_max_n = data[5];
	global->s_max_n = data[6];

	TS_LOG_INFO("%s tuning_ver = %d   f_ix1 = %d   s_ix1 = %d   f_cx1 = %d   s_cx1 = %d \n", __func__, global->tuning_ver, global->f_ix1, global->s_ix1, global->f_cx1, global->s_cx1);
	TS_LOG_INFO("%s max_n = %d   s_max_n = %d \n", __func__, global->f_max_n, global->s_max_n);
	

	*address += COMP_DATA_GLOBAL;

	return OK;

}

int readSelfSenseNodeData(u16 address, SelfSenseData *node)
{
	int size = node->header.force_node*2+node->header.sense_node*2;
	u8 data[size];

	node->ix2_fm = (u8*)kmalloc(node->header.force_node*(sizeof(u8)), GFP_KERNEL);
	if (node->ix2_fm == NULL) {
		TS_LOG_ERR("%s ix2_fm malloc fail\n", __func__);
		return ERROR_ALLOC;
	}
	node->cx2_fm = (u8*)kmalloc(node->header.force_node*(sizeof(u8)), GFP_KERNEL);
	if (node->cx2_fm == NULL) {
		TS_LOG_ERR("%s cx2_fm malloc fail\n", __func__);
		return ERROR_ALLOC;
	}
	node->ix2_sn = (u8*)kmalloc(node->header.sense_node*(sizeof(u8)), GFP_KERNEL);
	if (node->ix2_sn == NULL) {
		TS_LOG_ERR("%s ix2_sn malloc fail\n", __func__);
		return ERROR_ALLOC;
	}
	node->cx2_sn = (u8*)kmalloc(node->header.sense_node*(sizeof(u8)), GFP_KERNEL);
	if (node->cx2_sn == NULL) {
		TS_LOG_ERR("%s cx2_sn malloc fail\n", __func__);
		return ERROR_ALLOC;
	}

	TS_LOG_DEBUG("%s Node Data (Address is %02X) read %d bytes \n", __func__, address, size);

	if (readCmdU16(FTS_CMD_FRAMEBUFFER_R, address, data, size, DUMMY_FRAMEBUFFER) < 0) {
		TS_LOG_ERR("%s readSelfSenseNodeData: ERROR %02X\n", __func__, ERROR_I2C_R);
		return ERROR_I2C_R;
	}

	TS_LOG_INFO("%s Read node data ok! \n", __func__);

	memcpy(node->ix2_fm, data, node->header.force_node);
	memcpy(node->ix2_sn, &data[node->header.force_node], node->header.sense_node);
	memcpy(node->cx2_fm, &data[node->header.force_node+ node->header.sense_node], node->header.force_node);
	memcpy(node->cx2_sn, &data[node->header.force_node*2 + node->header.sense_node], node->header.sense_node);

	return OK;

}

int readSelfSenseCompensationData(u16 type, SelfSenseData *data)
{
	int ret;
	u16 address;

	if (!(type == SS_TOUCH || type == SS_KEY || type == SS_HOVER || type == SS_PROXIMITY)) {
		TS_LOG_ERR("%s readSelfSenseCompensationData: Choose a SS type of compensation data ERROR %02X\n", __func__, ERROR_OP_NOT_ALLOW);
		return ERROR_OP_NOT_ALLOW;
	}

	ret = requestCompensationData(type);
	if ( ret< 0) {
		TS_LOG_ERR("%s readSelfSenseCompensationData: ERROR %02X\n", __func__, ERROR_REQU_COMP_DATA);
		return (ret|ERROR_REQU_COMP_DATA);
	}

	ret = readCompensationDataHeader(type, &(data->header), &address);
	if ( ret < 0) {
		TS_LOG_ERR("%s readSelfSenseCompensationData: ERROR %02X\n", __func__, ERROR_COMP_DATA_HEADER);
		return (ret|ERROR_COMP_DATA_HEADER);
	}

	ret = readSelfSenseGlobalData(&address, data);
	if ( ret < 0) {
		TS_LOG_ERR("%s readSelfSenseCompensationData: ERROR %02X\n", __func__, ERROR_COMP_DATA_GLOBAL);
		return (ret|ERROR_COMP_DATA_GLOBAL);
	}

	ret = readSelfSenseNodeData(address, data);
	if ( ret < 0) {
		TS_LOG_ERR("%s readSelfSenseCompensationData: ERROR %02X\n", __func__, ERROR_COMP_DATA_NODE);
		return (ret|ERROR_COMP_DATA_NODE);
	}

	return OK;

}

int readGeneralGlobalData(u16 address, GeneralData *global)
{
	u8 data[COMP_DATA_GLOBAL] = {0};

	if (readCmdU16(FTS_CMD_FRAMEBUFFER_R, address, data, COMP_DATA_GLOBAL, DUMMY_FRAMEBUFFER) < 0) {
		TS_LOG_ERR("%s readGeneralGlobalData: ERROR %02X \n", __func__, ERROR_I2C_R);
		return ERROR_I2C_R;
	}

	global->ftsd_lp_timer_cal0 = data[0];
	global->ftsd_lp_timer_cal1 = data[1];
	global->ftsd_lp_timer_cal2 = data[2];
	global->ftsd_lp_timer_cal3 = data[3];
	global->ftsa_lp_timer_cal0 = data[4];
	global->ftsa_lp_timer_cal1 = data[5];

	return OK;
}

int readGeneralCompensationData(u16 type, GeneralData *data)
{
	int ret;
	u16 address;

	if (!(type == GENERAL_TUNING)) {
		TS_LOG_ERR("%s readGeneralCompensationData: Choose a GENERAL type of compensation data ERROR %02X\n", __func__, ERROR_OP_NOT_ALLOW);
		return ERROR_OP_NOT_ALLOW;
	}

	ret = requestCompensationData(type);
	if ( ret< 0) {
		TS_LOG_ERR("%s readGeneralCompensationData: ERROR %02X \n", __func__, ERROR_REQU_COMP_DATA);
		return ERROR_REQU_COMP_DATA;
	}

	ret = readCompensationDataHeader(type, &(data->header), &address);
	if ( ret < 0) {
		TS_LOG_ERR("%s readGeneralCompensationData: ERROR %02X\n", __func__, ERROR_COMP_DATA_HEADER);
		return ERROR_COMP_DATA_HEADER;
	}

	ret = readGeneralGlobalData(address, data);
	if (ret < 0) {
		TS_LOG_ERR("%s readGeneralCompensationData: ERROR %02X\n", __func__, ERROR_COMP_DATA_GLOBAL);
		return ERROR_COMP_DATA_GLOBAL;
	}

	return OK;

}

int parseProductionTestLimits(char * path, char *label, int **data, int *row, int *column)
{
	int find=0;
	char *token;
	int i = 0;
	int j = 0;
	int z = 0;

	char *line;
	int fd=-1;
	char *buf;
	int n,size,pointer=0;
	char *data_file;
	const struct firmware *fw = NULL;
	struct device * dev = NULL;

	if (path == NULL)
		return 0;
	line = (char *)kmalloc(1800*sizeof(char),GFP_KERNEL);
	if(NULL == line) {
		TS_LOG_ERR("line alloc failed\n");
		return 0;
	}
	buf = line;
	dev = getDev();
	if(dev!=NULL)
		fd = request_firmware(&fw,path,dev);

	if (fd ==0){
		size = fw->size;
		TS_LOG_INFO( "%s The size of the limits file is %d bytes...\n", __func__, size);
		data_file = (char * )fw->data;
		TS_LOG_INFO( "%s Start to reading %s...\n", __func__, path);

		while (find == 0)
		{
			//start to look for the wanted label
			if (readLine(&data_file[pointer],&line,size-pointer,&n) <0){
				find=-1; 
				break;
			}
			pointer+=n;
			//TS_LOG_INFO( "%s Pointer= %d riga = %s \n", __func__, pointer, line);
			if (line[0] == '*') {														//each header row start with *  ex. *label,n_row,n_colum
				buf =line;
				line += 1;
				//TS_LOG_INFO( "%s riga 2 = %s \n", __func__, line);
				token = strsep(&line, ",");
				//TS_LOG_INFO( "%s token = %s label = %s\n", __func__, token, label);
				if (strcmp(token, label) == 0) {										//if the row is the wanted one i retrieve rows and columns info
					find = 1;
					token = strsep(&line, ",");
					//TS_LOG_INFO( "%s riga 3 = %s \n", __func__, line);
					if (token != NULL) {
						sscanf(token, "%d", row);
						TS_LOG_INFO( "%s Row = %d\n", __func__, *row);
					}
					else {
						TS_LOG_ERR( "%s 1: ERROR %02X\n", __func__, ERROR_FILE_PARSE);
						return ERROR_FILE_PARSE;
					}
					token = strsep(&line, ",");
					if (token != NULL) {
						sscanf(token, "%d", column);
						TS_LOG_INFO( "%s Column = %d\n", __func__, *column);
					}
					else {
						TS_LOG_ERR( "%s 2: ERROR %02X\n", __func__, ERROR_FILE_PARSE);
						return ERROR_FILE_PARSE;
					}

					*data = (int *)kmalloc(((*row)*(*column))*sizeof(int), GFP_KERNEL);				//allocate the memory for containing the data
					j = 0;
					if (*data == NULL)
					{
						TS_LOG_ERR( "%s: ERROR %02X\n", __func__, ERROR_ALLOC);
						return ERROR_ALLOC;
					}

					
					//start to read the data 
					for (i = 0; i < *row; i++) {
						line =  buf;
						if (readLine(&data_file[pointer], &line, size-pointer, &n) < 0) {
							TS_LOG_ERR( "%s : ERROR %02X\n", __func__, ERROR_FILE_READ);
							return ERROR_FILE_READ;
						}
						pointer+=n;
						//TS_LOG_INFO( "%s Pointer= %d riga = %s \n", __func__, pointer, line);
						token = strsep(&line, ",");
						for (z = 0; (z < *column) && (token != NULL); z++) {
							//TS_LOG_INFO( "%s token = %s \n", __func__, token);
							//printf("%s \n", token);
							sscanf(token, "%d", ((*data) + j));
							//printf("%d \n", *((*data) + j));
							j++;
							token = strsep(&line, ",");
						}
					}
					if (j == ((*row)*(*column))) {												//check that all the data are read
						TS_LOG_INFO( "%s READ DONE!\n", __func__);
						return OK;
					}
					TS_LOG_ERR( "%s 3: ERROR %02X\n", __func__, ERROR_FILE_PARSE);
					return ERROR_FILE_PARSE;
				}
			}

		}
		TS_LOG_ERR( "%s: ERROR %02X\n", __func__, ERROR_LABEL_NOT_FOUND);
		return ERROR_LABEL_NOT_FOUND;
	}
	else
	{
		TS_LOG_ERR( "%s: ERROR %02X\n", __func__, ERROR_FILE_NOT_FOUND);
		return ERROR_FILE_NOT_FOUND;
	}
}

int readLine(char * data, char ** line, int size, int *n)
{
	int i=0;
	if(size<1)
		return -1;
	
		while(data[i]!='\n' && i<size){
			*(*line + i) = data[i];
			i++;
		}
		*n=i+1;
		*(*line + i) = '\0';

	return OK;
	
}

