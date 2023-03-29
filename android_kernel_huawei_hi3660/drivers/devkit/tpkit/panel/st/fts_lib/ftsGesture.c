/*

**************************************************************************
**                        STMicroelectronics 							**
**************************************************************************
**                        marco.cali@st.com								**
**************************************************************************
*                                                                        *
*                     FTS Gesture Utilities								 *
*                                                                        *
**************************************************************************
**************************************************************************

*/

#include "ftsSoftware.h"
#include "ftsError.h"
#include "ftsGesture.h"
#include "ftsIO.h"
#include "ftsTool.h"
#include "../fts.h"


static u8 gesture_mask[GESTURE_MASK_SIZE] = { 0 };
static u8 custom_gestures[GESTURE_CUSTOM_NUMBER][GESTURE_CUSTOM_POINTS];
static u8 custom_gesture_index[GESTURE_CUSTOM_NUMBER] = { 0 };
u16 gesture_coordinates_x[GESTURE_COORDS_REPORT_MAX] = {0};
u16 gesture_coordinates_y[GESTURE_COORDS_REPORT_MAX] = {0};
int gesture_coords_reported = ERROR_OP_NOT_ALLOW;
static int refreshGestureMask = 0;
struct mutex gestureMask_mutex;


int updateGestureMask(u8 *mask, int size, int en) {
	u8 temp;
	int i;

	if (mask == NULL) {
		TS_LOG_ERR("%s updateGestureMask: Mask NULL! ERROR %08X \n", __func__, ERROR_OP_NOT_ALLOW);
		return ERROR_OP_NOT_ALLOW;
	}

	if (size <= GESTURE_MASK_SIZE) {
		if (en==FEAT_ENABLE) {
			mutex_lock(&gestureMask_mutex);
			TS_LOG_INFO("%s: setting gesture mask to enable...\n", __func__);
			if (mask != NULL) {
				for (i = 0; i < size;i++) {
					gesture_mask[i] = gesture_mask[i] | mask[i];	//back up of the gesture enabled
				}
			}
			refreshGestureMask = 1;
			TS_LOG_INFO("%s: gesture mask to enable SET!\n", __func__);
			mutex_unlock(&gestureMask_mutex);
			return OK;
		} else if (en==FEAT_DISABLE) {
			mutex_lock(&gestureMask_mutex);
			TS_LOG_INFO("%s: setting gesture mask to disable...\n", __func__);
			for (i = 0; i < size;i++) {
				temp = gesture_mask[i] ^ mask[i];			// enabled XOR disabled
				gesture_mask[i] = temp & gesture_mask[i];	// temp AND enabled				//disable the gestures that were enabled
			}
			TS_LOG_INFO("%s: gesture mask to disable SET!\n", __func__);
			refreshGestureMask = 1;
			mutex_unlock(&gestureMask_mutex);
			return OK;
		} else {
			TS_LOG_ERR("%s:Enable parameter Invalid! %d != %d or %d ERROR %08X",
				__func__, en, FEAT_DISABLE, FEAT_ENABLE, ERROR_OP_NOT_ALLOW);
			return ERROR_OP_NOT_ALLOW;
		}
	} else {
		TS_LOG_ERR("%s: Size not valid! %d > %d ERROR\n", size, GESTURE_MASK_SIZE);
		return ERROR_OP_NOT_ALLOW;
	}

}

int enableGesture(u8 *mask, int size) {
	u8 cmd[GESTURE_MASK_SIZE + 2];
	u8 readData[FIFO_EVENT_SIZE];
	int i, res;
	int event_to_search[4] = { EVENTID_GESTURE, EVENT_TYPE_ENB, 0x00, GESTURE_ENABLE };

	TS_LOG_INFO("%s:Trying to enable gesture...\n", __func__);
	cmd[0] = FTS_CMD_GESTURE_CMD;
	cmd[1] = GESTURE_ENABLE;

	if (size <= GESTURE_MASK_SIZE) {
		mutex_lock(&gestureMask_mutex);
		if (mask != NULL) {
			for (i = 0; i < size;i++) {
				cmd[i + 2] = mask[i];
				gesture_mask[i] = gesture_mask[i] | mask[i];	//back up of the gesture enabled
			}
			while (i < GESTURE_MASK_SIZE) {
				cmd[i + 2] = gesture_mask[i];
				i++;
			}
		}
		else {
			for (i = 0; i < GESTURE_MASK_SIZE;i++) {
				cmd[i + 2] = gesture_mask[i];
			}
		}

		res = fts_writeFwCmd(cmd, GESTURE_MASK_SIZE + 2);
		if (res < OK) {
			TS_LOG_ERR("%s: ERROR %08X\n", __func__, res);
			goto END;
		}

		res = pollForEvent(event_to_search, 4, readData, GENERAL_TIMEOUT);
		if (res < OK) {
			TS_LOG_ERR("%s: pollForEvent ERROR %08X\n", __func__, res);
			goto END;
		}

		if (readData[4] != 0x00) {
			TS_LOG_ERR("%s: ERROR %08X\n", __func__, ERROR_GESTURE_ENABLE_FAIL);
			res= ERROR_GESTURE_ENABLE_FAIL;
			goto END;
		}

		TS_LOG_INFO("%s:DONE!\n", __func__);
		res = OK;

END:
		mutex_unlock(&gestureMask_mutex);
		return res;
	}
	else {
		TS_LOG_ERR("%s:Size not valid! %d > %d\n",
				__func__, size, GESTURE_MASK_SIZE);
		return ERROR_OP_NOT_ALLOW;
	}

}


int disableGesture(u8 *mask, int size) {
	u8 cmd[2 + GESTURE_MASK_SIZE];
	u8 readData[FIFO_EVENT_SIZE];
	u8 temp;
	int i, res;
	int event_to_search[4] = { EVENTID_GESTURE, EVENT_TYPE_ENB, 0x00, GESTURE_DISABLE };

	TS_LOG_INFO("%s:Trying to disable gesture\n", __func__);
	cmd[0] = FTS_CMD_GESTURE_CMD;
	cmd[1] = GESTURE_DISABLE;

	if (size <= GESTURE_MASK_SIZE) {
		mutex_lock(&gestureMask_mutex);
		if (mask != NULL) {
			for (i = 0; i < size;i++) {
				cmd[i + 2] = mask[i];
				temp = gesture_mask[i] ^ mask[i];		// enabled XOR disabled
				gesture_mask[i] = temp & gesture_mask[i];	// temp AND enabled, disable the gestures that were enabled
			}
			while (i < GESTURE_MASK_SIZE) {
				cmd[i + 2] = 0x00;	//leave untouched the gestures not specified
				i++;
			}
		} else {
			for (i = 0; i < GESTURE_MASK_SIZE;i++) {
				cmd[i + 2] = 0xFF;	//if NULL is passed disable all the possible gestures
			}
		}

		res = fts_writeFwCmd(cmd, 2 + GESTURE_MASK_SIZE);
		if (res < OK) {
			TS_LOG_ERR("%s: ERROR %08X\n", __func__, res);
			goto END;
		}

		res = pollForEvent(event_to_search, 4, readData, GENERAL_TIMEOUT);
		if (res < OK) {
			TS_LOG_ERR("%s: pollForEvent ERROR %08X\n", __func__, res);
			goto END;
		}

		if (readData[4] != 0x00) {
			TS_LOG_ERR("%s: ERROR %08X\n", __func__, ERROR_GESTURE_ENABLE_FAIL);
			res = ERROR_GESTURE_ENABLE_FAIL;
			goto END;
		}

		TS_LOG_INFO("%s:DONE!\n", __func__);

		res = OK;

END:
		mutex_unlock(&gestureMask_mutex);
		return res;
	}
	else {
		TS_LOG_ERR("%s disableGesture: Size not valid! %d > %d\n",
			__func__, size, GESTURE_MASK_SIZE);
		return ERROR_OP_NOT_ALLOW;
	}
}

int startAddCustomGesture(u8 gestureID) {
	u8 cmd[3] = { FTS_CMD_GESTURE_CMD, GESTURE_START_ADD,  gestureID };
	int res;
	u8 readData[FIFO_EVENT_SIZE];
	int event_to_search[4] = { EVENTID_GESTURE,EVENT_TYPE_ENB,gestureID,GESTURE_START_ADD };

	res = fts_writeFwCmd(cmd, 3);
	if (res < OK) {
		TS_LOG_ERR("%s: Impossible to start adding custom gesture ID = %02X! ERROR %08X\n",
					__func__,  gestureID, res);
		return res;
	}

	res = pollForEvent(event_to_search, 4, readData, GENERAL_TIMEOUT);
	if (res < OK) {
		TS_LOG_ERR("%s: start add event not found! ERROR %08X\n",
						__func__, res);
		return res;
	}

	if (readData[2] != gestureID || readData[4] != 0x00) {			//check of gestureID is redundant
		TS_LOG_ERR("%s: start add event status not OK! ERROR %08X\n", __func__, readData[4]);
		return ERROR_GESTURE_START_ADD;
	}


	return OK;
}


int finishAddCustomGesture(u8 gestureID) {
	u8 cmd[3] = { FTS_CMD_GESTURE_CMD, GESTURE_FINISH_ADD,  gestureID };
	int res;
	u8 readData[FIFO_EVENT_SIZE];
	int event_to_search[4] = { EVENTID_GESTURE,EVENT_TYPE_ENB,gestureID,GESTURE_FINISH_ADD };

	res = fts_writeFwCmd(cmd, 3);
	if (res < OK) {
		TS_LOG_ERR("%s: Impossible to finish adding custom gesture ID = %02X! ERROR %08X\n",
						__func__, gestureID, res);
		return res;
	}

	res = pollForEvent(event_to_search, 4, readData, GENERAL_TIMEOUT);
	if (res < OK) {
		TS_LOG_ERR("%s: finish add event not found! ERROR %08X\n", __func__, res);
		return res;
	}

	if (readData[2] != gestureID || readData[4] != 0x00) {			//check of gestureID is redundant
		TS_LOG_ERR("%s: finish add event status not OK! ERROR %08X\n", __func__, readData[4]);
		return ERROR_GESTURE_FINISH_ADD;
	}

	return OK;
}

int loadCustomGesture(u8 *template, u8 gestureID) {
	int res, i, wheel;
	int remaining = GESTURE_CUSTOM_POINTS;
	int toWrite, offset = 0;
	u8 cmd[TEMPLATE_CHUNK + 5];
	int event_to_search[4] = { EVENTID_GESTURE,EVENT_TYPE_ENB,gestureID,GESTURE_DATA_ADD };
	u8 readData[FIFO_EVENT_SIZE];

	TS_LOG_INFO("%s Starting adding custom gesture procedure...\n", __func__);

	res = startAddCustomGesture(gestureID);
	if (res < OK) {
		TS_LOG_ERR("%s: unable to start adding procedure! ERROR %08X\n", __func__, res);
		return res;
	}
	wheel =0;
	cmd[0] = FTS_CMD_GESTURE_CMD;
	cmd[1] = GESTURE_DATA_ADD;
	cmd[2] = gestureID;
	while (remaining > 0) {
		if (remaining > TEMPLATE_CHUNK) {
			toWrite = TEMPLATE_CHUNK;
		}
		else {
			toWrite = remaining;
		}

		cmd[3] = toWrite;
		cmd[4] = offset;
		for (i = 0; i < toWrite; i++) {
			cmd[i + 5] = template[wheel++];
		}

		res = fts_writeFwCmd(cmd, toWrite + 5);
		if (res < OK) {
			TS_LOG_ERR("%s: unable to start adding procedure! ERROR %08X\n", __func__, res);
			return res;
		}

		res = pollForEvent(event_to_search, 4, readData, GENERAL_TIMEOUT);
		if (res < OK) {
			TS_LOG_ERR("%s: add event not found! ERROR %08X\n", __func__, res);
			return res;
		}

		if (readData[2] != gestureID || readData[4] != 0x00) {			//check of gestureID is redundant
			TS_LOG_ERR("%s: add event status not OK! ERROR %08X\n", __func__, readData[4]);
			return ERROR_GESTURE_DATA_ADD;
		}

		remaining -= toWrite;
		offset += toWrite / 2;
	}

	res = finishAddCustomGesture(gestureID);
	if (res < OK) {
		TS_LOG_ERR("%s: unable to finish adding procedure! ERROR %08X\n", __func__, res);
		return res;
	}

	TS_LOG_INFO("%s: Adding custom gesture procedure DONE!\n", __func__);
	return OK;
}


int reloadCustomGesture() {
	int res, i;

	TS_LOG_INFO("%s: Starting reload Gesture Template...\n", __func__);

	for (i = 0; i < GESTURE_CUSTOM_NUMBER; i++) {
		if (custom_gesture_index[i] == 1) {
			res = loadCustomGesture(custom_gestures[i], GESTURE_CUSTOM_OFFSET + i);
			if (res < OK) {
				TS_LOG_ERR("%s: Impossible to load custom gesture ID = %02X! ERROR %08X\n", __func__, GESTURE_CUSTOM_OFFSET + i, res);
				return res;
			}
		}
	}

	TS_LOG_INFO("%s:Reload Gesture Template DONE!\n", __func__);
	return OK;
}

int enterGestureMode(int reload) {
	u8 cmd = FTS_CMD_GESTURE_MODE;
	int res, ret;

	res = fts_disableInterrupt();
	if (res < OK) {
		TS_LOG_ERR("%s enterGestureMode: ERROR %08X\n", __func__, res | ERROR_DISABLE_INTER);
		return res | ERROR_DISABLE_INTER;
	}

	if (reload == 1 || refreshGestureMask == 1) {

		if (reload == 1) {
			res = reloadCustomGesture();
			if (res < OK) {
				TS_LOG_ERR("%s: impossible reload custom gesture! ERROR %08X\n", __func__, res);
				goto END;
			}
		}

		/****** mandatory steps to set the correct gesture mask defined by the user ******/
		res = disableGesture(NULL, 0);
		if (res < OK) {
			TS_LOG_ERR("%s: disableGesture ERROR %08X\n", __func__, res);
			goto END;
		}

		res = enableGesture(NULL, 0);
		if (res < OK) {
			TS_LOG_ERR("%s: enableGesture ERROR %08X\n", __func__, res);
			goto END;
		}

		refreshGestureMask = 0;
		/**********************************************************************************/
	}

	res = fts_writeFwCmd(&cmd, 1);
	if (res < OK) {
		TS_LOG_ERR("%s: enter gesture mode ERROR %08X\n", __func__, res);
		goto END;
	}

	res = OK;
END:
	ret = fts_enableInterrupt();
	if (ret < OK) {
		TS_LOG_ERR("%s: fts_enableInterrupt ERROR %08X\n", __func__, res | ERROR_ENABLE_INTER);
		res |= ret | ERROR_ENABLE_INTER;
	}


	return res;
}

int addCustomGesture(u8 *data, int size, u8 gestureID) {
	int index, res, i;

	index = gestureID - GESTURE_CUSTOM_OFFSET;

	TS_LOG_INFO("%s Starting Custom Gesture Adding procedure...\n", __func__);
	if (size != GESTURE_CUSTOM_POINTS || (gestureID != GES_ID_CUST1 && gestureID != GES_ID_CUST2 && gestureID != GES_ID_CUST3 && gestureID != GES_ID_CUST4 && gestureID != GES_ID_CUST5)) {
		TS_LOG_ERR("%s: Invalid size (%d) or Custom GestureID (%02X)! ERROR %08X\n",
					__func__, size, gestureID, ERROR_OP_NOT_ALLOW);
		return ERROR_OP_NOT_ALLOW;
	}

	for (i = 0; i < GESTURE_CUSTOM_POINTS; i++) {
		custom_gestures[index][i] = data[i];
	}

	res = loadCustomGesture(custom_gestures[index], gestureID);
	if (res < OK) {
		TS_LOG_ERR("%s: impossible to load the custom gesture! ERROR %08X\n", __func__, res);
		return res;
	}

	custom_gesture_index[index] = 1;
	TS_LOG_INFO("%s: Custom Gesture Adding procedure DONE!\n", __func__);
	return OK;
}

int removeCustomGesture(u8 gestureID) {
	int res, index;
	u8 cmd[3] = { FTS_CMD_GESTURE_CMD, GETURE_REMOVE_CUSTOM, gestureID };
	int event_to_search[4] = { EVENTID_GESTURE, EVENT_TYPE_ENB, gestureID, GETURE_REMOVE_CUSTOM };
	u8 readData[FIFO_EVENT_SIZE];

	index = gestureID - GESTURE_CUSTOM_OFFSET;

	TS_LOG_INFO("%s Starting Custom Gesture Removing procedure...\n", __func__);
	if (gestureID != GES_ID_CUST1 && gestureID != GES_ID_CUST2 && gestureID != GES_ID_CUST3 && gestureID != GES_ID_CUST4 && gestureID != GES_ID_CUST5) {
		TS_LOG_ERR("%s removeCustomGesture: Invalid Custom GestureID (%02X)! ERROR %08X\n", __func__, gestureID, ERROR_OP_NOT_ALLOW);
		return ERROR_OP_NOT_ALLOW;
	}

	res = fts_writeFwCmd(cmd, 3);					//when a gesture is removed, it is also disabled automatically
	if (res < OK) {
		TS_LOG_ERR("%s removeCustomGesture: Impossible to remove custom gesture ID = %02X! ERROR %08X\n",__func__, gestureID, res);
		return res;
	}

	res = pollForEvent(event_to_search, 4, readData, GENERAL_TIMEOUT);
	if (res < OK) {
		TS_LOG_ERR("%s removeCustomGesture: remove event not found! ERROR %08X\n", __func__, res);
		return res;
	}

	if (readData[2] != gestureID || readData[4] != 0x00) {			//check of gestureID is redundant
		TS_LOG_ERR("%s removeCustomGesture: remove event status not OK! ERROR %08X\n", __func__, readData[4]);
		return ERROR_GESTURE_REMOVE;
	}


	custom_gesture_index[index] = 0;
	TS_LOG_INFO("%s Custom Gesture Remove procedure DONE!\n", __func__);
	return OK;

}

int isAnyGestureActive(void) {
	int res = 0;

	while (res < (GESTURE_MASK_SIZE-1) && gesture_mask[res] == 0) {		//-1 because in any case the last gesture mask byte will be evaluated with the following if
		res++;
	}

	if (gesture_mask[res] != 0) {
		TS_LOG_INFO("%s: Active Gestures Found! gesture_mask[%d] = %02X !\n", __func__, res, gesture_mask[res]);
		return FEAT_ENABLE;
	}
	else {
		TS_LOG_INFO("%s: All Gestures Disabled!\n", __func__);
		return FEAT_DISABLE;
	}
}

int gestureIDtoGestureMask(u8 id, u8 *mask) {
	TS_LOG_INFO("%s: Index = %d Position = %d !\n", __func__, ((int)((id) / 8)), (id % 8));
	mask[((int)((id) / 8))] |= 0x01 << (id % 8);
	return OK;
}

int readGestureCoords(u8 *event){
	int i = 0;
	u8 rCmd[3] = {FTS_CMD_FRAMEBUFFER_R, 0x00, 0x00 };
	int res;

	unsigned char val[GESTURE_COORDS_REPORT_MAX*4+1];			//the max coordinates to read are GESTURE_COORDS_REPORT_MAX*4(because each coordinate is a short(*2) and we have x and y) + dummy byte

	if(event[0]==EVENTID_GESTURE && event[1] == EVENT_TYPE_GESTURE_DTC2) {
		rCmd[1] = event[4];    // Offset address L
		rCmd[2] = event[3];    // Offset address H
		gesture_coords_reported = event[6];	    //number of coords reported L
		gesture_coords_reported = (gesture_coords_reported << 8) | event[5]; //number of coords reported H
		if(gesture_coords_reported>GESTURE_COORDS_REPORT_MAX){
			TS_LOG_ERR("%s:  FW reported more than %d points for the gestures! Decreasing to %d\n", __func__, gesture_coords_reported,GESTURE_COORDS_REPORT_MAX);
			gesture_coords_reported = GESTURE_COORDS_REPORT_MAX;
		}

		TS_LOG_INFO("%s: Offset: %02X %02X points = %d\n",__func__, rCmd[1], rCmd[2], gesture_coords_reported);
		res = fts_readCmd(rCmd, 3, (unsigned char *)val, 1 + (gesture_coords_reported*2));
		if (res<OK)
		{
			TS_LOG_ERR("%s: Cannot read the coordinates! ERROR %08X \n", __func__, res);
			gesture_coords_reported = ERROR_OP_NOT_ALLOW;
			return res;
		}
		//all the points of the gesture are stored in val
		for(i = 0;i < gesture_coords_reported;i++)
		{	//ignore first byte data because it is a dummy byte
			gesture_coordinates_x[i] =  (((u16) val[i*2 + 1 + 1]) & 0x0F) << 8 | (((u16) val[i*2 + 1])& 0xFF);
			gesture_coordinates_y[i] =  (((u16) val[gesture_coords_reported*2 + i*2 + 1 + 1]) & 0x0F) << 8 | (((u16) val[gesture_coords_reported*2 + i*2 + 1]) & 0xFF);
		}

		TS_LOG_ERR("%s: Reading Gesture Coordinates DONE! \n", __func__);
		return OK;

	} else {
		TS_LOG_ERR("%s: The event passsed as argument is invalid! ERROR %08X \n", __func__, ERROR_OP_NOT_ALLOW);
		return ERROR_OP_NOT_ALLOW;
	}

}

int getGestureCoords(u16 *x, u16 *y){
	x = gesture_coordinates_x;
	y = gesture_coordinates_y;
	TS_LOG_ERR("%s: Number of gesture coordinates returned = %d \n", __func__, gesture_coords_reported);
	return gesture_coords_reported;
}


