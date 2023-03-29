/*
 * File : wacom_update.c
 *
 *********************************************************************
	This is not a free source file, all content are protected by NDA
 *********************************************************************
*/
// Martin Chen, Original from user space flash program
#include "wacom.h"
#include "wac_flash.h"


#define DATA_ID_FC_OFFSET_VALUE 1
#define DATA_ID_FD_OFFSET_VALUE 2
#define DATA_ID_FB_OFFSET_VALUE 23

#define FW_VERSION_MSB_OFFSET 3
#define FW_VERSION_LSB_OFFSET 4
#define PID_MSB_OFFSET 5
#define PID_LSB_OFFSET 6
#define VID_MSB_OFFSET 7
#define VID_LSB_OFFSET 8

#define EXTRA_FW_VERSION_MSB_OFFSET 6
#define EXTRA_FW_VERSION_LSB_OFFSET 5
#define EXTRA_PID_MSB_OFFSET 4
#define EXTRA_PID_LSB_OFFSET 3
#define EXTRA_VID_MSB_OFFSET 2
#define EXTRA_VID_LSB_OFFSET 1

#define VID_MSB_VALUE 0x2D
#define VID_LSB_VALUE 0x1F

#define FW_BINDATA_ID3_OFFSET 1
#define FW_BINDATA_ID2_OFFSET 2
#define FW_BINDATA_ID1_OFFSET 3
#define DATA_ID_OFFSET 4

#define COUNT_BASE 0x3FF
#define NEWCOUNT_BASE  10

u16 wc_dev_pid = 0;
extern struct wacom_i2c *wac_data;


/*ok, return 0; else return negative number.*/
int wacom_iowrite_more(u8 *data_buf, u16 buf_size)
{
	int ret = NO_ERR;
	struct i2c_adapter *adap = wac_data->client->adapter;
	struct i2c_msg msg[TWO_MESSAGE];
	struct i2c_msg *xfer = NULL;
	u16 ts_len = 0;
	int count = 0;

	if( (!data_buf) || (!wac_data ) || (!wac_data->client) )
	{
		TS_LOG_ERR("%s : wacom data is null\n", __func__);
		return -EINVAL;
	}

	memset(msg, 0, sizeof(msg));

	if(buf_size > WACOM_I2C_RW_MAX_SIZE) {
		ts_len = WACOM_I2C_RW_MAX_SIZE;
		xfer = &msg[0];
		count = TWO_MESSAGE;
	} else {
		xfer = &msg[0];
		ts_len = buf_size;
		count = ONE_MESSAGE;
	}

	wac_data->client->flags = 0;
	msg[0].addr = wac_data->client->addr;
	msg[0].flags = 0;
	msg[0].len = ts_len;
	msg[0].buf = (char *)data_buf;

	if(buf_size > WACOM_I2C_RW_MAX_SIZE) {
		msg[1].addr = wac_data->client->addr;
		msg[1].flags = I2C_M_NOSTART;
		msg[1].len = buf_size - ts_len;
		msg[1].buf = (char *)(data_buf + ts_len);
	}

	ret = i2c_transfer(adap, xfer, count);
	if(ret == count){
		ret = NO_ERR;
	} else {
		TS_LOG_ERR("%s,fail write rc=%d\n", __func__, ret);
		ret = -EIO;

		ts_dmd_report(DSM_TP_I2C_RW_ERROR_NO, "irq_gpio:%d value:%d reset_gpio:%d  value:%d. I2C_status:%d;addr:%d.\n",
                g_ts_kit_platform_data.irq_gpio, gpio_get_value(g_ts_kit_platform_data.irq_gpio),\
                g_ts_kit_platform_data.reset_gpio, gpio_get_value(g_ts_kit_platform_data.reset_gpio), \
                g_ts_kit_platform_data.dsm_info.constraints_I2C_status, wac_data->client->addr);
	}

	// if OK, return length
	return ret;
}

//
//==========================================================================
//	Device descriptor report operation code
//==========================================================================
//
int wacom_gather_info(u16 *pid, u16 *fw_version)
{
	u8 cmd[] = {HID_DESC_REGISTER, 0x00};
	int ret = NO_ERR;
	HID_DESC wc_hid_descriptor;
	struct wacom_features *features = NULL;
	u8 reg_addr = WACOM_REG_BASE;

	if ((!pid) || (!fw_version) ) {
		TS_LOG_ERR("date is null\n");
		ret = -EINVAL;
		return ret;
	}

	memset(&wc_hid_descriptor, 0, sizeof(HID_DESC));

	features = (struct wacom_features *)kzalloc(sizeof(struct wacom_features), GFP_KERNEL);
	if (features == NULL) {
		TS_LOG_ERR("No memory space available for this \n");
		ret = WACOM_ERR_NOMEM;
		goto gainfo_out;
	}

	ret = g_ts_kit_platform_data.bops->bus_write(cmd, sizeof(cmd));
	if (ret != NO_ERR) {
		TS_LOG_ERR("%s obtaining query failed: %d \n", __func__, ret);
		ret = WACOM_ERR_DEV_WRITE;
		goto gainfo_err;
	}

	ret = g_ts_kit_platform_data.bops->bus_read(&reg_addr, INVALID_REG_LENGTH, &wc_hid_descriptor, sizeof(HID_DESC));
	if (ret != NO_ERR) {
		TS_LOG_ERR("%s obtaining hid descriptor failed: %d \n", __func__, ret);
		ret = WACOM_ERR_DEV_READ;
		goto gainfo_err;
	}

	*pid = wc_hid_descriptor.wProductID;
	*fw_version = (u16)wc_hid_descriptor.wVersion;
	TS_LOG_INFO("wc_hid_descriptor.wHIDDescLength:0x%04x\n", wc_hid_descriptor.wHIDDescLength);
	TS_LOG_INFO("wc_hid_descriptor.bcdVersion:0x%04x\n", wc_hid_descriptor.bcdVersion);
	TS_LOG_INFO("wc_hid_descriptor.wReportDescLength:0x%04x\n", wc_hid_descriptor.wReportDescLength);
	TS_LOG_INFO("wc_hid_descriptor.wReportDescRegister:0x%04x\n", wc_hid_descriptor.wReportDescRegister);
	TS_LOG_INFO("wc_hid_descriptor.wInputRegister:0x%04x\n", wc_hid_descriptor.wInputRegister);
	TS_LOG_INFO("wc_hid_descriptor.wMaxInputLength:0x%04x\n", wc_hid_descriptor.wMaxInputLength);
	TS_LOG_INFO("wc_hid_descriptor.wOutputRegister:0x%04x\n", wc_hid_descriptor.wOutputRegister);
	TS_LOG_INFO("wc_hid_descriptor.wCommandRegister:0x%04x\n", wc_hid_descriptor.wCommandRegister);
	TS_LOG_INFO("wc_hid_descriptor.wDataRegister:0x%04x\n", wc_hid_descriptor.wDataRegister);
	TS_LOG_INFO("wc_hid_descriptor.wVendorID:0x%04x\n", wc_hid_descriptor.wVendorID);
	TS_LOG_INFO("wc_hid_descriptor.wProductID:0x%04x\n", wc_hid_descriptor.wProductID);
	TS_LOG_INFO("wc_hid_descriptor.wVersion:0x%04x\n", wc_hid_descriptor.wVersion);
	TS_LOG_INFO("wc_hid_descriptor.RESERVED_HIGH:0x%04x\n", wc_hid_descriptor.RESERVED_HIGH);
	TS_LOG_INFO("wc_hid_descriptor.RESERVED_LOW:0x%04x\n", wc_hid_descriptor.RESERVED_LOW);
	ret = NO_ERR;

 gainfo_err:
	kfree(features);
	features = NULL;

 gainfo_out:
	return ret;
}

int wacom_get_fw_version(unsigned int *pid, unsigned int *fw_version)
{
	int ret = NO_ERR;
	u16 dev_pid = 0;
	u16 dev_fw_version = 0;

	if ((!pid) || (!fw_version) ) {
		TS_LOG_ERR("date is null\n");
		ret = -EINVAL;
		return ret;
	}

	ret = wacom_gather_info(&dev_pid, &dev_fw_version);
	if(ret != NO_ERR){
		TS_LOG_ERR("%s wacom_gather_info error, %d\n", __func__, ret);
	}

	*pid = dev_pid;
	*fw_version = dev_fw_version;

	return ret;
}


//
//==========================================================================
//	Feature report operation functions
//==========================================================================
//
// UBL_G11T_GetFeature  ok return 0 , fail return minus number.
int UBL_G11T_GetFeature(u8 report_id, u16 buf_size, u8 *recv_data)
{
	int ret = NO_ERR, i = 0;
	u8 gFeature[GFEATURE_SIZE];
	u8 *recv = NULL;
	u16 cmd_reg = 0;
	u16 data_reg = 0;

	if ((!recv_data) || (!wac_data) || (!wac_data->features) || (!wac_data->client)) {
		TS_LOG_ERR("date is null\n");
		ret = -EINVAL;
		return ret;
	}

	cmd_reg = wac_data->features->hid_desc.wCommandRegister;
	data_reg = wac_data->features->hid_desc.wDataRegister;

	/*"+ 2", adding 2 stores i2c data length*/
	recv = (u8 *)kzalloc(sizeof(u8) * (buf_size + DATA_LENGTH_SIZE), GFP_KERNEL);
	if (recv == NULL) {
		TS_LOG_ERR("%s cannot preserve memory \n", __func__);
		ret = WACOM_ERR_NOMEM;
		goto out_Getfeature;
	}
	memset(recv, 0, sizeof(u8) * (buf_size + DATA_LENGTH_SIZE));

	gFeature[i++] = (u8)(cmd_reg & 0x00ff);
	gFeature[i++] = (u8)((cmd_reg & 0xff00) >> 8);
	gFeature[i++] = (RTYPE_FEATURE << 4) | report_id;//type  occupy 4 bit
	gFeature[i++] = CMD_GET_FEATURE;
	gFeature[i++] = (u8)(data_reg & 0x00ff);
	gFeature[i++] = (u8)((data_reg & 0xff00) >> 8);

	ret = g_ts_kit_platform_data.bops->bus_read((char *)gFeature, GFEATURE_SIZE, (char *)recv, buf_size+DATA_LENGTH_SIZE);
	if (ret != NO_ERR) {
	//if (ret != TWO_MESSAGE) {
		TS_LOG_ERR("%s Receiving data failed; recieved bytes: %d \n", __func__, ret);
		ret = WACOM_ERR_UBL_GETFEATURE;
		goto err_Getfeature;
	}

	memcpy(recv_data, (recv + DATA_LENGTH_SIZE), buf_size);

 err_Getfeature:
	kfree(recv);
	recv = NULL;

 out_Getfeature:
	return ret;
}

// UBL_G11T_SetFeature //ok return 0 , fail return minus number.
int UBL_G11T_SetFeature(u8 report_id, u16 buf_size, u8 *data)

{
	int i=0, ret = NO_ERR;
	int total = SFEATURE_SIZE + buf_size;
	u8 *sFeature = NULL;
	u16 cmd_reg = 0;
	u16 data_reg = 0;

	if ((!data) || (!wac_data) || (!wac_data->features)) {
		TS_LOG_ERR("date is null\n");
		ret = -EINVAL;
		return ret;
	}

	cmd_reg = wac_data->features->hid_desc.wCommandRegister;
	data_reg = wac_data->features->hid_desc.wDataRegister;

	sFeature = (u8 *)kzalloc(sizeof(u8) * total, GFP_KERNEL);
	if (sFeature == NULL) {
		TS_LOG_ERR("%s cannot preserve memory \n", __func__);
		ret = WACOM_ERR_NOMEM;
		goto out_Setfeature;
	}
	memset(sFeature, 0, sizeof(u8) * total);

	sFeature[i++] = (u8)(cmd_reg & 0x00ff);
	sFeature[i++] = (u8)((cmd_reg & 0xff00) >> 8);
	sFeature[i++] = (RTYPE_FEATURE << 4) | report_id;//type  occupy 4 bit
	sFeature[i++] = CMD_SET_FEATURE;
	sFeature[i++] = (u8)(data_reg & 0x00ff);
	sFeature[i++] = (u8)((data_reg & 0xff00) >> 8);
	sFeature[i++] = (u8)((buf_size + DATA_LENGTH_SIZE) & 0x00ff);
	sFeature[i++] = (u8)(( (buf_size + DATA_LENGTH_SIZE) & 0xff00) >> 8);

	for (i = 0; i < buf_size; i++)
		sFeature[i + SFEATURE_SIZE] = *(data + i);

	ret = wacom_iowrite_more(sFeature, total);
	if (ret != NO_ERR) {
		TS_LOG_ERR("Sending Set_Feature failed sent bytes: %d \n", ret);
		ret = WACOM_ERR_UBL_SETFEATURE;
	}
	kfree(sFeature);
	sFeature = NULL;

 out_Setfeature:
	return ret;
}


//
//==========================================================================
//	UBL operation functions
//==========================================================================
//
// UBL_G11T_SendCommand  //ok return 0 , fail return minus number.
int UBL_G11T_SendCommand(boot_cmd *command, boot_rsp *response)
{
	int ret = UBL_OK;
	unsigned int i=0;

	if ((!command) || (!response)) {
		TS_LOG_ERR("date is null\n");
		ret = -EINVAL;
		return ret;
	}

	//TS_LOG_INFO("\n----------------------------\n");
	//TS_LOG_INFO("@%s \n", __func__);
	//TS_LOG_INFO("Send UBL command:0x%x \n", command->header.cmd);

	command->header.reportId = UBL_CMD_REPORT_ID;
	response->header.reportId = UBL_RSP_REPORT_ID;

	// Set feature here
	ret = UBL_G11T_SetFeature(UBL_CMD_REPORT_ID, UBL_CMD_SIZE_G11T, command->data);
	if ( ret != UBL_OK ) {
		if ( command->header.cmd == UBL_COM_EXIT ) { // Exit UBL, so don't care
			ret = WACOM_ERR_UBL_COMMAND;// set to OK ?
			goto g11t_cmd_out;
		}
		TS_LOG_ERR("%s failed \n", __func__);
		goto g11t_cmd_out;
	}

	if ( command->header.cmd != UBL_COM_EXIT ) {// Other UBL command exclude Exit UBL
		for ( i = 0; i < UBL_TIMEOUT_WRITE; i++ ) {
			// Get feature here
			ret = UBL_G11T_GetFeature(UBL_RSP_REPORT_ID, UBL_RSP_SIZE_G11T, response->data);
			if (  ret != UBL_OK  ) {
				TS_LOG_ERR("GET ERROR at command:%d \n", command->header.cmd);
				goto g11t_cmd_out;
			}
			//TS_LOG_INFO("Get Feature. response:0x%X \n", response->header.resp);

			//TS_LOG_INFO("Get Feature. cmd header.cmd:0x%X, command->header.echo:0x%x\n", command->header.cmd, command->header.echo);
			//TS_LOG_INFO("Get Feature. response header.cmd:0x%X, response->header.echo:0x%x\n", response->header.cmd, response->header.echo);
			if ( (command->header.cmd != response->header.cmd) || (command->header.echo != response->header.echo) ) {
				TS_LOG_ERR("RESPONSE not match at command:%d \n", command->header.cmd );
				ret = WACOM_ERR_UBL_COMMAND;
				goto g11t_cmd_out;
			}

			if ( response->header.resp != UBL_RES_BUSY ) {
				// other than returned value is UBL_RES_BUSY, quit
				break;
			}
			msleep(10);// wait 10 ms
		}// wait unti device processes the command

		if ( i == UBL_TIMEOUT_WRITE ) {
			TS_LOG_ERR("TIMEOUT at command:%d \n", command->header.cmd );
			ret = WACOM_ERR_UBL_TIMEOUT;
			goto g11t_cmd_out;
		}

		// Command that generally returns "OK" response is treated as an error
		if ( ((command->header.cmd == UBL_COM_WRITE ) || (command->header.cmd == UBL_COM_ALLERASE))
			&& (response->header.resp != UBL_RES_OK) ){
			TS_LOG_ERR("Error response:%d at command:%d \n", response->header.resp, command->header.cmd );
			ret = WACOM_ERR_UBL_COMMAND;
			goto g11t_cmd_out;
		}
	}
g11t_cmd_out:
	return ret;
}

// Switch to bootloader //ok return 0 , fail return minus number.
int EnterG11TUBL(void)
{
	int ret = UBL_OK;
	u8 data[2];// 2 byte data

	//TS_LOG_INFO("\n----------------------------\n");
	//TS_LOG_INFO("@Enter UBL. WWWWW\n");
	data[0] = DEVICETYPE_REPORT_ID;
	data[1] = DEVICETYPE_UBL;

	ret = UBL_G11T_SetFeature(DEVICETYPE_REPORT_ID, 2, data); // 2 byte data
	if ( ret != UBL_OK ) {
		TS_LOG_ERR("%s SET ERROR at EnterUBL. \n\n", __func__);
	}
	else {
		msleep(500);// wait 500 ms
	}
	return ret;
}

// Switch back to normal device
int ExitG11TUBL(void)
{
	int ret = UBL_OK;
	boot_cmd command;
	boot_rsp response;

	TS_LOG_INFO("\n----------------------------\n");
	TS_LOG_INFO("@%s \n", __func__);
	memset(&command, 0, sizeof(boot_cmd));
	memset(&response, 0, sizeof(boot_rsp));

	command.header.cmd = UBL_COM_EXIT;
	ret = UBL_G11T_SendCommand(&command, &response);
	if ( ret != UBL_OK ) {
		TS_LOG_ERR("%s Exiting failed \n\n", __func__);
	}
	else {
		msleep(500);// wait 500 ms
	}
	return ret;
}

// Check if switched to bootloader
// UBL_G11T_Check_Mode //ok return 0 , fail return minus number.
int UBL_G11T_Check_Mode(void)
{
	int ret = UBL_OK;
	boot_cmd command;
	boot_rsp response;

	TS_LOG_INFO("\n----------------------------\n");
	TS_LOG_INFO("@%s \n", __func__);
	memset(&command, 0, sizeof(boot_cmd));
	memset(&response, 0, sizeof(boot_rsp));

	command.header.cmd = UBL_COM_CHECKMODE;
	ret = UBL_G11T_SendCommand(&command, &response);
	if ( ret != UBL_OK ) {
		TS_LOG_ERR("%s Sending command failed \n\n", __func__);
		return ret;
	}
	if ( response.header.resp != UBL_G11T_MODE_UBL ) {
		TS_LOG_ERR("%s Not in UBL mode. MODE:%d \n\n", __func__, response.header.resp );
		ret = WACOM_ERR_UBL_COMMAND;
	}

	return ret;
}


// UBL_G11T_Check_Data //ok return 0 , fail return minus number.
int UBL_G11T_Check_Data(UBL_PROCESS *pUBLProcess)
{
	int ret = UBL_OK;

	if (!pUBLProcess) {
		TS_LOG_ERR("date is null\n");
		ret = -EINVAL;
		return ret;
	}

	TS_LOG_DEBUG("\n----------------------------\n");
	TS_LOG_DEBUG("@%s \n", __func__);

	// Checking start address
	if ( pUBLProcess->start_adrs != UBL_MAIN_ADDRESS ) {
		TS_LOG_ERR("%s failed \n", __func__);
		TS_LOG_ERR("Data error. Start at 0x%05x.\n", (u16)pUBLProcess->start_adrs );
		ret = WACOM_ERR_FW_FILE_ADDR;
		return ret;
	}

	// checking the total firmware size
	if ( (pUBLProcess->start_adrs + pUBLProcess->size) > UBL_MAIN_SIZE_PLUS_ONE ) {
		TS_LOG_ERR("%s failed \n", __func__);
		TS_LOG_ERR("Data size error. Size is 0x%05x. \n", (u16)pUBLProcess->size );
		ret = WACOM_ERR_FW_FILE_SIZE;
		return ret;
	}

	return UBL_OK;
}

// UBL_G11T_EraseAll //ok return 0 , fail return minus number.
int UBL_G11T_EraseAll(UBL_PROCESS *pUBLProcess)
{
	int ret = UBL_OK;
	boot_cmd command;
	boot_rsp response;
	int count = 3;//try 3 times

	if (!pUBLProcess) {
		TS_LOG_ERR("date is null\n");
		ret = -EINVAL;
		return ret;
	}

	TS_LOG_DEBUG("\n----------------------------\n");
	TS_LOG_DEBUG("@%s \n", __func__);
	memset(&command, 0, sizeof(boot_cmd));
	memset(&response, 0, sizeof(boot_rsp));

	command.erase_flash.reportId = UBL_CMD_REPORT_ID;
	command.erase_flash.cmd = UBL_COM_ALLERASE;	// erase all flash
	command.erase_flash.echo = 1;
	command.erase_flash.blkNo = 0;

	// Set feature here
	ret = UBL_G11T_SetFeature(UBL_CMD_REPORT_ID, UBL_CMD_SIZE_G11T, command.data);
	if ( ret != UBL_OK ) {
		TS_LOG_ERR("%s failed \n", __func__);
		goto err_GTErase;
	}
	msleep(500);// origin wait 2 seconds, becasue it erase all flash need some time. maybe 500ms is ok.

	response.erase_flash.reportId = UBL_RSP_REPORT_ID;
	response.header.resp = UBL_RES_BUSY;	// busy

	while((ret == UBL_OK) && (response.header.resp == UBL_RES_BUSY)) {
		// Get feature here
		if(count <=0)
		{
			break;
		}
		ret = UBL_G11T_GetFeature(UBL_RSP_REPORT_ID, UBL_RSP_SIZE_G11T, response.data);
		msleep(500);//delay 500 ms every try
		count--;
	}
	if ( (ret != UBL_OK ) || (count ==0) ) {
		TS_LOG_ERR("%s failed \n", __func__);
		goto err_GTErase;
	}

	if(response.header.resp != UBL_RES_OK) {
		TS_LOG_ERR("%s failed \n", __func__);
		goto err_GTErase;
	}
	return UBL_OK;

err_GTErase:
	ret = WACOM_ERR_ERASE_FAIL;
	return ret;
}

// UBL_G11T_SendData //ok return 0 , fail return minus number.
int UBL_G11T_SendData(unsigned char cmd, unsigned char *data, unsigned long start_adrs, unsigned long size, UBL_STATUS *pUBLStatus)
{
	unsigned int i, j;
	int ret = UBL_OK;
	boot_cmd command;
	boot_rsp response;
	u8 command_id = 0;

	if ((!data) || (!pUBLStatus)) {
		TS_LOG_ERR("date is null\n");
		ret = -EINVAL;
		return ret;
	}

	// g_FlashBlockSize - global variable containing size of one data block in read/write report
	for (i = 0; i < (UBL_MAIN_SIZE_PLUS_ONE/UBL_G11T_CMD_DATA_SIZE); i++)
	{
		if ( i * UBL_G11T_CMD_DATA_SIZE >= size ) {
			break;
		}
		if(i % 100 == 0) {//100 times print log
			TS_LOG_INFO("%s, write %d times\n", __func__, i);
		}
		memset(&command, 0, sizeof(boot_cmd));
		memset(&response, 0, sizeof(boot_rsp));

		pUBLStatus->progress = i * UBL_G11T_CMD_DATA_SIZE;
		command.write_flash.reportId = UBL_CMD_REPORT_ID;
		command.write_flash.cmd = cmd;
		command.write_flash.echo = ++command_id;//packed struct member, don't modify !
		command.write_flash.addr = (u32)(start_adrs + i * UBL_G11T_CMD_DATA_SIZE);
		command.write_flash.size8 = UBL_G11T_CMD_DATA_SIZE / 8;//IC defined, must divided 8.

		// Copy UBL_G11T_CMD_DATA_SIZE bytes to report
		for ( j = 0; j < UBL_G11T_CMD_DATA_SIZE; j++ ) {
			command.write_flash.data[j] = *(data + i * UBL_G11T_CMD_DATA_SIZE + j);//memcpy
		}

		// Set feature here
		ret = UBL_G11T_SetFeature(UBL_CMD_REPORT_ID, UBL_CMD_SIZE_G11T, command.data);
		if ( ret != UBL_OK ) {
			ret = WACOM_ERR_UBL_ERROR;
			TS_LOG_ERR("SET ERROR at block:%d \n", i);
			return ret;
		}

		response.write_flash.reportId = UBL_RSP_REPORT_ID;
		for ( j = 0; j < UBL_TIMEOUT_WRITE; j++ )
		{
			// Get feature here
			ret = UBL_G11T_GetFeature(UBL_RSP_REPORT_ID, UBL_RSP_SIZE_G11T, response.data);
			if ( ret != UBL_OK ) {
				ret = WACOM_ERR_UBL_ERROR;
				TS_LOG_ERR("GET ERROR at block:%d \n", i);
				return ret;

			}
			if ( (command.header.cmd != response.header.cmd) || (command.header.echo != response.header.echo) ) {
				ret = WACOM_ERR_UBL_ERROR;
				TS_LOG_ERR("RESPONSE not match at block:%d \n", i);
				return ret;
			}

			if ( (response.header.resp != UBL_RES_OK) && (response.header.resp != UBL_RES_BUSY) ) {
				pUBLStatus->response = response.header.resp;
				ret = WACOM_ERR_UBL_ERROR;

				switch ( response.header.resp ) {
					case UBL_RES_PID_ERROR:
						TS_LOG_ERR("File error. PID not match. \n" );
						break;
					case UBL_RES_VERSION_ERROR:
						TS_LOG_ERR("File error. VERSION is older. \n" );
						break;

					default:
						TS_LOG_ERR("Error response@SendData:%d at block:%d \n", response.header.resp, i );
						break;
				}
				return ret;
			}
			if ( response.header.resp == UBL_RES_OK ) {
				break;
			}
			msleep(10);// wait 10 ms
		} // End for j

		if ( j == UBL_TIMEOUT_WRITE ) {
			ret = WACOM_ERR_UBL_ERROR;
			TS_LOG_ERR("TIMEOUT at block:%d \n", i );
			return ret;
		}
	} // End for i

	return ret;
}

// UBL_G11T_Write
int UBL_G11T_Write(UBL_PROCESS *pUBLProcess, UBL_STATUS *pUBLStatus)
{
	int ret = UBL_OK;

	if ((!pUBLProcess) || (!pUBLStatus)) {
		TS_LOG_ERR("date is null\n");
		ret = -EINVAL;
		return ret;
	}

	TS_LOG_DEBUG("\n----------------------------\n");
	TS_LOG_DEBUG("@%s \n", __func__);

	ret = UBL_G11T_Check_Data(pUBLProcess);
	if ( ret != UBL_OK ) {
		TS_LOG_ERR("%s failed \n", __func__);
		return ret;
	}

	// Erasing
	TS_LOG_DEBUG("Erasing flash. \n");
	ret = UBL_G11T_EraseAll(pUBLProcess);
	if ( ret != UBL_OK ) {
		TS_LOG_ERR("%s failed \n", __func__);
		return ret;
	}

	// Writing firmware
	TS_LOG_DEBUG("Write start. \n");
	TS_LOG_INFO("WRITE address:0x%x size:0x%x\n", (u16)pUBLProcess->start_adrs, (u16)pUBLProcess->size);

	ret = UBL_G11T_SendData(UBL_COM_WRITE, pUBLProcess->data, pUBLProcess->start_adrs, pUBLProcess->size, pUBLStatus );
	if ( ret != UBL_OK ) {
		TS_LOG_ERR("%s failed \n", __func__);
		return ret;
	}
	TS_LOG_INFO("%s, write success\n", __func__);
	return ret;
}


bool find_pid_version_in_bindata(UBL_PROCESS *pUBLProcess)
{
	bool found = false;
	u32 count = 0, new_count = 0,chk_size = 0;
	u8 *data_ptr = NULL;
	u8 *data_id = 0;
	u8 id = 0;
	int offset = 1;

	if ((!pUBLProcess) || (!pUBLProcess->data)) {
		TS_LOG_ERR("date is null\n");
		return false;
	}

	if ( (pUBLProcess->size < 10) || (pUBLProcess->size > UBL_ROM_SIZE )) {//late 10 bytes contains no ID
		TS_LOG_ERR("date size : %x  is invalid\n", pUBLProcess->size);
		return false;
	}

	chk_size = pUBLProcess->size - 10;//late 10 bytes contains no ID
	data_ptr = pUBLProcess->data + pUBLProcess->size-1;

	TS_LOG_INFO("%s:  chk_size is %x,  data_ptr data is %x\n", __func__, chk_size, *data_ptr);

	/*
	--------------------------------------------------------------------
	VID(2 bytes) | PID(2 bytes) | VERSION(1 or 2 bytes) | FC/FD | 00 | 00 | 0F | D9
	0XFC:  VERSION is 1 bytes
	0XFD:  VERSION is 2 bytes
	--------------------------------------------------------------------
	*/
	while (count < chk_size ) {
		if (*(data_ptr - count) == FW_BINDATA_ID4) {
			data_id = data_ptr - count;// current is 0xD9

			if((count+DATA_ID_OFFSET) >= chk_size ){
				TS_LOG_ERR("%s, %d, current count is %x, can't continue find pid. \n", __func__, __LINE__, count);
				break;
			}

			//find 00 00 0F D9.  1, 2, 3, 4, 5, 6, 7, 8 means byte offset from 0XD9(data_id).
			if ((*(data_id - FW_BINDATA_ID3_OFFSET) == FW_BINDATA_ID3) && (*(data_id - FW_BINDATA_ID2_OFFSET) == FW_BINDATA_ID2) 
				&& (*(data_id - FW_BINDATA_ID1_OFFSET) == FW_BINDATA_ID1)) {
				id = *(data_id - DATA_ID_OFFSET);
				if (id == FW_VERSION_ONE_BYTE) {
					offset = DATA_ID_FC_OFFSET_VALUE;// version need 1 bytes to store
					if((count+offset+FW_VERSION_LSB_OFFSET) >= chk_size ){
						TS_LOG_ERR("%s, %d, current count is %x, can't continue find one byte pid. \n", __func__, __LINE__, count);
						break;
					}
					pUBLProcess->version = *(data_id - FW_VERSION_LSB_OFFSET - offset);
				}
				else if (id == FW_VERSION_TWO_BYTE) {
					offset = DATA_ID_FD_OFFSET_VALUE;// version need 2 bytes to store
					if((count+offset+FW_VERSION_LSB_OFFSET) >= chk_size ){
						TS_LOG_ERR("%s, %d current count is %x, %d, can't continue find two byte pid. \n", __func__, __LINE__, count);
						break;
					}
					pUBLProcess->version = (*(data_id - FW_VERSION_MSB_OFFSET - offset)) << 8 + *(data_id - FW_VERSION_LSB_OFFSET - offset);
				}
				else if (id == FW_VERSION_MORE_BYTE) {
					offset = DATA_ID_FB_OFFSET_VALUE;// version need 2 bytes to store
					if((count+offset+FW_VERSION_LSB_OFFSET) >= chk_size ){
						TS_LOG_ERR("%s, %d current count is %x, can't continue find more byte pid. \n", __func__, __LINE__, count);
						break;
					}
					pUBLProcess->version = (*(data_id - FW_VERSION_MSB_OFFSET - offset)) << 8 + *(data_id - FW_VERSION_LSB_OFFSET - offset);
				}
				else {// Try another pattern "FA000C"
					goto check_pattern_FAC;
				}

				if((count+offset+VID_LSB_OFFSET) >= chk_size ){
					TS_LOG_ERR("%s, %d current count is %x, can't continue find more byte pid. \n", __func__, __LINE__, count);
					break;
				}

				pUBLProcess->pid = (u16)(*(data_id - PID_MSB_OFFSET - offset) << 8) + (u16)(*(data_id - PID_LSB_OFFSET - offset));
				pUBLProcess->vid = (u16)(*(data_id - VID_MSB_OFFSET - offset) << 8) + (u16)(*(data_id - VID_LSB_OFFSET - offset));
				found = true;
				TS_LOG_INFO("%s, %d, found pid & vid in the first way\n", __func__, __LINE__);
				break;
			}
		}

check_pattern_FAC:
		if (*(data_ptr - count) == FW_VERSION_EXTRA_OFFSET_BYTE_0) {
			data_id = data_ptr - count;// current is 0x0C
			if(count + FW_BINDATA_ID2_OFFSET < chk_size && count >= EXTRA_FW_VERSION_MSB_OFFSET) {
				if ((*(data_id - FW_BINDATA_ID3_OFFSET) == FW_VERSION_EXTRA_OFFSET_BYTE_1) && (*(data_id - FW_BINDATA_ID2_OFFSET) == FW_VERSION_EXTRA_OFFSET_BYTE_2))
				{
					if ((*(data_id + EXTRA_VID_MSB_OFFSET) == VID_MSB_VALUE) && (*(data_id + EXTRA_VID_LSB_OFFSET) == VID_LSB_VALUE))	// add check vid is 2D1F
					{
						pUBLProcess->vid = *(data_id + EXTRA_VID_MSB_OFFSET) * 256 + *(data_id + EXTRA_VID_LSB_OFFSET);
						pUBLProcess->pid = *(data_id + EXTRA_PID_MSB_OFFSET) * 256 + *(data_id + EXTRA_PID_LSB_OFFSET);
						pUBLProcess->version = *(data_id + EXTRA_FW_VERSION_MSB_OFFSET) * 256 + *(data_id + EXTRA_FW_VERSION_LSB_OFFSET);
						TS_LOG_INFO("%s, %d, found pid & vid in the second way\n", __func__, __LINE__);
						found = true;
						break;
					}
				}
			}
		}
		count++;
		if(count % COUNT_BASE == 0) {
			TS_LOG_INFO("%s,  current count is %x \n", __func__, count);
		}
	}

	TS_LOG_INFO("%s:  pid 0x%04x,  vid 0x%04x\n", __func__, pUBLProcess->pid, pUBLProcess->vid);
	return found;
}


//
//==========================================================================
//	Hex file and main code
//==========================================================================
// check_hex_file_g11 //ok return 0 , fail return minus number.
int check_hex_file_g11(const u8 *strFiledata, unsigned long fwFile_size, UBL_PROCESS *pUBLProcess, UBL_STATUS *pUBLStatus)
{
	unsigned long expand_address = 0;
	unsigned long startLinearAddress = 0;
	unsigned long count = 0;
	unsigned long file_size = 0;
	unsigned long start_address = (unsigned long)-1;
	unsigned long max_address = 0;
	unsigned long data_size = UBL_ROM_SIZE;
	unsigned long address = 0;
	unsigned long byte_count = 0;
	unsigned long record_type =0;
	unsigned long sum = 0;
	unsigned long data = 0;
	unsigned long total = 0;//used to compute checksum.
	unsigned long idx = 0;
	unsigned long intaddress = 0;
	unsigned long bc=0, ia=0, rt=0;
	char s[26]={0}, n[8]={0};
	char cr[2]={0}, lf[2]={0};
	u8 *rom_data = NULL;//Pointer to the rom
	int ret = NO_ERR;


	if ((!strFiledata) || (!pUBLProcess) || (!pUBLStatus) || (!fwFile_size)) {
		TS_LOG_ERR("date is null\n");
		return -EINVAL;
	}

	rom_data = pUBLProcess->data;//Pointer to the rom

	file_size = fwFile_size;

	// Set all content as 0xFF, important
	for ( idx = 0; idx < data_size; idx++ ){
		*(rom_data + idx) = 0xFF;
	}
	//memset(rom_data, 0xFF, data_size); // size too big for memset?
	TS_LOG_INFO("%s, memset romdata\n",__func__);

	/*-------------------------------------------------------------*/
	while(count < file_size) {
		memcpy( s, strFiledata + count, 1 );
		if ( s[0] != 0 ) {
			count++;

			if ( s[0] == ':' ) {/*: means data begin*/
				memcpy( s, strFiledata + count, 2 + 4 + 2 );// 2 byte count, 4 byte int address, 2 byte record type
				s[2 + 4 + 2] = '\0';
				count += 2 + 4 + 2;

				//sscanf(s, "%2lx%4lx%2lx", &byte_count, &intaddress, &record_type );
				memcpy( n, s, 2 );
				n[2] = '\0';
				bc = simple_strtol( n, NULL, HEXADECIMAL );//str to sexadecimal number
				memcpy( n, s+2, 4 );
				n[4] = '\0';
				ia = simple_strtol( n, NULL, HEXADECIMAL );
				memcpy( n, s+2+4, 2 );
				n[2] = '\0';
				rt = simple_strtol( n, NULL, HEXADECIMAL );
				byte_count = (unsigned long)bc;
				intaddress = (unsigned long)ia;
				record_type = (unsigned long)rt;
				//TS_LOG_DEBUG("byte_count=0x%x, intaddress=0x%x, record_type=0x%x \n", byte_count, intaddress, record_type);

				address = (unsigned long)intaddress;
				total = byte_count + (unsigned char)(address) + (unsigned char)(address >> 8) + record_type;//get high 8 bit data to compute checksum

				switch (record_type) {
				case RECORD_TYPE_0:/*0 1 2 3 4 5 means different type, defines different data formart */
					address += expand_address;
					if (start_address == (unsigned long)-1)//first here
						start_address = address;

					if(address < start_address){
						TS_LOG_ERR("%s, %d err, address is %x, start_address is %x, expand_address is %x\n", __func__, __LINE__, address, start_address, expand_address);
						TS_LOG_ERR("%s, %d err, max_address is %x, byte_count is %x, data_size is %x, intaddress=0x%x\n", __func__, __LINE__, max_address, byte_count, data_size, intaddress);
						goto release_rom_data;
					}

					address -= start_address;
					if (address > max_address)
						max_address = address + byte_count-1;

					for (idx = 0; idx < byte_count; idx++) {
						memcpy( s, strFiledata + count, 2 );// hex 2 bytes string convert one data
						s[2] = '\0';
						count += 2;
						data = simple_strtol( s, NULL, HEXADECIMAL );
						total += data;
						if ( address + idx < data_size ) {
							*(rom_data+address+idx) = (unsigned char)data;
						} else {
							TS_LOG_ERR("%s, %d convert data err\n", __func__, __LINE__);
							goto release_rom_data;
						}
					}

					memcpy( s, strFiledata+count, 2 );// hex 2 bytes string convert one data
					s[2] = '\0';
					count += 2;
					sum = simple_strtol( s, NULL, HEXADECIMAL );
					total += sum;

					if ((unsigned char)(total & 0xff) != 0x00) {
						TS_LOG_ERR("%s, %d checksum err, count is %d\n", __func__, __LINE__, count);
						TS_LOG_ERR("%s, %d checksum err, total is %x, sum is %x\n", __func__, __LINE__, total, sum);
						TS_LOG_ERR("%s, %d checksum err, address is %x, start_address is %x, expand_address is %x\n", __func__, __LINE__, address, start_address, expand_address);
						TS_LOG_ERR("%s, %d checksum err, max_address is %x, byte_count is %x, data_size is %x, intaddress=0x%x\n", __func__, __LINE__, max_address, byte_count, data_size, intaddress);
						for (idx = 0; idx < byte_count; idx++) {
							printk("  %4x", *(rom_data+address+idx));
						}

						goto release_rom_data; /* check sum error */
					}
					break;

				case RECORD_TYPE_1:
					memcpy( s, strFiledata+count, 2 );// hex 2 bytes string convert one data
					s[2] = '\0';
					count += 2;
					sum = simple_strtol( s, NULL, HEXADECIMAL );
					total += sum;

					if ((unsigned char)(total & 0xff) != 0x00) {
						TS_LOG_ERR("%s, %d checksum err\n", __func__, __LINE__);
						goto release_rom_data; /* check sum error */
					}
					break;

				case RECORD_TYPE_2:
					memcpy( s, strFiledata + count, 4 );// hex 4 bytes string convert 2 bytes address
					s[4] = '\0';
					count += 4;
					expand_address = simple_strtol( s, NULL, HEXADECIMAL );
					total += (unsigned char)(expand_address) + (unsigned char)(expand_address >> 8); //compute high 8 bytes and low 8 bytes address
					memcpy( s, strFiledata+count, 2 );// hex 2 bytes string convert one data
					s[2] = '\0';
					count += 2;
					sum = simple_strtol( s, NULL, HEXADECIMAL );
					total += sum;

					if ((unsigned char)(total & 0xff) != 0x00) {
						TS_LOG_ERR("%s, %d checksum err, count is %d\n", __func__, __LINE__, count);
						TS_LOG_ERR("%s, %d checksum err, total is %x\n", __func__, __LINE__, total);
						goto release_rom_data; /* check sum error */
					}
					expand_address <<=4;//??
					break;

				case RECORD_TYPE_3:
					{
						unsigned long cs=0, ip=0;

						memcpy( s, strFiledata+count, 4 );//convert 4 bytes
						s[4] = '\0';
						count += 4;
						cs = simple_strtol( s, NULL, HEXADECIMAL );
						total += (unsigned char)(cs) + (unsigned char)(cs >> 8);//compute checksum
						memcpy( s, strFiledata + count, 4 );
						s[4] = '\0';
						count += 4;
						ip = simple_strtol( s, NULL, HEXADECIMAL );
						total += (unsigned char)(ip) + (unsigned char)(ip >> 8);//compute checksum

						expand_address = (cs << 4) + ip;
						memcpy( s, strFiledata + count, 2 );//convert 2 bytes
						s[2] = '\0';
						count += 2;
						sum = simple_strtol( s, NULL, HEXADECIMAL );
						total += sum;

						if ((unsigned char)(total & 0x0f) != 0x00) {
							TS_LOG_ERR("%s, %d checksum err\n", __func__, __LINE__);
							goto release_rom_data;/* check sum error */
						}
						expand_address <<= 16;//??
						break;
					}

				case RECORD_TYPE_4:
					memcpy( s, strFiledata + count, 4 );//convert 4 bytes
					s[4] = '\0';
					count += 4;
					expand_address = simple_strtol( s, NULL, HEXADECIMAL );
					total += (unsigned char)(expand_address) + (unsigned char)(expand_address >> 8);//compute checksum
					memcpy( s, strFiledata + count, 2 );//convert 2 bytes
					s[2] = '\0';
					count += 2;
					sum = simple_strtol( s, NULL, HEXADECIMAL );
					total += sum;

					if ((unsigned char)(total & 0x0f) != 0x00) {
						TS_LOG_ERR("%s, %d checksum err\n", __func__, __LINE__);
						goto release_rom_data;/* check sum error */
					}
					expand_address <<= 16;//??
					break;

				case RECORD_TYPE_5:
					memcpy( s, strFiledata + count, 8 );//convert 8 bytes
					s[8] = '\0';
					count += 8;
					startLinearAddress = simple_strtol( s, NULL, HEXADECIMAL );
					total += (unsigned char)(startLinearAddress) + (unsigned char)(startLinearAddress >> 8);//compute checksum
					total += (unsigned char)(startLinearAddress >> 16) + (unsigned char)(startLinearAddress >> 24);//compute checksum
					memcpy( s, strFiledata+count, 2 );//convert 2 bytes
					s[2] = '\0';
					count += 2;
					sum = simple_strtol( s, NULL, HEXADECIMAL );
					total += sum;

					if ((unsigned char)(total & 0x0f) != 0x00) {
						TS_LOG_ERR("%s, %d checksum err\n", __func__, __LINE__);
						goto release_rom_data; /* check sum error */
					}
					break;
				default:
					TS_LOG_ERR("%s, %d record type unknown\n", __func__, __LINE__);
					goto release_rom_data;
				}
				memcpy( cr, strFiledata + count, 1 );
				count += 1;
				memcpy( lf, strFiledata + count, 1 );
				count += 1;
				if (cr[0] != '\r' || lf[0] != '\n') {
					TS_LOG_ERR("%s, %d  cr invalid\n", __func__, __LINE__);
					goto release_rom_data;
				}
			}
		}
		else {
			TS_LOG_ERR("%s, %d data invalid\n", __func__, __LINE__);
			goto release_rom_data;
		}
	}
	while ( 1 ) {
		if ( (max_address & 0x1FF) == 0x1FF ) {
			break;
		}
		max_address++;
	}

	if ( max_address >= UBL_MAIN_SIZE ) {
		TS_LOG_ERR("File size error. \n");
		ret = WACOM_ERR_FW_FILE_SIZE;
		goto release_rom_data;
	}

	if ( start_address != UBL_MAIN_ADDRESS ) {
		TS_LOG_ERR("Start address error.  %x \n",  __func__, start_address);
		ret = WACOM_ERR_FW_FILE_ADDR;
		goto release_rom_data;
	}

	pUBLProcess->start_adrs = start_address;
	pUBLProcess->max_addres = max_address;
	pUBLProcess->size = max_address + 1;

	TS_LOG_INFO("Start address %x, Size %d. \n", pUBLProcess->start_adrs, pUBLProcess->size);

	TS_LOG_INFO("%s size(hex): 0x%x \n",  __func__, pUBLProcess->size);
	TS_LOG_INFO("%s start address: 0x%x \n",  __func__, start_address);
	TS_LOG_INFO("%s max address: 0x%x \n",  __func__, max_address);

	// Everything OK
	ret = UBL_OK;

release_rom_data:
	return ret;
}


//
//==========================================================================
//	wacom update API functions
//==========================================================================
// isProjectPID
bool isProjectPID(u16 thePID)
{
	int i = 0;
	if(!wac_data ){
		TS_LOG_ERR("%s : data is null\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < wac_data->pid_info.pid_num; i++) {
		if(wac_data->pid_info.pid[i] == thePID)
			return true;
	}
	return false;
}

// wacom_get_touch_query  ok return 0 , fail return minus number.
int wacom_get_touch_query(unsigned char *thePinID)
{
	u8 dataBuf[REPSIZE_TOUCH_QUERY];
	int ret = NO_ERR;

	if (!thePinID) {
		TS_LOG_ERR("date is null\n");
		ret = -EINVAL;
		return ret;
	}

	ret = UBL_G11T_GetFeature(FEATURE_REPID_TOUCH_QUERY, REPSIZE_TOUCH_QUERY, dataBuf);
	if ( ret == UBL_OK) {
		TOUCH_QUERY *p = (TOUCH_QUERY *)&dataBuf[1];  // //1 byte offset
			*thePinID = p->Reserverd_B2;//p can't be nell
	}
	else {
		*thePinID = 0x03;	// default 2 bits pull high is 3, means no connect
	}
	return ret;
}

//ok return 0 , fail return minus number.
int wacom_get_hw_pinid(unsigned char *pin_id)
{
	int ret = NO_ERR;
	u8 the_pin_id = 0;

	if (!pin_id) {
		TS_LOG_ERR("date is null\n");
		ret = -EINVAL;
		return ret;
	}

	ret = wacom_get_touch_query(&the_pin_id);
	*pin_id = the_pin_id;

	return ret;
}


// wacom_do_fw_update
// force update to the fw_img  //ok return 0 , fail return minus number.
int wacom_do_fw_update(const u8 *fw_img, u32 fw_filesize)
{
	UBL_STATUS *pUBLStatus = NULL;
	UBL_PROCESS *pUBLProcess = NULL;
	u8 *flash_rom_data = NULL;
	int i = 0, ret = UBL_OK;
	u16 dev_pid = 0, hex_file_pid = 0;
	u16 dev_fw_version = 0, hex_file_fw_version = 0;
	bool b_ret = false;

	if (!fw_img || !wac_data) {
		TS_LOG_ERR("date is null\n");
		return -EINVAL;
	}

	pUBLStatus = (UBL_STATUS *)kzalloc(sizeof(UBL_STATUS), GFP_KERNEL);
	pUBLProcess = (UBL_PROCESS *)kzalloc(sizeof(UBL_PROCESS), GFP_KERNEL);
	flash_rom_data = (u8 *)vmalloc(UBL_ROM_SIZE);
	if (pUBLStatus == NULL || pUBLProcess == NULL || flash_rom_data == NULL) {
		TS_LOG_ERR("cannot preserve memories \n");
		return WACOM_ERR_NOMEM;
	}

	pUBLProcess->data = flash_rom_data;
	pUBLProcess->start_adrs = 0;
	pUBLProcess->process = UBL_NONE;


	/*From here starts reading hex file, the file read by caller****/
	TS_LOG_INFO("checking hex file..., file_size=%u \n", fw_filesize);

	ret = check_hex_file_g11(fw_img, fw_filesize, pUBLProcess, pUBLStatus);
	if( ret !=  NO_ERR) {
		//TS_LOG_DEBUG("reading the hex file failed, ret = %d\n", pUBLStatus->ret);
		TS_LOG_DEBUG("reading the hex file failed, \n");
		goto dfu_Release_Memory;
	}
	if(wac_data->wacom_update_firmware_from_sd_flag) {
		TS_LOG_INFO("%s, begin update firmware from sd, update directly\n", __func__);
		goto todo_update_firmware;
	}
	// Get PID and F/W Version by  check the binary data
	b_ret = find_pid_version_in_bindata(pUBLProcess);
	TS_LOG_INFO("Hex file VID: 0x%04x, PID: 0x%04x \n", pUBLProcess->vid, pUBLProcess->pid);
	TS_LOG_INFO("Hex file firmware version: 0x%04x\n", pUBLProcess->version);
	if (false == b_ret){
		ret = NO_ERR;
		goto dfu_Release_Memory;
	}
	hex_file_pid = pUBLProcess->pid;
	hex_file_fw_version = pUBLProcess->version;

	// Check these update rule only when not bootloader pid
	if(  wac_data->dev_pid != UBL_G11T_UBL_PID) {
		// hex file pid is not project pid, don't update
		if( isProjectPID( hex_file_pid ) == false ) {
			TS_LOG_ERR("firmware hex file not match this project. \n");
			ret = WACOM_ERR_FW_FILE_ERROR;
			goto dfu_Release_Memory;
		}

		/*fw_restore false means normal boot.
		if  broken, and second upgrade, don't need check pid.*/
		if(wac_data->fw_restore == false ) {
			// pid not match -- different SKU,  don't update
			if( hex_file_pid != wac_data->dev_pid ) {
				TS_LOG_ERR("firmware hex file pid mismatch. \n");
				ret = WACOM_ERR_PID_MISMATCH;
				goto dfu_Release_Memory;
			}
			// pid match but same version, don't update
			if( hex_file_fw_version == wac_data->dev_fw_version ){
				TS_LOG_INFO("firmware already up to date! \n");
				ret =  NO_ERR;
				goto dfu_Release_Memory;
			}
		}
	}

todo_update_firmware:
	// Start from here --- starts flash operation
	// Enter BootLoader mode
	i = 0;
	TS_LOG_INFO("%s, pUBLStatus->pid = 0x%04x\n", __func__, pUBLStatus->pid);
	if( wac_data->dev_pid != UBL_G11T_UBL_PID ) {
		ret = EnterG11TUBL();
		if( ret != UBL_OK ) {
			TS_LOG_ERR("EnterUBL error. \n");
			goto dfu_Exit_UBL;
		}
		/*Check if the device successfully entered in UBL mode*/
		for ( i = 0; i < UBL_RETRY_NUM; i++ ) {
			ret = UBL_G11T_Check_Mode();
			if ( ret ==UBL_OK ) { // break when OK
				break;
			}
			TS_LOG_ERR("EnterUBL Check mode error. \n");
			msleep(200);// wait 200 ms
		}
		if ( ret != UBL_OK ) {
			//ret = UBL_ERROR;
			TS_LOG_ERR("EnterUBL error(2). \n");
			goto dfu_Exit_UBL;
		}
	}

	TS_LOG_INFO("%s, enter bootloder\n", __func__);

	/*Conducting erasing and writing operation, the main part of the flash*/
	pUBLProcess->process = UBL_WRITE;
	ret = UBL_G11T_Write(pUBLProcess, pUBLStatus);
	if ( ret != UBL_OK ) {
		TS_LOG_ERR("UBL_G11T_Write returned false \n");
		ret = WACOM_EXIT_FAIL;
		//If writing is failed(including erase), do not call ExitG11TUBL
		goto dfu_Exit_UBL;
	}

	// Success
	ret = WACOM_RET_OK;

dfu_Exit_UBL:
	TS_LOG_DEBUG("closing device... \n");
	ExitG11TUBL();

dfu_Release_Memory:
	if (flash_rom_data != NULL) {
		vfree(flash_rom_data);
		flash_rom_data = NULL;
	}
	if (pUBLStatus != NULL) {
		kfree(pUBLStatus);
		pUBLStatus = NULL;
	}
	if (pUBLProcess != NULL) {
		kfree(pUBLProcess);
		pUBLProcess = NULL;
	}

	return ret;
}


// wacom_fw_update
int wacom_fw_update(char *file_name)
{
	int ret = NO_ERR;
	struct device *dev = NULL;
	const struct firmware *fw_entry = NULL;
	char  fw_file_full_name[4*MAX_STR_LEN] = {0};

	if ((!file_name) || (!wac_data) || (!wac_data->wacom_dev)) {
		TS_LOG_ERR("date is null\n");
		ret = -EINVAL;
		return ret;
	}

	dev = &wac_data->wacom_dev->dev;

	wake_lock(&wac_data->ts_wake_lock);

	// Construct the firmware name with project_code, name, lcd, tppind id...  //firmware name is like:cameron_W9015_CAME58000_ofilm_inx.hex
	snprintf(fw_file_full_name, sizeof(fw_file_full_name), "ts/%s.hex",file_name);

	// Request firmware from system
	if (request_firmware(&fw_entry, fw_file_full_name, dev) != 0) {
		TS_LOG_ERR("cannot load firmware file %s \n", fw_file_full_name);
		ret = NO_ERR;
		wake_unlock(&wac_data->ts_wake_lock);
		return ret;
	}

	ret = wacom_do_fw_update(fw_entry->data, fw_entry->size);
	if(ret != NO_ERR ){
		TS_LOG_ERR("wacom_fw_update fail:  %d \n", ret);
	}

	wake_unlock(&wac_data->ts_wake_lock);

	release_firmware(fw_entry);

	return ret;
}


