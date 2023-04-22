/*

**************************************************************************
**                        STMicroelectronics 				**
**************************************************************************
**                        marco.cali@st.com				**
**************************************************************************
*                                                                        *
*                     API used by Driver Test Apk			 *
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
#include "fts.h"
#include "fts_lib/ftsCompensation.h"
#include "fts_lib/ftsIO.h"
#include "fts_lib/ftsError.h"
#include "fts_lib/ftsFrame.h"
#include "fts_lib/ftsFlash.h"
#include "fts_lib/ftsTest.h"
#include "fts_lib/ftsTime.h"
#include "fts_lib/ftsTool.h"

#ifdef DRIVER_TEST

#define MAX_PARAMS 50


//DEFINE COMMANDS TO TEST
#define CMD_READ   0x00
#define CMD_WRITE   0x01
#define CMD_READU16   0x02
#define CMD_READB2   0x03
#define CMD_READB2U16   0x04
#define CMD_POLLFOREVENT  0x05
#define CMD_SYSTEMRESET   0x06
#define CMD_CLEANUP   0x07
#define CMD_GETFORCELEN   0x08
#define CMD_GETSENSELEN   0x09
#define CMD_GETMSFRAME   0x0A
//#define CMD_GETMSKEYFRAME  0x0B
#define CMD_GETSSFRAME   0x0C
#define CMD_REQCOMPDATA   0x0D
#define CMD_READCOMPDATAHEAD  0x0E
#define CMD_READMSCOMPDATA  0x0F
#define CMD_READSSCOMPDATA  0x10
#define CMD_READGNCOMPDATA  0x11
#define CMD_GETFWVER   0x12
#define CMD_FLASHSTATUS   0x13
#define CMD_FLASHUNLOCK   0x14
#define CMD_READFWFILE   0x15
#define CMD_FLASHPROCEDURE  0x16
#define CMD_ITOTEST   0x17
#define CMD_INITTEST   0x18
#define CMD_MSRAWTEST   0x19
#define CMD_MSINITDATATEST  0x1A
#define CMD_SSRAWTEST   0x1B
#define CMD_SSINITDATATEST  0x1C
#define CMD_MAINTEST   0x1D
#define CMD_POWERCYCLE   0x1E
#define CMD_FWWRITE   0x1F
#define CMD_READCHIPINFO  0x20
#define CMD_REQFRAME	0x21




u32 functionToTest[MAX_PARAMS];
int numberParam = 0;

static ssize_t stm_driver_test_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
	int n;
	char *p = (char *) buf;

	memset(functionToTest, 0, MAX_PARAMS * sizeof (u32));


	for (n = 0; n < (count + 1) / 3 && n < MAX_PARAMS; n++) {
		sscanf(p, "%02X ", &functionToTest[n]);
		p += 3;
		TS_LOG_ERR("%s functionToTest[%d] = %02X\n", __func__, n, functionToTest[n]);

	}

	numberParam = n;
	TS_LOG_ERR("%s Number of Parameters = %d\n", __func__, numberParam);
	return count;
}

static ssize_t stm_driver_test_show(struct device *dev, struct device_attribute *attr, char *buf) {
	char buff[CMD_STR_LEN] = {0};
	int res = -1, j, count;
	int size = 6 * 2;
	int temp, i, byteToRead;
	u8* readData = NULL;
	u8 *all_strbuff = NULL;
	u8 *cmd = NULL;

	MutualSenseFrame frameMS;
	SelfSenseFrame frameSS;

	DataHeader dataHead;
	MutualSenseData compData;
	SelfSenseData comData;
	GeneralData gnData;

	u16 address;
	u16 fw_version;
	u16 config_id;

	Firmware fw;

	//struct used for defining which test  perform during the  MP test

	TestToDo todoDefault;
	struct fts_ts_info *info = fts_get_info();

	memset(&todoDefault, 0, sizeof(TestToDo));
	fw.data = NULL;

	todoDefault.MutualRaw = 1;
	todoDefault.SelfForceRaw = 1;
	todoDefault.SelfSenseRaw = 1;

	if (numberParam >= 1) {
		TS_LOG_ERR("%s NO COMMAND SPECIFIED!!! do: 'echo [cmd_code] [args] > stm_fts_cmd' before looking for result!\n", __func__);
		res = ERROR_OP_NOT_ALLOW;
		goto END;
	}

	res = fts_disableInterrupt();
	if (res < 0) {
		TS_LOG_INFO("%s stm_driver_test_show: ERROR %08X\n", __func__, res);
		res = (res | ERROR_DISABLE_INTER);
		goto END;
	}
	switch (functionToTest[0]) {
	case CMD_SYSTEMRESET:
		res = fts_system_reset();
		break;

	case CMD_READCHIPINFO:
		if (numberParam != 2) { //need to pass: doRequest
			TS_LOG_ERR("%s Wrong number of parameters!\n", __func__);
			res = ERROR_OP_NOT_ALLOW;
			break;
		}
		res = readChipInfo(functionToTest[1]);
		break;

	case CMD_CLEANUP: // TOUCH ENABLE/DISABLE
		if (numberParam != 2) { //need to pass: enableTouch
			TS_LOG_ERR("%s Wrong number of parameters!\n", __func__);
			res = ERROR_OP_NOT_ALLOW;
			break;
		}
		res = cleanUp(functionToTest[1]);
		break;

	case CMD_GETMSFRAME:
		if (numberParam != 3) {
			TS_LOG_ERR("%s Wrong number of parameters!\n", __func__);
			res = ERROR_OP_NOT_ALLOW;
			break;
		}
		TS_LOG_INFO("%s Get 1 MS Frame\n", __func__);
		flushFIFO(); //delete the events related to some touch (allow to call this function while touching the sreen without having a flooding of the FIFO)
		res = getMSFrame2((u16) ((((u8) functionToTest[1] & 0x00FF) << 8) + ((u8) functionToTest[2] & 0x00FF)), &frameMS);
		if (res >= 0) {
			TS_LOG_INFO("%s The frame size is %d words\n", __func__, res);
			size = (res * sizeof (short) + 8)*2;
			/* set res to OK because if getMSFrame is
			* successful res = number of words read
			*/
			res = OK;
			print_frame_short("MS frame =", array1dTo2d_short(frameMS.node_data, frameMS.node_data_size, frameMS.header.sense_node), frameMS.header.force_node, frameMS.header.sense_node);
		}
		break;
	/*read self raw*/
	case CMD_GETSSFRAME:
		if (numberParam != 3) {
			TS_LOG_ERR("%s Wrong number of parameters!\n", __func__);
			res = ERROR_OP_NOT_ALLOW;
			break;
		}
		TS_LOG_INFO("%s Get 1 SS Frame\n", __func__);
		flushFIFO(); //delete the events related to some touch (allow to call this function while touching the sreen without having a flooding of the FIFO)
		res = getSSFrame2((u16) ((((u8) functionToTest[1] & 0x00FF) << 8) + ((u8) functionToTest[2] & 0x00FF)), &frameSS);

		if (res >= OK) {
			TS_LOG_INFO("%s The frame size is %d words\n", __func__, res);
			size = (res * sizeof (short) + 8)*2+1;
			/* set res to OK because if getMSFrame is
			 * successful res = number of words read
			 */
			res = OK;
			print_frame_short("SS force frame =", array1dTo2d_short(frameSS.force_data, frameSS.header.force_node, 1), frameSS.header.force_node, 1);
			print_frame_short("SS sense frame =", array1dTo2d_short(frameSS.sense_data, frameSS.header.sense_node, frameSS.header.sense_node), 1, frameSS.header.sense_node);
		}
		break;



	case CMD_READCOMPDATAHEAD: //read comp data header
		if (numberParam != 3) {
			TS_LOG_ERR("%s Wrong number of parameters!\n", __func__);
			res = ERROR_OP_NOT_ALLOW;
			break;
		}
		TS_LOG_INFO("%s Requesting Compensation Data\n", __func__);
		res = requestCompensationData((u16) ((((u8) functionToTest[1] & 0x00FF) << 8) + ((u8) functionToTest[2] & 0x00FF)));
		if (res >= OK) {
			TS_LOG_INFO("%s Requesting Compensation Data Finished!\n", __func__);
			res = readCompensationDataHeader((u16) ((((u8) functionToTest[1] & 0x00FF) << 8) + ((u8) functionToTest[2] & 0x00FF)), &dataHead, &address);
			if (res >= OK){
				TS_LOG_INFO("%s Read Compensation Data Header OK!\n", __func__);
				size += (2 * sizeof (u8))*2;
			}
		}
		break;

	case CMD_READMSCOMPDATA: //read mutual comp data
		if (numberParam != 3) {
			TS_LOG_ERR("%s Wrong number of parameters!\n", __func__);
			res = ERROR_OP_NOT_ALLOW;
			break;
		}
		TS_LOG_INFO("%s Get MS Compensation Data\n", __func__);
		res = readMutualSenseCompensationData((u16) ((((u8) functionToTest[1] & 0x00FF) << 8) + ((u8) functionToTest[2] & 0x00FF)), &compData);

		if (res >= OK) {
			TS_LOG_INFO("%s MS Compensation Data Reading Finished!\n", __func__);
			size = ((compData.node_data_size + 9) * sizeof (u8))*2;
			print_frame_u8("MS Data (Cx2) =", array1dTo2d_u8(compData.node_data, compData.node_data_size, compData.header.sense_node), compData.header.force_node, compData.header.sense_node);
		}
		break;

	case CMD_READSSCOMPDATA:
		if (numberParam != 3) { //read self comp data
			TS_LOG_ERR("%s Wrong number of parameters!\n", __func__);
			res = ERROR_OP_NOT_ALLOW;
			break;
		}
		TS_LOG_INFO("%s Get SS Compensation Data...\n", __func__);
		res = readSelfSenseCompensationData((u16) ((((u8) functionToTest[1] & 0x00FF) << 8) + ((u8) functionToTest[2] & 0x00FF)), &comData);
		if (res >= OK) {
			TS_LOG_INFO("%s SS Compensation Data Reading Finished!\n", __func__);
			size = ((comData.header.force_node + comData.header.sense_node)*2 + 12) * sizeof (u8)*2;
			print_frame_u8("SS Data Ix2_fm = ", array1dTo2d_u8(comData.ix2_fm, comData.header.force_node, comData.header.force_node), 1, comData.header.force_node);
			print_frame_u8("SS Data Cx2_fm = ", array1dTo2d_u8(comData.cx2_fm, comData.header.force_node, comData.header.force_node), 1, comData.header.force_node);
			print_frame_u8("SS Data Ix2_sn = ", array1dTo2d_u8(comData.ix2_sn, comData.header.sense_node, comData.header.sense_node), 1, comData.header.sense_node);
			print_frame_u8("SS Data Cx2_sn = ", array1dTo2d_u8(comData.cx2_sn, comData.header.sense_node, comData.header.sense_node), 1, comData.header.sense_node);
		}
		break;

	case CMD_READGNCOMPDATA:
		if (numberParam != 3) { //read self comp data
			TS_LOG_ERR("%s Wrong number of parameters!\n", __func__);
			res = ERROR_OP_NOT_ALLOW;
			break;
		}
		TS_LOG_INFO("%s Get General Compensation Data...\n", __func__);
		res = readGeneralCompensationData((u16) ((((u8) functionToTest[1] & 0x00FF) << 8) + ((u8) functionToTest[2] & 0x00FF)), &gnData);
		if (res >= OK) {
			TS_LOG_INFO("%s General Compensation Data Reading Finished!\n", __func__);
			size = (14) * sizeof (u8)*2;
		}
		break;


	case CMD_MSINITDATATEST: // MS CX DATA TEST
		if (numberParam != 2) {//need to specify if stopOnFail 
			TS_LOG_ERR("%s Wrong number of parameters!\n", __func__);
			res = ERROR_OP_NOT_ALLOW;
			break;
		}
		res = production_test_ms_cx(LIMITS_FILE, functionToTest[1], &todoDefault,NULL,NULL);
		break;


	case CMD_POWERCYCLE:
		res = fts_chip_powercycle(info);
		break;

	default:
		TS_LOG_ERR("%s COMMAND ID NOT VALID!! Inset a value between 00 and 1E..\n", __func__);
		res = ERROR_OP_NOT_ALLOW;
		break;
	}


END: //here start the reporting phase, assembling the data to send in the file node
	all_strbuff = (u8 *) kmalloc(size, GFP_KERNEL);
	memset(all_strbuff, 0, size);

	snprintf(buff, sizeof (buff), "%02X", 0xAA);
	strncat(all_strbuff, buff, size);

	snprintf(buff, sizeof (buff), "%08X", res);
	strncat(all_strbuff, buff, size);

	if (res >= OK) {
		/*all the other cases are already fine printing only the res.*/
		switch (functionToTest[0]) {
			case CMD_GETMSFRAME:
				snprintf(buff, sizeof (buff), "%02X", (u8) frameMS.header.force_node);
				strncat(all_strbuff, buff, size);

				snprintf(buff, sizeof (buff), "%02X", (u8) frameMS.header.sense_node);
				strncat(all_strbuff, buff, size);

				for (j = 0; j < frameMS.node_data_size; j++) {
					snprintf(buff, sizeof (buff), "%04X", frameMS.node_data[j]);
					strncat(all_strbuff, buff, size);
				}

				kfree(frameMS.node_data);
				break;

			case CMD_GETSSFRAME:
				snprintf(buff, sizeof (buff), "%02X", (u8) frameSS.header.force_node);
				strncat(all_strbuff, buff, size);

				snprintf(buff, sizeof (buff), "%02X", (u8) frameSS.header.sense_node);
				strncat(all_strbuff, buff, size);

				// Copying self raw data Force
				for (j = 0; j < frameSS.header.force_node; j++) {
					snprintf(buff, sizeof (buff), "%04X", frameSS.force_data[j]);
					strncat(all_strbuff, buff, size);
				}


				// Copying self raw data Sense
				for (j = 0; j < frameSS.header.sense_node; j++) {
					snprintf(buff, sizeof (buff), "%04X", frameSS.sense_data[j]);
					strncat(all_strbuff, buff, size);
				}

				kfree(frameSS.force_data);
				kfree(frameSS.sense_data);
				break;

			case CMD_READMSCOMPDATA:
				snprintf(buff, sizeof (buff), "%02X", (u8) compData.header.force_node);
				strncat(all_strbuff, buff, size);

				snprintf(buff, sizeof (buff), "%02X", (u8) compData.header.sense_node);
				strncat(all_strbuff, buff, size);

				//Cpying CX1 value
				snprintf(buff, sizeof (buff), "%02X", compData.cx1);
				strncat(all_strbuff, buff, size);

				//Copying CX2 values
				for (j = 0; j < compData.node_data_size; j++) {
					snprintf(buff, sizeof (buff), "%02X", *(compData.node_data + j));
					strncat(all_strbuff, buff, size);
				}

				kfree(compData.node_data);
				break;

			case CMD_READSSCOMPDATA:
				snprintf(buff, sizeof (buff), "%02X", comData.header.force_node);
				strncat(all_strbuff, buff, size);

				snprintf(buff, sizeof (buff), "%02X", comData.header.sense_node);
				strncat(all_strbuff, buff, size);

				snprintf(buff, sizeof (buff), "%02X", comData.f_ix1);
				strncat(all_strbuff, buff, size);

				snprintf(buff, sizeof (buff), "%02X", comData.s_ix1);
				strncat(all_strbuff, buff, size);

				snprintf(buff, sizeof (buff), "%02X", comData.f_cx1);
				strncat(all_strbuff, buff, size);

				snprintf(buff, sizeof (buff), "%02X", comData.s_cx1);
				strncat(all_strbuff, buff, size);

				//Copying IX2 Force
				for (j = 0; j < comData.header.force_node; j++) {
					snprintf(buff, sizeof (buff), "%02X", comData.ix2_fm[j]);
					strncat(all_strbuff, buff, size);
				}

				//Copying IX2 Sense
				for (j = 0; j < comData.header.sense_node; j++) {
					snprintf(buff, sizeof (buff), "%02X", comData.ix2_sn[j]);
					strncat(all_strbuff, buff, size);
				}

				//Copying CX2 Force
				for (j = 0; j < comData.header.force_node; j++) {
					snprintf(buff, sizeof (buff), "%02X", comData.cx2_fm[j]);
					strncat(all_strbuff, buff, size);
				}

				//Copying CX2 Sense
				for (j = 0; j < comData.header.sense_node; j++) {
					snprintf(buff, sizeof (buff), "%02X", comData.cx2_sn[j]);
					strncat(all_strbuff, buff, size);
				}

				kfree(comData.ix2_fm);
				kfree(comData.ix2_sn);
				kfree(comData.cx2_fm);
				kfree(comData.cx2_sn);
				break;

			case CMD_READGNCOMPDATA:
				snprintf(buff, sizeof (buff), "%02X", gnData.header.force_node);
				strncat(all_strbuff, buff, size);

				snprintf(buff, sizeof (buff), "%02X", gnData.header.sense_node);
				strncat(all_strbuff, buff, size);

				snprintf(buff, sizeof (buff), "%02X", gnData.ftsd_lp_timer_cal0);
				strncat(all_strbuff, buff, size);

				snprintf(buff, sizeof (buff), "%02X", gnData.ftsd_lp_timer_cal1);
				strncat(all_strbuff, buff, size);

				snprintf(buff, sizeof (buff), "%02X", gnData.ftsd_lp_timer_cal2);
				strncat(all_strbuff, buff, size);

				snprintf(buff, sizeof (buff), "%02X", gnData.ftsd_lp_timer_cal3);
				strncat(all_strbuff, buff, size);

				snprintf(buff, sizeof (buff), "%02X", gnData.ftsa_lp_timer_cal0);
				strncat(all_strbuff, buff, size);

				snprintf(buff, sizeof (buff), "%02X", gnData.ftsa_lp_timer_cal1);
				strncat(all_strbuff, buff, size);
				break;

			case CMD_READCOMPDATAHEAD:
				snprintf(buff, sizeof (buff), "%02X", dataHead.force_node);
				strncat(all_strbuff, buff, size);

				snprintf(buff, sizeof (buff), "%02X", dataHead.sense_node);
				strncat(all_strbuff, buff, size);
				break;


			default:
				break;

		}
	}

	snprintf(buff, sizeof (buff), "%02X", 0xBB);
	strncat(all_strbuff, buff, size);

	count = snprintf(buf, TSP_BUF_SIZE, "%s\n", all_strbuff);
    /* need to reset the number of parameters in order to wait
     * the next comand, comment if you want to repeat the last comand sent just doing a cat
     */
	numberParam = 0;
	kfree(readData);
	kfree(all_strbuff);
	return count;
}

static DEVICE_ATTR(stm_driver_test, 0664, stm_driver_test_show, stm_driver_test_store);


static struct attribute *test_cmd_attributes[] = {
	&dev_attr_stm_driver_test.attr,
	NULL,
};

struct attribute_group test_cmd_attr_group = {
	.attrs = test_cmd_attributes,
};

#endif
