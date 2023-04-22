/*

**************************************************************************
**                        STMicroelectronics 		                **
**************************************************************************
**                        marco.cali@st.com				**
**************************************************************************
*                                                                        *
*               FTS functions for getting Initialization Data		 *
*                                                                        *
**************************************************************************
**************************************************************************

*/

#include "ftsCrossCompile.h"
#include "ftsCompensation.h"
#include "ftsError.h"
#include "ftsFrame.h"
#include "ftsHardware.h"
#include "ftsIO.h"
#include "ftsSoftware.h"
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


chipInfo	ftsInfo;

int requestCompensationData(u16 type)
{
	int retry = 0;
	int ret;
	char *temp = NULL;
	u16 answer;

	int event_to_search[3];
	u8 readEvent[FIFO_EVENT_SIZE];

	u8 cmd[3] = { FTS_CMD_REQU_COMP_DATA, 0x00, 0x00 }; // B8 is the command for asking compensation data
	u16ToU8(type, &cmd[1]);

	event_to_search[0] = (int)EVENTID_COMP_DATA_READ;
	event_to_search[1] = cmd[1];
	event_to_search[2] = cmd[2];

	while (retry < COMP_DATA_READ_RETRY) {
		temp =  printHex("Command = ", cmd, 3);
		if(temp!=NULL)
			TS_LOG_INFO("%s %s", __func__,temp);
		kfree(temp);
		ret=fts_writeFwCmd(cmd, 3); //send the request to the chip to load in memory the Compensation Data
		if(ret<OK){
			TS_LOG_ERR("%s requestCompensationData:  ERROR %02X\n", __func__, ERROR_I2C_W);
			return ERROR_I2C_W;
		}
		ret=pollForEvent(event_to_search, 3, readEvent,TIMEOUT_REQU_COMP_DATA);
		if (ret < OK) {
			TS_LOG_INFO("%s Event did not Found at %d attemp!\n", __func__, retry+1);
			retry += 1;
		}
		else
		{
			retry = 0;
			break;
		}
	}


	if (retry == COMP_DATA_READ_RETRY){
		 TS_LOG_ERR("%s requestCompensationData: ERROR %02X\n",__func__, ERROR_TIMEOUT);
		 return ERROR_TIMEOUT;
	}

	u8ToU16_le(&readEvent[1], &answer);

	if(answer == type)
		return OK;
	else {
		TS_LOG_ERR("%s The event found has a different type of Compensation data ERROR %02X\n", __func__, ERROR_DIFF_COMP_TYPE);
		return ERROR_DIFF_COMP_TYPE;
	}

}


int readCompensationDataHeader(u16 type, DataHeader *header, u16 *address)
{

	u16 offset=ADDR_FRAMEBUFFER_DATA;
	u16 answer;
	u8 data[COMP_DATA_HEADER];

	if (readCmdU16(FTS_CMD_FRAMEBUFFER_R, offset, data, COMP_DATA_HEADER, DUMMY_FRAMEBUFFER) < 0) {
		TS_LOG_ERR("%s  readCompensationDataHeader: ERROR %02X\n", __func__, ERROR_I2C_R);
		return ERROR_I2C_R;
	}
	TS_LOG_INFO("%s Read Data Header done!\n",__func__);

	if (data[0] != HEADER_SIGNATURE) {
		TS_LOG_ERR("%s readCompensationDataHeader: ERROR %02X The Header Signature was wrong!  %02X != %02X\n", __func__,
				ERROR_WRONG_COMP_SIGN, data[0],HEADER_SIGNATURE);
		return ERROR_WRONG_COMP_SIGN;
	}

	u8ToU16_le(&data[1], &answer);

	if (answer != type) {
		TS_LOG_ERR("%s readCompensationDataHeader:  ERROR %02X\n", __func__, ERROR_DIFF_COMP_TYPE);
		return ERROR_DIFF_COMP_TYPE;
	}

	TS_LOG_INFO("%s Type of Compensation data OK!\n", __func__);

	header->type = type;
	header->force_node = (int)data[4];
	header->sense_node = (int)data[5];

	*address = offset + COMP_DATA_HEADER;

	return OK;

}

int readMutualSenseGlobalData(u16 *address, MutualSenseData *global)
{
	u8 data[COMP_DATA_GLOBAL];

	TS_LOG_INFO("%s Address for Global data= %02X\n", __func__, *address);

	if (readCmdU16(FTS_CMD_FRAMEBUFFER_R, *address, data, COMP_DATA_GLOBAL, DUMMY_FRAMEBUFFER) < 0) {
		TS_LOG_ERR("%s readMutualSenseGlobalData: ERROR %02X\n", __func__, ERROR_I2C_R);
		return ERROR_I2C_R;
	}
	TS_LOG_DEBUG("%s Global data Read !\n", __func__);

	global->tuning_ver = data[0];
	global->cx1 = data[1];

	TS_LOG_INFO("%s tuning_ver = %d   CX1 = %d\n", __func__, global->tuning_ver, global->cx1);

	*address += COMP_DATA_GLOBAL;
	return OK;

}



int readMutualSenseNodeData(u16 address, MutualSenseData *node)
{
	int size = node->header.force_node*node->header.sense_node;

	TS_LOG_INFO("%s Address for Node data = %02X\n", __func__, address);

	node->node_data = (u8*)kmalloc(size*(sizeof(u8)), GFP_KERNEL);

	if (node->node_data == NULL) {
		TS_LOG_ERR("%s readMutualSenseNodeData: ERROR %02X", __func__, ERROR_ALLOC);
		return ERROR_ALLOC;
	}

	TS_LOG_INFO("%s Node Data to read %d bytes\n", __func__, size);

	if (readCmdU16(FTS_CMD_FRAMEBUFFER_R, address, node->node_data, size, DUMMY_FRAMEBUFFER) < 0) {
		TS_LOG_ERR("%s readMutualSenseNodeData: ERROR %02X\n", __func__, ERROR_I2C_R);
		kfree(node->node_data);
		return ERROR_I2C_R;
	}
	node->node_data_size = size;

	TS_LOG_INFO("%s Read node data ok!\n",__func__);

	return size;

}


int readMutualSenseCompensationData(u16 type, MutualSenseData *data)
{
	int ret;
	u16 address;
	data->node_data = NULL;

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
		TS_LOG_ERR("%s readMutualSenseCompensationData: ERROR %02X\n", __func__, ERROR_COMP_DATA_GLOBAL);
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
	u8 data[COMP_DATA_GLOBAL];

	TS_LOG_INFO("%s Address for Global data= %02X\n", __func__, *address);

	if (readCmdU16(FTS_CMD_FRAMEBUFFER_R, *address, data, COMP_DATA_GLOBAL, DUMMY_FRAMEBUFFER) < 0) {
		TS_LOG_ERR("%s readSelfSenseGlobalData: ERROR %02X\n", __func__, ERROR_I2C_R);
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

	TS_LOG_INFO("%s tuning_ver = %d   f_ix1 = %d   s_ix1 = %d   f_cx1 = %d   s_cx1 = %d\n", __func__, global->tuning_ver, global->f_ix1, global->s_ix1, global->f_cx1, global->s_cx1);
	TS_LOG_INFO("%s max_n = %d   s_max_n = %d\n", __func__, global->f_max_n, global->s_max_n);

	*address += COMP_DATA_GLOBAL;

	return OK;

}

int readSelfSenseNodeData(u16 address, SelfSenseData *node)
{
	int size = node->header.force_node*2+node->header.sense_node*2;
	u8 data[size];

	node->ix2_fm = (u8*)kmalloc(node->header.force_node*(sizeof(u8)), GFP_KERNEL);
	if (node->ix2_fm == NULL) {
		TS_LOG_ERR("%s readSelfSenseNodeData: ERROR %02X", __func__, ERROR_ALLOC);
		return ERROR_ALLOC;
	}

	node->cx2_fm = (u8*)kmalloc(node->header.force_node*(sizeof(u8)), GFP_KERNEL);
	if (node->cx2_fm == NULL) {
		TS_LOG_ERR("%s readSelfSenseNodeData: ERROR %02X", __func__, ERROR_ALLOC);
		kfree(node->ix2_fm);
		return ERROR_ALLOC;
	}
	node->ix2_sn = (u8*)kmalloc(node->header.sense_node*(sizeof(u8)), GFP_KERNEL);
	if (node->ix2_sn == NULL) {
		TS_LOG_ERR("%s readSelfSenseNodeData: ERROR %02X", __func__, ERROR_ALLOC);
		kfree(node->ix2_fm);
		kfree(node->cx2_fm);
		return ERROR_ALLOC;
	}
	node->cx2_sn = (u8*)kmalloc(node->header.sense_node*(sizeof(u8)), GFP_KERNEL);
	if (node->cx2_sn == NULL) {
		TS_LOG_ERR("%s readSelfSenseNodeData: ERROR %02X", __func__, ERROR_ALLOC);
		kfree(node->ix2_fm);
		kfree(node->cx2_fm);
		kfree(node->ix2_sn);
		return ERROR_ALLOC;
	}


	TS_LOG_INFO("%s Address for Node data = %02X\n", __func__, address);

	TS_LOG_INFO("%s Node Data to read %d bytes\n", __func__, size);

	if (readCmdU16(FTS_CMD_FRAMEBUFFER_R, address, data, size, DUMMY_FRAMEBUFFER) < 0) {
		TS_LOG_ERR("%s readSelfSenseNodeData: ERROR %02X\n", __func__, ERROR_I2C_R);
		kfree(node->ix2_fm);
		kfree(node->cx2_fm);
		kfree(node->ix2_sn);
		kfree(node->cx2_sn);
		return ERROR_I2C_R;
	}

	TS_LOG_INFO("%s Read node data ok!\n", __func__);

	memcpy(node->ix2_fm, data, node->header.force_node);
	memcpy(node->ix2_sn, &data[node->header.force_node], node->header.sense_node);
	memcpy(node->cx2_fm, &data[node->header.force_node+ node->header.sense_node], node->header.force_node);
	memcpy(node->cx2_sn, &data[node->header.force_node*2 + node->header.sense_node], node->header.sense_node);

	return OK;

}

int readSelfSenseCompensationData(u16 type, SelfSenseData *data) {

	int ret;
	u16 address;

	data->ix2_fm = NULL;
	data->cx2_fm = NULL;
	data->ix2_sn = NULL;
	data->cx2_sn = NULL;

	if (!(type == SS_TOUCH || type == SS_KEY || type == SS_HOVER || type == SS_PROXIMITY)) {
		TS_LOG_ERR("%s Choose a SS type of compensation data ERROR %02X\n", __func__, ERROR_OP_NOT_ALLOW);
		return ERROR_OP_NOT_ALLOW;
	}

	ret = requestCompensationData(type);
	if ( ret< 0) {
		TS_LOG_ERR("%s : ERROR %02X\n", __func__, ERROR_REQU_COMP_DATA);
		return (ret|ERROR_REQU_COMP_DATA);
	}

	ret = readCompensationDataHeader(type, &(data->header), &address);
	if ( ret < 0) {
		TS_LOG_ERR("%s: ERROR %02X\n", __func__, ERROR_COMP_DATA_HEADER);
		return (ret|ERROR_COMP_DATA_HEADER);
	}

	ret = readSelfSenseGlobalData(&address, data);
	if ( ret < 0) {
		TS_LOG_ERR("%s: ERROR %02X\n", __func__, ERROR_COMP_DATA_GLOBAL);
		return (ret|ERROR_COMP_DATA_GLOBAL);
	}

	ret = readSelfSenseNodeData(address, data);
	if ( ret < 0) {
		TS_LOG_ERR("%s: ERROR %02X\n", __func__, ERROR_COMP_DATA_NODE);
		return (ret|ERROR_COMP_DATA_NODE);
	}

	return OK;

}


int readGeneralGlobalData(u16 address, GeneralData *global)
{
	u8 data[COMP_DATA_GLOBAL];

	if (readCmdU16(FTS_CMD_FRAMEBUFFER_R, address, data, COMP_DATA_GLOBAL, DUMMY_FRAMEBUFFER) < 0) {
		TS_LOG_ERR("%s: ERROR %02X\n", __func__, ERROR_I2C_R);
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
		TS_LOG_ERR("%s: Choose a GENERAL type of compensation data ERROR %02X\n", __func__, ERROR_OP_NOT_ALLOW);
		return ERROR_OP_NOT_ALLOW;
	}

	ret = requestCompensationData(type);
	if ( ret< 0) {
		TS_LOG_ERR("%s: ERROR %02X\n", __func__, ERROR_REQU_COMP_DATA);
		return ERROR_REQU_COMP_DATA;
	}

	ret = readCompensationDataHeader(type, &(data->header), &address);
	if ( ret < 0) {
		TS_LOG_ERR("%s: ERROR %02X\n", __func__, ERROR_COMP_DATA_HEADER);
		return ERROR_COMP_DATA_HEADER;
	}

	ret = readGeneralGlobalData(address, data);
	if (ret < 0) {
		TS_LOG_ERR("%s: ERROR %02X\n", __func__, ERROR_COMP_DATA_GLOBAL);
		return ERROR_COMP_DATA_GLOBAL;
	}

	return OK;

}


int defaultChipInfo(int i2cError)
{
	int i;
	TS_LOG_INFO("%s Setting default Chip Info...\n", __func__);
	ftsInfo.u32_echoEn = 0x00000000;
	ftsInfo.u8_msScrConfigTuneVer = 0;
	ftsInfo.u8_ssTchConfigTuneVer = 0;
	ftsInfo.u8_msScrCxmemTuneVer = 0;
	ftsInfo.u8_ssTchCxmemTuneVer = 0;
	if(i2cError==1){
		ftsInfo.u16_fwVer=0xFFFF;
		ftsInfo.u16_cfgId=0xFFFF;
		for(i=0; i< EXTERNAL_RELEASE_INFO_SIZE; i++){
			ftsInfo.u8_extReleaseInfo[i]=0xFF;
		}
	}else{
		ftsInfo.u16_fwVer=0x0000;
		ftsInfo.u16_cfgId=0x0000;
		for(i=0; i< EXTERNAL_RELEASE_INFO_SIZE; i++){
			ftsInfo.u8_extReleaseInfo[i]=0x00;
		}
	}
	ftsInfo.u32_mpPassFlag = INIT_FIELD;
	ftsInfo.u16_errOffset = INVALID_ERROR_OFFS;
	TS_LOG_INFO("%s default Chip Info DONE!\n", __func__);
	return OK;

}

int readChipInfo(int doRequest)
{
	int ret,i;
	u16 answer;
	u8 data[CHIP_INFO_SIZE+3]; //+3 because need to read all the field of the struct plus the signature and 2 address bytes
	int index = 0;

	TS_LOG_INFO("%s Starting Read Chip Info...\n", __func__);
	if (doRequest == 1) {
		ret = requestCompensationData(CHIP_INFO);
		if (ret < 0) {
			TS_LOG_ERR("%s: ERROR %02X\n", __func__, ERROR_REQU_COMP_DATA);
			ret = (ret | ERROR_REQU_COMP_DATA);
			goto FAIL;
		}
	}

	TS_LOG_INFO("%s Byte to read = %d bytes\n", __func__, CHIP_INFO_SIZE+3);

	if (readCmdU16(FTS_CMD_FRAMEBUFFER_R, ADDR_FRAMEBUFFER_DATA, data, CHIP_INFO_SIZE+3, DUMMY_FRAMEBUFFER) < 0) {
		TS_LOG_ERR("%s: ERROR %02X\n", __func__, ERROR_I2C_R);
		ret = ERROR_I2C_R;
		goto FAIL;
	}

	TS_LOG_INFO("%s Read data ok!\n", __func__);

	TS_LOG_INFO("%s Starting parsing of data...\n", __func__);

	if (data[0] != HEADER_SIGNATURE) {
		TS_LOG_ERR("%s: ERROR %02X The Header Signature was wrong!  %02X != %02X\n", __func__, ERROR_WRONG_COMP_SIGN, data[0], HEADER_SIGNATURE);
		ret = ERROR_WRONG_COMP_SIGN;
		goto FAIL;
	}

	u8ToU16_le(&data[1], &answer);

	if (answer != CHIP_INFO) {
		TS_LOG_ERR("%s:  ERROR %02X\n", __func__, ERROR_DIFF_COMP_TYPE);
		ret = ERROR_DIFF_COMP_TYPE;
		goto FAIL;
	}

	index += 3;
	ftsInfo.u8_loadCnt = data[index++];
	ftsInfo.u8_infoVer = data[index++];
	u8ToU16(&data[index], &ftsInfo.u16_ftsdId);
	index += 2;
	ftsInfo.u8_ftsdVer = data[index++];
	ftsInfo.u8_ftsaId = data[index++];
	ftsInfo.u8_ftsaVer = data[index++];
	ftsInfo.u8_tchRptVer = data[index++];

	TS_LOG_INFO("%s External Release =  ", __func__);
	for (i = 0; i < EXTERNAL_RELEASE_INFO_SIZE;i++) {
		ftsInfo.u8_extReleaseInfo[i] = data[index++];
		TS_LOG_ERR("%02X ", ftsInfo.u8_extReleaseInfo[i]);
	}

	for (i = 0; i < sizeof(ftsInfo.u8_custInfo); i++) {
		ftsInfo.u8_custInfo[i] = data[index++];
	}

	u8ToU16(&data[index], &ftsInfo.u16_fwVer);
	index += 2;
	TS_LOG_INFO("%s FW VERSION = %04X\n", __func__, ftsInfo.u16_fwVer);

	u8ToU16(&data[index], &ftsInfo.u16_cfgId);
	index += 2;
	TS_LOG_INFO("%s CONFIG ID = %04X\n", __func__, ftsInfo.u16_cfgId);

	ftsInfo.u32_projId = ((data[index + 3] & 0x000000FF) << 24) + ((data[index + 2] & 0x000000FF) << 16) + ((data[index + 1] & 0x000000FF) << 8) + (data[index] & 0x000000FF);
	index += 4;

	u8ToU16(&data[index], &ftsInfo.u16_scrXRes);
	index += 2;

	u8ToU16(&data[index], &ftsInfo.u16_scrYRes);
	index += 2;

	ftsInfo.u8_scrForceLen = data[index++];
	TS_LOG_INFO("%s Force Len = %d\n", __func__, ftsInfo.u8_scrForceLen);

	ftsInfo.u8_scrSenseLen = data[index++];
	TS_LOG_INFO("%s Sense Len = %d\n", __func__, ftsInfo.u8_scrSenseLen);

	for (i = 0; i < 8; i++) {
		ftsInfo.u64_scrForceEn[i] = data[index++];
	}

	for (i = 0; i < 8; i++) {
		ftsInfo.u64_scrSenseEn[i] = data[index++];
	}

	ftsInfo.u8_msKeyLen = data[index++];
	TS_LOG_INFO("%s MS Key Len = %d\n", __func__, ftsInfo.u8_msKeyLen);

	for (i = 0; i < 8; i++) {
		ftsInfo.u64_msKeyForceEn[i] = data[index++];
	}

	for (i = 0; i < 8; i++) {
		ftsInfo.u64_msKeySenseEn[i] = data[index++];
	}

	ftsInfo.u8_ssKeyLen = data[index++];
	TS_LOG_INFO("%s SS Key Len = %d\n", __func__, ftsInfo.u8_ssKeyLen);

	for (i = 0; i < 8; i++) {
		ftsInfo.u64_ssKeyForceEn[i] = data[index++];
	}

	for (i = 0; i < 8; i++) {
		ftsInfo.u64_ssKeySenseEn[i] = data[index++];
	}

	ftsInfo.u8_frcTchXLen = data[index++];

	ftsInfo.u8_frcTchYLen = data[index++];

	for (i = 0; i < 8; i++) {
		ftsInfo.u64_frcTchForceEn[i] = data[index++];
	}

	for (i = 0; i < 8; i++) {
		ftsInfo.u64_frcTchSenseEn[i] = data[index++];
	}


	ftsInfo.u8_msScrConfigTuneVer = data[index++];
	TS_LOG_INFO("%s CFG MS TUNING VERSION = %02X\n", __func__, ftsInfo.u8_msScrConfigTuneVer);
	ftsInfo.u8_msScrLpConfigTuneVer = data[index++];
	ftsInfo.u8_msScrHwulpConfigTuneVer = data[index++];
	ftsInfo.u8_msKeyConfigTuneVer = data[index++];
	ftsInfo.u8_ssTchConfigTuneVer = data[index++];
	TS_LOG_INFO("%s CFG SS TUNING VERSION = %02X\n", __func__, ftsInfo.u8_ssTchConfigTuneVer);
	ftsInfo.u8_ssKeyConfigTuneVer = data[index++];
	ftsInfo.u8_ssHvrConfigTuneVer = data[index++];
	ftsInfo.u8_frcTchConfigTuneVer = data[index++];
	ftsInfo.u8_msScrCxmemTuneVer = data[index++];
	TS_LOG_INFO("%s CX MS TUNING VERSION = %02X\n", __func__, ftsInfo.u8_msScrCxmemTuneVer);
	ftsInfo.u8_msScrLpCxmemTuneVer = data[index++];
	ftsInfo.u8_msScrHwulpCxmemTuneVer = data[index++];
	ftsInfo.u8_msKeyCxmemTuneVer = data[index++];
	ftsInfo.u8_ssTchCxmemTuneVer = data[index++];
	TS_LOG_INFO("%s CX SS TUNING VERSION = %02X\n", __func__, ftsInfo.u8_ssTchCxmemTuneVer);
	ftsInfo.u8_ssKeyCxmemTuneVer = data[index++];
	ftsInfo.u8_ssHvrCxmemTuneVer = data[index++];
	ftsInfo.u8_frcTchCxmemTuneVer = data[index++];
	ftsInfo.u32_mpPassFlag = ((data[index + 3] & 0x000000FF) << 24) + ((data[index + 2] & 0x000000FF) << 16) + ((data[index + 1] & 0x000000FF) << 8) + (data[index] & 0x000000FF);
	index += 4;
	TS_LOG_INFO("%s MP SIGNATURE = %08X\n", __func__, ftsInfo.u32_mpPassFlag);
	ftsInfo.u32_featEn = ((data[index + 3] & 0x000000FF) << 24) + ((data[index + 2] & 0x000000FF) << 16) + ((data[index + 1] & 0x000000FF) << 8) + (data[index] & 0x000000FF);
	index += 4;
	ftsInfo.u32_echoEn = ((data[index + 3] & 0x000000FF) << 24) + ((data[index + 2] & 0x000000FF) << 16) + ((data[index + 1] & 0x000000FF) << 8) + (data[index] & 0x000000FF);
	index += 4;
	TS_LOG_INFO("%s FEATURES = %08X\n", __func__, ftsInfo.u32_echoEn);
	ftsInfo.u8_sideTchConfigTuneVer = data[index++];
	ftsInfo.u8_sideTchCxmemTuneVer = data[index++];
	ftsInfo.u8_sideTchForceLen = data[index++];
	TS_LOG_INFO("%s Side Touch Force Len = %d\n", __func__, ftsInfo.u8_sideTchForceLen);
	ftsInfo.u8_sideTchSenseLen = data[index++];
	TS_LOG_INFO("%s Side Touch Sense Len = %d\n", __func__, ftsInfo.u8_sideTchSenseLen);
	for (i = 0; i < 8; i++) {
		ftsInfo.u64_sideTchForceEn[i] = data[index++];
	}
	for (i = 0; i < 8; i++) {
		ftsInfo.u64_sideTchSenseEn[i] = data[index++];
	}
	ftsInfo.u8_errSign=data[index++];
	TS_LOG_INFO("%s ERROR SIGNATURE = %02X\n", __func__, ftsInfo.u8_errSign);
	if(ftsInfo.u8_errSign==ERROR_SIGN_HEAD){
		TS_LOG_INFO("%s Correct Error Signature found!\n", __func__);
		u8ToU16(&data[index], &ftsInfo.u16_errOffset);
	}else{
		TS_LOG_ERR("%s Error Signature NOT FOUND!\n", __func__);
		ftsInfo.u16_errOffset = INVALID_ERROR_OFFS;
	}
	TS_LOG_INFO("%s ERROR OFFSET = %04X\n", __func__, ftsInfo.u16_errOffset);
	index += 2;
	TS_LOG_INFO("%s Parsed %d bytes!\n", __func__, index);


	if (index != CHIP_INFO_SIZE + 3) {
		TS_LOG_ERR("%s readChipInfo: index = %d different from %d ERROR %02X\n", __func__, index, CHIP_INFO_SIZE+3,ERROR_OP_NOT_ALLOW);
		return ERROR_OP_NOT_ALLOW;
	}

	TS_LOG_INFO("%s Chip Info Read DONE!\n", __func__);
	return OK;

FAIL:
	defaultChipInfo(isI2cError(ret));
	return ret;

}

