/*
 * fts.c
 *
 * FTS Capacitive touch screen controller (FingerTipS)
 *
 * Copyright (C) 2016, STMicroelectronics Limited.
 * Authors: AMG(Analog Mems Group)
 *
 * 		marco.cali@st.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
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

#include <linux/notifier.h>
#include <linux/fb.h>

#include "fts.h"
#include "fts_lib/ftsCompensation.h"
#include "fts_lib/ftsIO.h"
#include "fts_lib/ftsError.h"
#include "fts_lib/ftsFlash.h"
#include "fts_lib/ftsFrame.h"
#include "fts_lib/ftsGesture.h"
#include "fts_lib/ftsTest.h"
#include "fts_lib/ftsTime.h"
#include "fts_lib/ftsTool.h"

#if defined (CONFIG_TEE_TUI)
#include "tui.h"
#endif

struct fts_ts_info *fts_info;
static struct completion st_roi_data_done;
static u8 st_roi_data_staled;
static u8 st_roi_switch_on;
static unsigned char st_roi_data[ROI_DATA_READ_LENGTH] = { 0 };

/*
 * Event installer helpers
 */
#define event_id(_e)     EVENTID_##_e
#define handler_name(_h) fts_##_h##_event_handler

#define install_handler(_i, _evt, _hnd) \
do { \
		_i->event_dispatch_table[event_id(_evt)] = handler_name(_hnd); \
} while (0)


#if defined(SCRIPTLESS) || defined(DRIVER_TEST)
static struct class *fts_cmd_class = NULL;
#endif

extern chipInfo ftsInfo;

static u32 typeOfComand[CMD_STR_LEN] = {0};
static int numberParameters = 0;
#ifdef PHONE_GESTURE
static u8 mask[GESTURE_MASK_SIZE+2];
extern u16 gesture_coordinates_x[GESTURE_COORDS_REPORT_MAX];
extern u16 gesture_coordinates_y[GESTURE_COORDS_REPORT_MAX];
extern int gesture_coords_reported;
extern struct mutex gestureMask_mutex;
#ifdef USE_CUSTOM_GESTURES
static int custom_gesture_res;
#endif
#endif
#ifdef USE_NOISE_PARAM
static u8 noise_params[NOISE_PARAMETERS_SIZE] = {0};
#endif

#if defined (CONFIG_TEE_TUI)
extern struct ts_tui_data tee_tui_data;
#endif

#define TYPE_ID_2DBARCODE 1
#define TYPE_ID_2DBARCODE_LENGTH 3
#define STATUS_GLOVE_MODE 6
#define STATUS_FINGER_MODE 1
#define BUFF_SEEK_BIT 1 //for buf[0] is real size

int check_MutualRawResGap(void);
struct fts_ts_info* fts_get_info(void)
{
	return fts_info;
}

static int fts_set_info(struct fts_ts_info* info)
{
	fts_info = info;
	return 0;
}

unsigned int le_to_uint(const unsigned char *ptr)
{
	return (unsigned int) ptr[0] + (unsigned int) ptr[1] * 0x100;
}

unsigned int be_to_uint(const unsigned char *ptr)
{
	return (unsigned int) ptr[1] + (unsigned int) ptr[0] * 0x100;
}

static int fts_command(struct fts_ts_info *info, unsigned char cmd)
{
	unsigned char regAdd;
	int ret;

	regAdd = cmd;

	ret = fts_writeCmd(&regAdd, sizeof (regAdd)); // 0 = ok

	TS_LOG_INFO( "%s Issued command 0x%02x, return value %08X\n", __func__, cmd, ret);

	return ret;
}


void fts_input_report_key(struct fts_ts_info *info, int key_code)
{
	input_report_key(info->input_dev, key_code, 1);
	input_sync(info->input_dev);
	input_report_key(info->input_dev, key_code, 0);
	input_sync(info->input_dev);
}


/*
 * New Interrupt handle implementation
 */

static void st_set_event_to_fingers(struct fts_ts_info *info, int x, int y, int major,
	int minor, int pressure, int status, unsigned char touchId, unsigned char touchcount)
{
	struct ts_fingers *f_info = info->fingers_info;

	TS_LOG_DEBUG("%s: z:%d, status:%d, touchId:%u, touchcount:%u\n",
		__func__, pressure, status, touchId, touchcount);

	if (touchId <= 0 || touchId >= 10){
		touchId = 0;
	}

	if (0xEFFE == f_info->fingers[touchId].pressure) {
		TS_LOG_INFO("Not handle pointer event, has handled leave event\n");
//		return;
	}

	memset(&f_info->fingers[touchId], 0, sizeof(f_info->fingers[touchId]));
	f_info->cur_finger_number = 0;

	f_info->fingers[touchId].x = x;
	f_info->fingers[touchId].y = y;
	f_info->fingers[touchId].ewx = major;
	f_info->fingers[touchId].ewy = minor;
	f_info->fingers[touchId].pressure = pressure;
	f_info->fingers[touchId].status = status;
	f_info->cur_finger_number += touchcount;

	return;
}
static void st_set_ext_event_to_fingers(struct fts_ts_info *info, int wx, int wy, int xer,
	int yer, int pressure, int status, unsigned char touchId, unsigned char touchcount)
{
	struct ts_fingers *f_info = info->fingers_info;

	TS_LOG_DEBUG("%s: wx:%d, wy:%d, xer:%d, yer:%d, touchId:%u, touchcount:%u\n",
		__func__, wx, wy, xer, yer, touchId, touchcount);

	if (touchId <= 0 || touchId >= 10){
		touchId = 0;
	}
	f_info->fingers[touchId].wx = wx;
	f_info->fingers[touchId].wy = wy;
	f_info->fingers[touchId].xer= xer;
	f_info->fingers[touchId].yer = yer;
	return;
}
/*
 * New Interrupt handle implementation
 */

static inline unsigned char *fts_next_event(unsigned char *evt) {
	/* Nothing to do with this event, moving to the next one */
	evt += FIFO_EVENT_SIZE;

	/* the previous one was the last event ?  */
	return (evt[-1] & 0x1F) ? evt : NULL;
}

/* EventId : 0x00 */
static void fts_nop_event_handler(struct fts_ts_info *info,
		unsigned char *event) {
}
static u8 g_pressure;
/* EventId : 0x0D */
static void fts_pressure_event_handler(struct fts_ts_info *info,
		unsigned char *event)
{
	g_pressure = event[3];
	TS_LOG_DEBUG("%s:pressure event: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
		__func__, event[0], event[1], event[2], event[3],event[4]);
}

/* EventId : 0x03 */
static void fts_enter_pointer_event_handler(struct fts_ts_info *info,
		unsigned char *event) {
	unsigned char touchId = 0, touchcount = 0;
	int x = 0;
	int y = 0;
	int z = 0;
	int major = 0;
	int minor = 0;
	int status = 0;
	int status1 = 0;
	u8 touchsize = 0;

	if (!info->resume_bit)
		return;

	touchId = event[1] & 0x0F;
	touchcount = (event[1] & 0xF0) >> 4;
	touchsize = (event[5] & 0xC0) >> 6;

	__set_bit(touchId, &info->touch_id);

	x = (event[2] << 4) | (event[4] & 0xF0) >> 4;
	y = (event[3] << 4) | (event[4] & 0x0F);
	z = (event[5] & 0x3F);
	major = (event[5] & 0x1F);		// bit0-bit4: major
	minor = event[6];
	status = (event[5] & 0xE0) >> 5;	//bit5-bit7:finger status// event6:minor

	if(status == 0)
		status1 = STATUS_GLOVE_MODE;
	else if(status == 4)
		status1 = STATUS_FINGER_MODE;
	else
		TS_LOG_ERR("[%s] status error 0x%x\n",__func__,status);

	TS_LOG_DEBUG("[%s] coor status from 0x%x to  0x%x\n",__func__,status,status1);

	if (x == X_AXIS_MAX)
		x--;

	if (y == Y_AXIS_MAX)
		y--;

	st_set_event_to_fingers(info, x, y, major, minor, 1, status1, touchId, touchcount);

	if (st_roi_switch_on)
		st_roi_data_staled = 1;
}

/* EventId : 0x04 */
static void fts_leave_pointer_event_handler(struct fts_ts_info *info,
		unsigned char *event) {
	unsigned char touchId, touchcount;
	u8 touchsize;
	int major;
	int minor;
	touchId = event[1] & 0x0F;
	touchcount = (event[1] & 0xF0) >> 4;
	touchsize = (event[5] & 0xC0) >> 6;

	__clear_bit(touchId, &info->touch_id);

	touchId = event[1] & 0x0F;
	touchcount = (event[1] & 0xF0) >> 4;
	major = (event[5] & 0x1F);		// bit0-bit4: major
	minor = event[6];
	st_set_event_to_fingers(info, 0, 0, major,minor, 0xEFFE, 1, touchId, touchcount);
}

/* EventId : 0x05 */
#define fts_motion_pointer_event_handler fts_enter_pointer_event_handler

/* EventId : 0x28 */
static void fts_ext_pointer_event_handler(struct fts_ts_info *info,unsigned char *event)
{
	unsigned char touchId, touchcount;
	u8 touchsize;
	int wx;
	int wy;
	int xer;
	int yer;
	int evt_fifo_stat;
	touchId = event[1] & 0x0F;
	touchcount = (event[1] & 0xF0) >> 4;
	__clear_bit(touchId, &info->touch_id);
	wx =  (event[2] & 0xF0) >> 4;
	wy =  event[2] & 0x0F;
	xer = event[3];
	yer = event[4];
	evt_fifo_stat = event[7];
	st_set_ext_event_to_fingers(info, wx,wy, xer,yer, 0xEFFE, 1, touchId, touchcount);
}
static int fts_mode_handler(struct fts_ts_info *info,int force)
{
	int res = OK;
	int ret = OK;

	/*
	 * initialize the mode to Nothing in order to be updated
	 * depending on the features enabled
	 */
	info->mode = MODE_NOTHING; 

	TS_LOG_INFO( "%s: Mode Handler starting, resume_bit=0x%x\n", __func__, info->resume_bit);
	switch(info->resume_bit){
	case 0:/* screen down */
		/*
		 * Do sense off in order to avoid the flooding of the fifo with touch
		 * events if someone is touching the panel during suspend
		 */

		/*
		 * for support force key in suspend, shoud not sense off
		 *TS_LOG_INFO( "%s: Sense OFF!\n", __func__);
		 res |=fts_command(info, FTS_CMD_MS_MT_SENSE_OFF); //we need to use fts_command for speed reason (no need to check echo in this case and interrupt can be enabled)
		 */
#ifdef PHONE_GESTURE
		if (info->gesture_enabled==1) {
			TS_LOG_INFO( "%s: enter in gesture mode !\n", __func__);
			ret=enterGestureMode(isSystemResettedDown());
			if (ret >= OK) {
				info->mode |= FEAT_GESTURE;
			}
			else {
				TS_LOG_ERR("%s: enterGestureMode failed! %08X recovery in senseOff...\n", __func__,ret);
			}
			res|=ret;
		}
#endif
		if(info->mode != (FEAT_GESTURE|MODE_NOTHING) || info->gesture_enabled==0){

		info->mode |= MODE_SENSEOFF;

		}
		setSystemResettedDown(0);
		break;

	case 1:	/* screen up */
		TS_LOG_INFO("%s: Screen ON\n", __func__);

#ifdef FEAT_GLOVE
		if((info->glove_enabled == FEAT_ENABLE && isSystemResettedUp()) || force==1){
			TS_LOG_INFO( "%s: Glove Mode setting...\n", __func__);
			ret=featureEnableDisable(info->glove_enabled,FEAT_GLOVE);
			if(ret<OK){
				TS_LOG_ERR("%s: error during setting GLOVE_MODE! %08X\n", __func__,ret);
			}
			res |=ret;

			if(ret>=OK && info->glove_enabled == FEAT_ENABLE){
				info->mode |= FEAT_GLOVE;
				TS_LOG_ERR("%s: GLOVE_MODE Enabled!\n", __func__);
			}else{
				TS_LOG_ERR("%s: GLOVE_MODE Disabled!\n", __func__);
			}

		}

#endif
#ifdef FEAT_COVER
		if((info->cover_enabled == FEAT_ENABLE && isSystemResettedUp()) || force==1){
				TS_LOG_INFO( "%s: Cover Mode setting...\n", __func__);
				ret=featureEnableDisable(info->cover_enabled,FEAT_COVER);
				if(ret<OK){
					TS_LOG_ERR("%s: error during setting COVER_MODE! %08X\n", __func__,ret);
				}
				res |=ret;

			if(ret>=OK && info->cover_enabled == FEAT_ENABLE){
					info->mode |= FEAT_COVER;
					TS_LOG_ERR("%s: COVER_MODE Enabled!\n", __func__);
			}else{
				TS_LOG_ERR("%s: COVER_MODE Disabled!\n", __func__);
			}

		}
#endif
#ifdef FEAT_CHARGER
		if((info->charger_enabled == FEAT_ENABLE && isSystemResettedUp()) || force==1){
			TS_LOG_INFO( "%s: Charger Mode setting...\n", __func__);
			ret=featureEnableDisable(info->charger_enabled,FEAT_CHARGER);
			if(ret<OK){
				TS_LOG_ERR("%s: error during setting CHARGER_MODE! %08X\n", __func__,ret);
			}
			res |=ret;

			if(ret>=OK && info->charger_enabled == FEAT_ENABLE){
				info->mode |= FEAT_CHARGER;
				TS_LOG_ERR("%s: CHARGER_MODE Enabled!\n", __func__);
			}else{
				TS_LOG_ERR("%s: CHARGER_MODE Disabled!\n", __func__);
			}

		}
#endif
#ifdef FEAT_EDGE_REJECTION
		if((info->edge_rej_enabled == FEAT_ENABLE && isSystemResettedUp()) || force==1){
			TS_LOG_INFO( "%s: Edge Rejection Mode setting...\n", __func__);
			ret=featureEnableDisable(info->edge_rej_enabled,FEAT_EDGE_REJECTION);
			if(ret<OK){
				TS_LOG_ERR("%s: error during setting EDGE_REJECTION_MODE! %08X\n", __func__,ret);
			}
			res |=ret;

			if(ret>=OK && info->edge_rej_enabled == FEAT_ENABLE){
				info->mode |= FEAT_EDGE_REJECTION;
				TS_LOG_ERR("%s: EDGE_REJECTION_MODE Enabled!\n", __func__);
			}else{
				TS_LOG_ERR("%s: EDGE_REJECTION_MODE Disabled!\n", __func__);
			}
		}
#endif
#ifdef FEAT_CORNER_REJECTION
		if((info->corner_rej_enabled == FEAT_ENABLE && isSystemResettedUp()) || force==1){
			TS_LOG_INFO( "%s: Corner rejection Mode setting...\n", __func__);
			ret=featureEnableDisable(info->corner_rej_enabled,FEAT_CORNER_REJECTION);
			if(ret<OK){
				TS_LOG_ERR("%s: error during setting CORNER_REJECTION_MODE! %08X\n", __func__,ret);
			}
				res |=ret;

			if(ret>=OK && info->corner_rej_enabled == FEAT_ENABLE){
				info->mode |= FEAT_CORNER_REJECTION;
				TS_LOG_ERR("%s: CORNER_REJECTION_MODE Enabled!\n", __func__);
			}else{
				TS_LOG_ERR("%s: CORNER_REJECTION_MODE Disabled!\n", __func__);
			}

		}
#endif
#ifdef FEAT_EDGE_PALM_REJECTION
		if((info->edge_palm_rej_enabled == FEAT_ENABLE && isSystemResettedUp()) || force==1){
			TS_LOG_INFO( "%s: Edge Palm rejection Mode setting...\n", __func__);
			ret=featureEnableDisable(info->edge_palm_rej_enabled,FEAT_EDGE_PALM_REJECTION);
			if(ret<OK){
				TS_LOG_ERR("%s: error during setting EDGE_PALM_REJECTION_MODE! %08X\n", __func__,ret);
			}
				res |=ret;

			if(ret>=OK && info->edge_palm_rej_enabled == FEAT_ENABLE){
				info->mode |= FEAT_EDGE_PALM_REJECTION;
				TS_LOG_ERR("%s: EDGE_PALM_REJECTION_MODE Enabled!\n", __func__);
			}else{
				TS_LOG_ERR("%s: EDGE_PALM_REJECTION_MODE Disabled!\n", __func__);
			}

		}
#endif
		TS_LOG_INFO("%s: Sense ON\n", __func__);
		res |=fts_command(info, FTS_CMD_MS_MT_SENSE_ON);
		info->mode |= MODE_SENSEON;

		setSystemResettedUp(0);
		break;

		default:
			TS_LOG_ERR("%s: invalid resume_bit value = %d! %08X\n", __func__,info->resume_bit,ERROR_OP_NOT_ALLOW);
			res = ERROR_OP_NOT_ALLOW;
	}


	TS_LOG_INFO( "%s: Mode Handler finished! res = %08X\n", __func__, res);
	return res;

}


/* EventId : 0x0F */
static void fts_error_event_handler(struct fts_ts_info *info,
		unsigned char *event) {
	int error = 0;
	TS_LOG_INFO( "%s Received event %02X %02X %02X %02X %02X %02X %02X %02X\n",
			 __func__, event[0], event[1], event[2], event[3], event[4], event[5], event[6], event[7]);

	switch (event[1]) {
	case EVENT_TYPE_ESD_ERROR: //esd

		fts_chip_powercycle(info);

		error = fts_system_reset();
		error |= fts_mode_handler(info,0);
		error |= fts_enableInterrupt();
		if (error < OK) {
			TS_LOG_ERR("%s Cannot restore the device %08X\n", __func__, error);
		}
		break;
	case EVENT_TYPE_WATCHDOG_ERROR: //watch dog timer
		dumpErrorInfo();
		error = fts_system_reset();
		error |= fts_mode_handler(info,0);
		error |= fts_enableInterrupt();
		if (error < OK) {
			TS_LOG_ERR("%s Cannot reset the device %08X\n", __func__, error);
		}
		break;
	}
}

/* EventId : 0x10 */
static void fts_controller_ready_event_handler(
		struct fts_ts_info *info, unsigned char *event)
{
	int error;
	TS_LOG_INFO( "%s Received event 0x%02x\n", __func__, event[0]);
	setSystemResettedUp(1);
	setSystemResettedDown(1);
	error = fts_mode_handler(info,0);
	if (error < OK) {
		 TS_LOG_ERR("%s Cannot restore the device status %08X\n", __func__, error);
	}
}

/* EventId : 0x16 */
static void fts_status_event_handler(
		struct fts_ts_info *info, unsigned char *event)
{
	switch (event[1]) {
		case EVENT_TYPE_MS_TUNING_CMPL:
		case EVENT_TYPE_SS_TUNING_CMPL:
		case FTS_FORCE_CAL_SELF_MUTUAL:
		case FTS_FLASH_WRITE_CONFIG:
		case FTS_FLASH_WRITE_COMP_MEMORY:
		case FTS_FORCE_CAL_SELF:
		case FTS_WATER_MODE_ON:
		case FTS_WATER_MODE_OFF:
		default:
			TS_LOG_ERR("%s Received unhandled status event = %02X %02X %02X %02X %02X %02X %02X %02X\n",
					__func__, event[0], event[1], event[2], event[3],
					event[4], event[5], event[6], event[7]);
			break;
	}
}

#ifdef PHONE_GESTURE

static void fts_gesture_event_handler(struct fts_ts_info *info, unsigned char *event)
{
	unsigned char touchId;
	int value;
	int needCoords = 0;

	TS_LOG_INFO("%s  gesture event data: %02X %02X %02X %02X %02X %02X %02X %02X\n",
		__func__, event[0], event[1], event[2], event[3], event[4],
		event[5], event[6], event[7]);

	if (event[1] == 0x03) {

		TS_LOG_ERR("%s: Gesture ID %02X enable_status = %02X\n", __func__, event[2],event[3]);

	}

	if (event[1] == EVENT_TYPE_ENB && event[2] == 0x00) {
		switch (event[3]) {
		case GESTURE_ENABLE:
			TS_LOG_ERR("%s: Gesture Enabled! res = %02X\n", __func__, event[4]);
			break;

		case GESTURE_DISABLE:
			TS_LOG_ERR("%s: Gesture Disabled! res = %02X\n", __func__, event[4]);
			break;

		default:
			TS_LOG_ERR("%s: Event not Valid!\n", __func__);

		}

	}


	if (event[0] == EVENTID_GESTURE && (event[1] == EVENT_TYPE_GESTURE_DTC1 ||
			event[1] == EVENT_TYPE_GESTURE_DTC2)) {
		/* always use touchId zero */
		touchId = 0;
		__set_bit(touchId, &info->touch_id);

		needCoords = 1;	//by default read the coordinates for all gestures excluding double tap

		switch (event[2]) {
			case GES_ID_DBLTAP:
				value = KEY_WAKEUP;
				TS_LOG_INFO( "%s: double tap !\n", __func__);
				needCoords = 0;
			break;

			case GES_ID_AT:
				value = KEY_WWW;
				TS_LOG_INFO( "%s: @ !\n", __func__);
				break;

			case GES_ID_C:
				value = KEY_C;
				TS_LOG_INFO( "%s: C !\n", __func__);
				break;

			case GES_ID_E:
				value = KEY_E;
				TS_LOG_INFO( "%s: e !\n", __func__);
				break;

			case GES_ID_F:
				value = KEY_F;
				TS_LOG_INFO( "%s: F !\n", __func__);
				break;

			case GES_ID_L:
				value = KEY_L;
				TS_LOG_INFO( "%s: L !\n", __func__);
				break;

			case GES_ID_M:
				value = KEY_M;
				TS_LOG_INFO( "%s: M !\n", __func__);
				break;

			case GES_ID_O:
				value = KEY_O;
				TS_LOG_INFO( "%s: O !\n", __func__);
				break;

			case GES_ID_S:
				value = KEY_S;
				TS_LOG_INFO( "%s: S !\n", __func__);
				break;

			case GES_ID_V:
				value = KEY_V;
				TS_LOG_INFO( "%s:  V !\n", __func__);
				break;

			case GES_ID_W:
				value = KEY_W;
				TS_LOG_INFO( "%s:  W !\n", __func__);
				break;

			case GES_ID_Z:
				value = KEY_Z;
				TS_LOG_INFO( "%s:  Z !\n", __func__);
				break;

			case GES_ID_HFLIP_L2R:
				value = KEY_RIGHT;
				TS_LOG_INFO( "%s:  -> !\n", __func__);
				break;

			case GES_ID_HFLIP_R2L:
				value = KEY_LEFT;
				TS_LOG_INFO( "%s:  <- !\n", __func__);
				break;

			case GES_ID_VFLIP_D2T:
				value = KEY_UP;
				TS_LOG_INFO( "%s:  UP !\n", __func__);
				break;

			case GES_ID_VFLIP_T2D:
				value = KEY_DOWN;
				TS_LOG_INFO( "%s:  DOWN !\n", __func__);
				break;

			case GES_ID_CUST1:
				value = KEY_F1;
				TS_LOG_INFO( "%s:  F1 !\n", __func__);
				break;

			case GES_ID_CUST2:
				value = KEY_F1;
				TS_LOG_INFO( "%s:  F2 !\n", __func__);
				break;

			case GES_ID_CUST3:
				value = KEY_F3;
				TS_LOG_INFO( "%s:  F3 !\n", __func__);
				break;

			case GES_ID_CUST4:
				value = KEY_F1;
				TS_LOG_INFO( "%s:  F4 !\n", __func__);
				break;

			case GES_ID_CUST5:
				value = KEY_F1;
				TS_LOG_INFO( "%s:  F5 !\n", __func__);
				break;

			case GES_ID_LEFTBRACE:
				value = KEY_LEFTBRACE;
				TS_LOG_INFO( "%s:  < !\n", __func__);
				break;

			case GES_ID_RIGHTBRACE:
				value = KEY_RIGHTBRACE;
				TS_LOG_INFO( "%s:  > !\n", __func__);
				break;
			default:
				TS_LOG_INFO( "%s:  No valid GestureID!\n", __func__);
				goto gesture_done;

		}


		if(event[1] == EVENT_TYPE_GESTURE_DTC1)		//no coordinates for gestures reported by FW
			needCoords = 0;

		if(needCoords == 1)
			readGestureCoords(event);

		fts_input_report_key(info, value);

	gesture_done:
		/* Done with gesture event, clear bit. */
		__clear_bit(touchId, &info->touch_id);
	}
}
#endif

/* EventId : 0x05 */
#define fts_motion_pointer_event_handler fts_enter_pointer_event_handler

/*
 * This handler is called each time there is at least
 * one new event in the FIFO
 */
static void fts_event_handler(struct fts_ts_info *info,
				int *touch_event_count) {
	int error=0, count=0;
	unsigned char regAdd;
	unsigned char data[FIFO_EVENT_SIZE] = {0};
	unsigned char event_id;

	event_dispatch_handler_t event_handler;

	/*
	 * read all the FIFO and parsing events
	 */
	regAdd = FIFO_CMD_READONE;

	for(count=0; count<FIFO_DEPTH; count++){
		error = fts_readCmd(&regAdd, sizeof (regAdd), data, FIFO_EVENT_SIZE);
		if(error == OK && data[0]!= EVENTID_NO_EVENT)
			event_id = data[0];
		else
			break;

		if (event_id < EVENTID_LAST) {
			event_handler = info->event_dispatch_table[event_id];
			event_handler(info, (data));
		}
		if (event_id == EVENTID_ENTER_POINTER ||
			event_id == EVENTID_LEAVE_POINTER ||
			event_id == EVENTID_MOTION_POINTER)
			(*touch_event_count)++;
	}

}

static int cx_crc_check(void)
{
	unsigned char regAdd1[3] = {FTS_CMD_HW_REG_R, ADDR_CRC_BYTE0, ADDR_CRC_BYTE1};
	unsigned char val[2];
	unsigned char crc_status;
	int res;
#ifndef FTM3_CHIP
	u8 cmd[4] = { FTS_CMD_HW_REG_W, 0x00, 0x00, SYSTEM_RESET_VALUE };
	int event_to_search[2] = {(int)EVENTID_ERROR_EVENT, (int)EVENT_TYPE_CHECKSUM_ERROR} ;
	u8 readData[FIFO_EVENT_SIZE];
#endif

	res = fts_readCmd(regAdd1, sizeof (regAdd1), val, 2);		//read 2 bytes because the first one is a dummy byte!
	if (res < OK) {
		TS_LOG_ERR("%s Cannot read crc status %08X\n", __func__, res);
		return res;
	}

	crc_status = val[1] & CRC_MASK;
	if (crc_status != OK) // CRC error if crc_status!=0
	{
		TS_LOG_ERR("%s CRC = %X\n", __func__, crc_status);
		return crc_status;
	}

#ifndef FTM3_CHIP
	TS_LOG_ERR("%s: Verifying if Config CRC Error...\n", __func__);
	u16ToU8_be(SYSTEM_RESET_ADDRESS, &cmd[1]);
	res = fts_writeCmd(cmd, 4);
	if (res < OK) {
		TS_LOG_ERR("%s Cannot send system resest command %08X\n", __func__, res);
		return res;
	}
	setSystemResettedDown(1);
	setSystemResettedUp(1);
	res = pollForEvent(event_to_search, 2, readData, GENERAL_TIMEOUT);
	if ( res < OK) {
		TS_LOG_ERR("%s cx_crc_check: No Config CRC Found!\n",__func__);
	}else{
		if(readData[2] == CRC_CONFIG_SIGNATURE || readData[2] == CRC_CONFIG){
			TS_LOG_ERR("%s cx_crc_check: CRC Error for config found! CRC = %02X\n",__func__, readData[2]);
			return readData[2];
		}
	}

#endif

	return OK; //return OK if no CRC error, or a number >OK if crc error
}

static int fts_init_afterProbe(struct fts_ts_info *info) {
	int error = 0;

	error = cleanUp(0);					//system reset
	error |= fts_mode_handler(info,0);			//enable the features and the sensing
	error |= fts_enableInterrupt();			//enable the interrupt

	if (error < OK)
		TS_LOG_ERR("%s Init after Probe error (ERROR = %08X)\n", __func__, error);


	return error;
}

static int generate_fw_name(int force, char *fw_name)
{
	struct fts_ts_info *info = fts_get_info();

	if (force)
		snprintf(fw_name, ST_FW_NAME_MAX_LEN -1 , "%s", ST_FW_PATH_SD);
	else
		snprintf(fw_name, ST_FW_NAME_MAX_LEN -1 , "ts/%s_st_%s.bin",
			info->chip_data->ts_platform_data->product_name,
			info->project_id);

	return 0;
}

static int fts_enable_force_key(void)
{
	char cmd = 0x9B; /* force ker enable cmd */

	TS_LOG_INFO("%s: force key enable\n", __func__);
	fts_writeCmd(&cmd, 1);
	return 0;
}

#define FTS_FW_UPDATE_RETRYCOUNT 2

static int fts_fw_update_auto(struct fts_ts_info *info, int force)
{
#ifndef FTM3_CHIP
	u8 cmd[4] = { FTS_CMD_HW_REG_W, 0x00, 0x00, SYSTEM_RESET_VALUE };
	int event_to_search[2] = {(int)EVENTID_ERROR_EVENT, (int)EVENT_TYPE_CHECKSUM_ERROR} ;
	u8 readData[FIFO_EVENT_SIZE];
	int flag_init = 0;
#endif
	int retval = 0;
	int ret;

	int crc_status = 0;
	int error = 0;
	char fw_name[ST_FW_NAME_MAX_LEN] = {'\0'};
	int retry_count = 0;

start_update_fw:

	TS_LOG_ERR("%s Fw Auto Update is starting...\n", __func__);

	retry_count++;

	generate_fw_name(force, fw_name);
	TS_LOG_INFO("%s:fw name:%s\n", __func__, fw_name);

	/* check CRC status */
	ret = cx_crc_check();
	if (ret > OK && ftsInfo.u16_fwVer == 0x0000) {
		TS_LOG_ERR("%s: CRC Error or NO FW!\n", __func__);
		crc_status = 1;
	} else {
		crc_status = 0;
		TS_LOG_ERR("%s: NO CRC Error or Impossible to read CRC register!\n", __func__);
	}

	retval = flashProcedure(fw_name, force || crc_status , true);

#ifndef FTM3_CHIP
	TS_LOG_ERR("%s: Verifying if CX CRC Error\n", __func__);
	u16ToU8_be(SYSTEM_RESET_ADDRESS, &cmd[1]);
	ret = fts_writeCmd(cmd, 4);
	if (ret < OK) {
		TS_LOG_ERR("%s Cannot send system reset command %08X\n", __func__, ret);
	} else {
		 setSystemResettedDown(1);
		 setSystemResettedUp(1);
		 ret = pollForEvent(event_to_search, 2, readData, GENERAL_TIMEOUT);
		 if ( ret < OK) {
			TS_LOG_ERR("%s: No CX CRC Found!\n", __func__);
		 }else{
			if(readData[2] == CRC_CX_MEMORY){
				TS_LOG_ERR("%s: CRC Error for CX found! CRC = %02X\n", __func__, readData[2]);
				flag_init=1;
			}
		 }
	}
#endif
	error = fts_init_afterProbe(info);
	if (error < OK) {
		TS_LOG_ERR("%s Cannot initialize the hardware device %08X\n", __func__, error);
	}

	/*
	 * When fw damaged, ST IC not able to read projecet id and driver will
	 * update default fw to IC. Driver do project id check again after fw
	 * updated and upadte again with right fw if necessary.
	 */
	char origin_id[ST_PROJECT_ID_LEN + 1] = {'\0'};
	strncpy(origin_id, info->project_id, ST_PROJECT_ID_LEN);

	st_get_project_id(info);
	if (retry_count <= FTS_FW_UPDATE_RETRYCOUNT
		&& strcmp(origin_id, info->project_id)) {
		TS_LOG_ERR("%s: reupdate fw wite real project id\n", __func__);
		goto start_update_fw;
	}

	fts_enable_force_key();
	snprintf(info->chip_data->version_name, MAX_STR_LEN,"%x.%x", ftsInfo.u16_fwVer, ftsInfo.u16_cfgId);

	TS_LOG_ERR("%s Fw Auto Update Finished!\n", __func__);
	return 0;
}

static int fts_interrupt_install(struct fts_ts_info *info)
{
	int i, error = 0;

	info->event_dispatch_table = kzalloc(
			sizeof (event_dispatch_handler_t) * EVENTID_LAST, GFP_KERNEL);

	if (!info->event_dispatch_table) {
		TS_LOG_ERR("%s OOM allocating event dispatch table\n", __func__);
		return -ENOMEM;
	}

	for (i = 0; i < EVENTID_LAST; i++)
		info->event_dispatch_table[i] = fts_nop_event_handler;

	install_handler(info, ENTER_POINTER, enter_pointer);
	install_handler(info, LEAVE_POINTER, leave_pointer);
	install_handler(info, MOTION_POINTER, motion_pointer);
	install_handler(info, ERROR_EVENT, error);
	install_handler(info, CONTROL_READY, controller_ready);
	install_handler(info, STATUS_UPDATE, status);
	install_handler(info, PRESSURE, pressure);
	install_handler(info, EXT_POINTER,ext_pointer);
#ifdef PHONE_GESTURE
	install_handler(info, GESTURE, gesture);
#endif

	/* disable interrupts in any case */
	error = fts_disableInterrupt();

	return error;
}

static ssize_t fts_strength_frame_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	char *p = (char *)buf;

	sscanf(p, "%d ", &typeOfComand[0]);

	TS_LOG_INFO("%s: Type of Strength Frame selected: %d\n", __func__, typeOfComand[0]);
	return count;
}

static ssize_t fts_strength_frame_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	MutualSenseFrame frame;
	int res= ERROR_OP_NOT_ALLOW, count, j, size =6*2;
	u16 type = 0;
	char *all_strbuff = NULL;
	char buff[CMD_STR_LEN] = {0};
	struct fts_ts_info *info = fts_get_info();

	frame.node_data =NULL;

	res=fts_disableInterrupt();
		if(res<OK)
		goto END;

	res = senseOn();
	if(res<OK){
		TS_LOG_ERR("%s: could not start scanning! %08X\n", __func__, res);
		goto END;
	}
	mdelay(WAIT_FOR_FRESH_FRAMES);

	res = senseOff();
	if(res<OK){
		TS_LOG_ERR("%s: could not finish scanning! %08X\n", __func__, res);
		goto END;
	}

	mdelay(WAIT_AFTER_SENSEOFF);
	flushFIFO();

	switch(typeOfComand[0]){
	case 1:
		type = ADDR_NORM_TOUCH;
		break;
	default:
		TS_LOG_ERR("%s: Strength type %d not valid! %08X\n", __func__, typeOfComand[0], ERROR_OP_NOT_ALLOW);
		res = ERROR_OP_NOT_ALLOW;
		goto END;
	}

	res = getMSFrame(type, &frame, 0);
	if(res<OK){
		TS_LOG_ERR("%s: could not get the frame! %08X\n",__func__, res);
		goto END;
	}else{
		size += (res * 6);
		TS_LOG_INFO("%s The frame size is %d words\n", __func__, res);
		res = OK;
		print_frame_short("MS Strength frame =", array1dTo2d_short(frame.node_data, frame.node_data_size,
				frame.header.sense_node), frame.header.force_node, frame.header.sense_node);
	}

END:
	flushFIFO();
	fts_mode_handler(info,1);

	all_strbuff = (char *)kmalloc(size*sizeof(char),GFP_KERNEL);

	if(all_strbuff!=NULL) {
		memset(all_strbuff, 0, size);
		snprintf(buff, sizeof(buff), "%02X", 0xAA);
		strncat(all_strbuff, buff, size);

		snprintf(buff, sizeof (buff), "%08X", res);
		strncat(all_strbuff, buff, size);

		if (res >= OK) {
			snprintf(buff, sizeof (buff), "%02X", (u8) frame.header.force_node);
			strncat(all_strbuff, buff, size);

			snprintf(buff, sizeof (buff), "%02X", (u8) frame.header.sense_node);
			strncat(all_strbuff, buff, size);

			for (j = 0; j < frame.node_data_size; j++) {
				snprintf(buff, sizeof(buff), "%d,", frame.node_data[j]);
				strncat(all_strbuff, buff, size);
			}

			kfree(frame.node_data);
		}

		snprintf(buff, sizeof (buff), "%02X", 0xBB);
		strncat(all_strbuff, buff, size);

		count = snprintf(buf, TSP_BUF_SIZE, "%s\n", all_strbuff);
		kfree(all_strbuff);
	} else {
		TS_LOG_ERR("%s: Unable to allocate all_strbuff! %08X\n",
				__func__,ERROR_ALLOC);
	}

	fts_enableInterrupt();
	return count;

}
static ssize_t stm_fts_cmd_store(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t count)
{
	int n;
	char *p = (char *) buf;

	memset(typeOfComand, 0, CMD_STR_LEN * sizeof (u32));

	TS_LOG_ERR("%s\n", __func__);
	for (n = 0; n < (count + 1) / 3 && n < CMD_STR_LEN; n++) {
		sscanf(p, "%02X ", &typeOfComand[n]);
		p += 3;
		TS_LOG_ERR("%s typeOfComand[%d] = %02X\n", __func__, n, typeOfComand[n]);

	}

	numberParameters = n;
	TS_LOG_ERR("%s Number of Parameters = %d\n", __func__, numberParameters);
	return count;
}

static ssize_t stm_fts_cmd_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char buff[CMD_STR_LEN] = {0};
	int res, j, doClean = 0, count;

	int size = 20; /*default info len*/
	u8 *all_strbuff = NULL;

	MutualSenseData compData;
	SelfSenseData comData;
	MutualSenseFrame frameMS;
	SelfSenseFrame frameSS;
	u32 signature = 0;
	/*
	 * struct used for defining which test
	 * perform during the production test
	 */
	TestToDo todoDefault;

	todoDefault.MutualRaw = 1;
	todoDefault.MutualRawGap = 0;
	todoDefault.MutualCx1 = 0;
	todoDefault.MutualCx2 = 0;
	todoDefault.MutualCx2Adj = 0;
	todoDefault.MutualCxTotal = 0;
	todoDefault.MutualCxTotalAdj = 0;

	todoDefault.MutualKeyRaw = 0;
	todoDefault.MutualKeyCx1 = 0;
	todoDefault.MutualKeyCx2 = 0;
	todoDefault.MutualKeyCxTotal = 0;

	todoDefault.SelfForceRaw = 1;
	todoDefault.SelfForceRawGap = 0;
	todoDefault.SelfForceIx1 = 0;
	todoDefault.SelfForceIx2 = 0;
	todoDefault.SelfForceIx2Adj = 0;
	todoDefault.SelfForceIxTotal = 0;
	todoDefault.SelfForceIxTotalAdj = 0;
	todoDefault.SelfForceCx1 = 0;
	todoDefault.SelfForceCx2 = 0;
	todoDefault.SelfForceCx2Adj = 0;
	todoDefault.SelfForceCxTotal = 0;
	todoDefault.SelfForceCxTotalAdj = 0;

	todoDefault.SelfSenseRaw = 1;
	todoDefault.SelfSenseRawGap = 0;
	todoDefault.SelfSenseIx1 = 0;
	todoDefault.SelfSenseIx2 = 0;
	todoDefault.SelfSenseIx2Adj = 0;
	todoDefault.SelfSenseIxTotal = 0;
	todoDefault.SelfSenseIxTotalAdj = 0;
	todoDefault.SelfSenseCx1 = 0;
	todoDefault.SelfSenseCx2 = 0;
	todoDefault.SelfSenseCx2Adj = 0;
	todoDefault.SelfSenseCxTotal = 0;
	todoDefault.SelfSenseCxTotalAdj = 0;

	if (numberParameters <= 0) {
		TS_LOG_ERR("%s NO COMMAND SPECIFIED!!! do: 'echo [cmd_code] [args] > stm_fts_cmd' before looking for result!\n",
				 __func__);
		res = ERROR_OP_NOT_ALLOW;
		goto END;

	}

	res = fts_disableInterrupt();
	if (res < 0) {
		TS_LOG_INFO("%s fts_disableInterrupt: %08X\n", __func__, res);
		res = (res | ERROR_DISABLE_INTER);
		goto END;
	}
	switch (typeOfComand[0]) {
	/*ITO TEST*/
	case 0x01:
		res = production_test_ito(NULL);
		break;
	/*PRODUCTION TEST*/
	case 0x00:
		res = production_test_main(LIMITS_FILE, 1, 0, &todoDefault, INIT_MP,NULL,NULL);
		break;

	case 0x02:
		if(numberParameters>=5){
			signature = ((u8)typeOfComand[1])<<24 | ((u8)typeOfComand[2])<<16 |
					((u8)typeOfComand[3])<<8 | ((u8)typeOfComand[4]) ;
			TS_LOG_INFO("%s Signature to write = %08X\n", __func__, signature);
			res = production_test_initialization(signature,NULL);
			fts_system_reset();
		}else{
			TS_LOG_ERR("%s Wrong number of parameters... Missing Signature!\n", __func__);
			res = ERROR_OP_NOT_ALLOW;
		}
		break;

	case 0x20:
		TS_LOG_INFO("%s Value of signature = %08X\n", __func__, ftsInfo.u32_mpPassFlag);
		res = ftsInfo.u32_mpPassFlag;
		break;
	/*read mutual raw*/
	case 0x13:
		TS_LOG_INFO("%s Get 1 MS Frame\n", __func__);
		res = getMSFrame2(MS_TOUCH_ACTIVE, &frameMS);
		if (res < 0) {
			TS_LOG_INFO("%s Error while taking the MS frame... %02X\n", __func__, res);

		} else {
			TS_LOG_INFO("%s The frame size is %d words\n", __func__, res);
			size = (res * sizeof (short) + 8)*2;
			/* set res to OK because if getMSFrame is
			   successful res = number of words read
			 */
			res = OK;
			print_frame_short("MS frame =", array1dTo2d_short(frameMS.node_data, frameMS.node_data_size, frameMS.header.sense_node), frameMS.header.force_node, frameMS.header.sense_node);
		}
		break;
	/*read self raw*/
	case 0x15:
		TS_LOG_INFO("%s Get 1 SS Frame\n", __func__);
		res = getSSFrame2(SS_TOUCH, &frameSS);

		if (res < OK) {
			TS_LOG_INFO("%s Error while taking the SS frame... %02X\n", __func__, res);

		} else {
			TS_LOG_INFO("%s The frame size is %d words\n", __func__, res);
			size = (res * sizeof (short) + 8)*2+1;
			/*
			 * set res to OK because if getMSFrame is
			 *  successful res = number of words read
			 */
			res = OK;
			print_frame_short("SS force frame =", array1dTo2d_short(frameSS.force_data, frameSS.header.force_node, 1), frameSS.header.force_node, 1);
			print_frame_short("SS sense frame =", array1dTo2d_short(frameSS.sense_data, frameSS.header.sense_node, frameSS.header.sense_node), 1, frameSS.header.sense_node);
		}

				break;

	case 0x14: //read mutual comp data
		TS_LOG_INFO("%s Get MS Compensation Data\n", __func__);
		res = readMutualSenseCompensationData(MS_TOUCH_ACTIVE, &compData);

		if (res < 0) {
			TS_LOG_INFO("%s Error reading MS compensation data %02X\n", __func__, res);
		} else {
			TS_LOG_INFO("%s MS Compensation Data Reading Finished!\n", __func__);
			size = ((compData.node_data_size + 9) * sizeof (u8))*2;
			print_frame_u8("MS Data (Cx2) =", array1dTo2d_u8(compData.node_data, compData.node_data_size, compData.header.sense_node), compData.header.force_node, compData.header.sense_node);
		}
		break;

	case 0x16: //read self comp data
		TS_LOG_INFO("%s Get SS Compensation Data...\n", __func__);
		res = readSelfSenseCompensationData(SS_TOUCH, &comData);
		if (res < 0) {
			TS_LOG_INFO("%s Error reading SS compensation data %02X\n", __func__, res);
		} else {
			TS_LOG_INFO("%s SS Compensation Data Reading Finished!\n", __func__);
			size = ((comData.header.force_node + comData.header.sense_node)*2 + 12) * sizeof (u8)*2;
			print_frame_u8("SS Data Ix2_fm = ", array1dTo2d_u8(comData.ix2_fm, comData.header.force_node, 1), comData.header.force_node, 1);
			print_frame_u8("SS Data Cx2_fm = ", array1dTo2d_u8(comData.cx2_fm, comData.header.force_node, 1), comData.header.force_node, 1);
			print_frame_u8("SS Data Ix2_sn = ", array1dTo2d_u8(comData.ix2_sn, comData.header.sense_node, comData.header.sense_node), 1, comData.header.sense_node);
			print_frame_u8("SS Data Cx2_sn = ", array1dTo2d_u8(comData.cx2_sn, comData.header.sense_node, comData.header.sense_node), 1, comData.header.sense_node);
		}
		break;

	case 0x03: // MS Raw DATA TEST
		res = fts_system_reset();
		if (res >= OK)
			res = production_test_ms_raw(LIMITS_FILE, 1, &todoDefault,0,0);
		break;

	case 0x04: // MS CX DATA TEST
		res = fts_system_reset();
		if (res >= OK)
			res = production_test_ms_cx(LIMITS_FILE, 1, &todoDefault,NULL,NULL);
		break;

	case 0x05: // SS RAW DATA TEST
		res = fts_system_reset();
		if (res >= OK)
			res = production_test_ss_raw(LIMITS_FILE, 1, &todoDefault,NULL,NULL);
		break;

	case 0xF0:
	case 0xF1: // TOUCH ENABLE/DISABLE
		doClean = (int) (typeOfComand[0]&0x01);
		res = cleanUp(doClean);
		break;

	default:
		TS_LOG_ERR("%s COMMAND NOT VALID!! Insert a proper value ...\n", __func__);
		res = ERROR_OP_NOT_ALLOW;
		break;
	}

	doClean = fts_enableInterrupt();
	if (doClean < 0)
		TS_LOG_INFO("%s fts_enableInterrupt: %08X\n", __func__,
				(doClean|ERROR_ENABLE_INTER));

END: //here start the reporting phase, assembling the data to send in the file node
	all_strbuff = (u8 *) kmalloc(size, GFP_KERNEL);
	memset(all_strbuff, 0, size);

	snprintf(buff, sizeof (buff), "%02X", 0xAA);
	strncat(all_strbuff, buff, 2);

	snprintf(buff, sizeof (buff), "%08X", res);
	strncat(all_strbuff, buff, 8);

	if (res >= OK) {
		/*all the other cases are already fine printing only the res.*/
		switch (typeOfComand[0]) {
		case 0x13:
			snprintf(buff, sizeof (buff), "%02X", (u8) frameMS.header.force_node);
			strncat(all_strbuff, buff, 2);

			snprintf(buff, sizeof (buff), "%02X", (u8) frameMS.header.sense_node);
			strncat(all_strbuff, buff, 2);

			for (j = 0; j < frameMS.node_data_size; j++) {
				snprintf(buff, sizeof (buff), "%04X", frameMS.node_data[j]);
				strncat(all_strbuff, buff, 4);
			}

			kfree(frameMS.node_data);
			break;

		case 0x15:
			snprintf(buff, sizeof (buff), "%02X", (u8) frameSS.header.force_node);
			strncat(all_strbuff, buff, 2);

			snprintf(buff, sizeof (buff), "%02X", (u8) frameSS.header.sense_node);
			strncat(all_strbuff, buff, 2);

			// Copying self raw data Force
			for (j = 0; j < frameSS.header.force_node; j++) {
				snprintf(buff, sizeof (buff), "%04X", frameSS.force_data[j]);
				strncat(all_strbuff, buff, 4);
			}


			// Copying self raw data Sense
			for (j = 0; j < frameSS.header.sense_node; j++) {
				snprintf(buff, sizeof (buff), "%04X", frameSS.sense_data[j]);
				strncat(all_strbuff, buff, 4);
			}

			kfree(frameSS.force_data);
			kfree(frameSS.sense_data);
			break;

		case 0x14:
			snprintf(buff, sizeof (buff), "%02X", (u8) compData.header.force_node);
			strncat(all_strbuff, buff, 2);

			snprintf(buff, sizeof (buff), "%02X", (u8) compData.header.sense_node);
			strncat(all_strbuff, buff, 2);

			//Cpying CX1 value
			snprintf(buff, sizeof (buff), "%02X", compData.cx1);
			strncat(all_strbuff, buff, 2);

			//Copying CX2 values
			for (j = 0; j < compData.node_data_size; j++) {
				snprintf(buff, sizeof (buff), "%02X", *(compData.node_data + j));
				strncat(all_strbuff, buff, 2);
			}

			kfree(compData.node_data);
			break;

		case 0x16:
			snprintf(buff, sizeof (buff), "%02X", comData.header.force_node);
			strncat(all_strbuff, buff, 2);

			snprintf(buff, sizeof (buff), "%02X", comData.header.sense_node);
			strncat(all_strbuff, buff, 2);

			snprintf(buff, sizeof (buff), "%02X", comData.f_ix1);
			strncat(all_strbuff, buff, 2);

			snprintf(buff, sizeof (buff), "%02X", comData.s_ix1);
			strncat(all_strbuff, buff, 2);

			snprintf(buff, sizeof (buff), "%02X", comData.f_cx1);
			strncat(all_strbuff, buff, 2);

			snprintf(buff, sizeof (buff), "%02X", comData.s_cx1);
			strncat(all_strbuff, buff, 2);

			//Copying IX2 Force
			for (j = 0; j < comData.header.force_node; j++) {
				snprintf(buff, sizeof (buff), "%02X", comData.ix2_fm[j]);
				strncat(all_strbuff, buff, size);
			}

			//Copying IX2 Sense
			for (j = 0; j < comData.header.sense_node; j++) {
				snprintf(buff, sizeof (buff), "%02X", comData.ix2_sn[j]);
				strncat(all_strbuff, buff, 2);
			}

			//Copying CX2 Force
			for (j = 0; j < comData.header.force_node; j++) {
				snprintf(buff, sizeof (buff), "%02X", comData.cx2_fm[j]);
				strncat(all_strbuff, buff, 2);
			}

			//Copying CX2 Sense
			for (j = 0; j < comData.header.sense_node; j++) {
				snprintf(buff, sizeof (buff), "%02X", comData.cx2_sn[j]);
				strncat(all_strbuff, buff, 2);
			}

			kfree(comData.ix2_fm);
			kfree(comData.ix2_sn);
			kfree(comData.cx2_fm);
			kfree(comData.cx2_sn);
			break;

		default:
			break;

		}
	}

	snprintf(buff, sizeof (buff), "%02X", 0xBB);
	strncat(all_strbuff, buff, 2);

	count = snprintf(buf, TSP_BUF_SIZE, "%s\n", all_strbuff);
	numberParameters = 0; //need to reset the number of parameters in order to wait the next comand, comment if you want to repeat the last comand sent just doing a cat
	kfree(all_strbuff);

	return count;
}

static DEVICE_ATTR(strength_frame, (S_IRUGO | S_IWUSR | S_IWGRP), fts_strength_frame_show, fts_strength_frame_store);
static DEVICE_ATTR(stm_fts_cmd, (S_IRUGO | S_IWUSR | S_IWGRP), stm_fts_cmd_show, stm_fts_cmd_store);

static struct attribute *fts_attr_attributes[] = {
	&dev_attr_strength_frame.attr,
	&dev_attr_stm_fts_cmd.attr,
	NULL,
};
struct attribute_group fts_attr_group = {
	.attrs = fts_attr_attributes,
};
static int fts_init(struct fts_ts_info *info)
{
	int error = fts_system_reset();

	if (error < OK && error != (ERROR_TIMEOUT | ERROR_SYSTEM_RESET_FAIL)) {
		TS_LOG_ERR("%s Cannot reset the device! %08X\n", __func__, error);
		return error;
	} 

	if (error == (ERROR_TIMEOUT | ERROR_SYSTEM_RESET_FAIL)) {
		TS_LOG_ERR("%s Setting default Chip INFO!\n", __func__);
		defaultChipInfo(0);
	}else{
		error = readChipInfo(0); //system reset OK
		if (error < OK) {
			TS_LOG_ERR("%s Cannot read Chip Info! %08X\n", __func__, error);
		}
	}

	error = fts_interrupt_install(info);
	if (error != OK)
		TS_LOG_ERR("%s Init (1) error (ERROR  = %08X)\n", __func__, error);

	return error;
}

int fts_chip_powercycle(struct fts_ts_info *info)
{
	int error;

	TS_LOG_ERR("%s: Power Cycle Starting...\n", __func__);
	TS_LOG_ERR("%s: Disabling IRQ...\n", __func__);	//if IRQ pin is short with DVDD a call to the ISR will triggered when the regulator is turned off
	disable_irq_nosync(info->chip_data->ts_platform_data->irq_id);
	if (info->pwr_reg) {
		error = regulator_disable(info->pwr_reg);
		if (error < 0) {
			TS_LOG_ERR("%s: Failed to disable DVDD regulator\n", __func__);
		}
	}

	if (info->bus_reg) {
		error = regulator_disable(info->bus_reg);
		if (error < 0) {
			TS_LOG_ERR("%s: Failed to disable AVDD regulator\n", __func__);
		}
	}

	gpio_set_value(info->chip_data->ts_platform_data->reset_gpio, 0);

	if (info->pwr_reg) {
		error = regulator_enable(info->bus_reg);
		if (error < 0) {
			TS_LOG_ERR("%s: Failed to enable AVDD regulator\n", __func__);
		}
	}

	if (info->bus_reg) {
		error = regulator_enable(info->pwr_reg);
		if (error < 0) {
			TS_LOG_ERR("%s: Failed to enable DVDD regulator\n", __func__);
		}
	}
	mdelay(10); //time needed by the regulators for reaching the regime values

	gpio_set_value(info->chip_data->ts_platform_data->reset_gpio, 1);

	TS_LOG_ERR("%s: Enabling IRQ...\n", __func__);
	enable_irq(info->chip_data->ts_platform_data->irq_id);
	TS_LOG_ERR("%s: Power Cycle Finished! CODE = %08x\n", __func__, error);
	setSystemResettedUp(1);
	setSystemResettedDown(1);
	return error;
}

static int st_resume(void)
{
	struct fts_ts_info *info = fts_get_info();

	info->resume_bit = 1;

#ifdef USE_NOISE_PARAM
	readNoiseParameters(noise_params);
#endif
	fts_system_reset();

#ifdef USE_NOISE_PARAM
	writeNoiseParameters(noise_params);
#endif

	fts_mode_handler(info,0);

	fts_enableInterrupt();

	if (g_tskit_pt_station_flag)
	{
		TS_LOG_INFO("%s: pt flag   key serse on \n", __func__);
		keyOn();
		suspensionOn();
		senseOn();
	}
	return 0;
}

static int st_suspend(void)
{
	struct fts_ts_info *info = fts_get_info();

	info->resume_bit = 0;

	fts_mode_handler(info,0);

	fts_enableInterrupt();

	if ((g_tskit_pt_station_flag)  ||(! info->chip_data->ts_platform_data->udfp_enable_flag))
	{
		TS_LOG_INFO("%s: pt station or no ud fingerprint \n", __func__);
		keyOff();
		suspensionOff();
		senseOff();
	}

	return 0;
}

static int fts_get_reg(struct fts_ts_info *info, bool get) {
	int retval;

	if (!get) {
		retval = 0;
		goto regulator_put;
	}

	info->pwr_reg = regulator_get(info->dev, ST_TS_AVCC_NAME);
	if (IS_ERR(info->pwr_reg)) {
		TS_LOG_ERR("%s: Failed to get power regulator\n", __func__);
		retval = PTR_ERR(info->pwr_reg);
		goto regulator_put;
	}
	retval = regulator_set_voltage(info->pwr_reg, info->avdd_value,
				info->avdd_value);
	if (retval)
		TS_LOG_ERR("%s: Failed to set pwr regulator vol\n", __func__);


	info->bus_reg = regulator_get(info->dev, ST_TS_IOVCC_NAME);
	if (IS_ERR(info->bus_reg)) {
		TS_LOG_ERR("%s: Failed to get bus pullup regulator\n", __func__);
		retval = PTR_ERR(info->bus_reg);
		goto regulator_put;
	}

	retval = regulator_set_voltage(info->bus_reg, info->iovdd_value,
				info->iovdd_value);
	if (retval)
		TS_LOG_ERR("%s: Failed to set bus regulator vol\n", __func__);

	return 0;

regulator_put:
	if (info->pwr_reg) {
		regulator_put(info->pwr_reg);
		info->pwr_reg = NULL;
	}

	if (info->bus_reg) {
		regulator_put(info->bus_reg);
		info->bus_reg = NULL;
	}

	return retval;
}

#define ST_POWER_ON_DELAY_MS 20
static int fts_enable_reg(struct fts_ts_info *info,
		bool enable) {
	int retval;

	if (!enable) {
		retval = 0;
		goto power_off;
	}

	if (info->pwr_reg) {
		retval = regulator_enable(info->pwr_reg);
		TS_LOG_INFO("%s: avdd on\n", __func__);
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to enable power regulator\n", __func__);
			goto disable_bus_reg;
		}
		mdelay(ST_POWER_ON_DELAY_MS);
	}

	if (info->bus_reg) {
		retval = regulator_enable(info->bus_reg);
		TS_LOG_INFO("%s: iovdd on\n", __func__);
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to enable bus regulator\n", __func__);
			goto power_off;
		}
		mdelay(ST_POWER_ON_DELAY_MS);
	}

	return OK;

power_off:
	if (info->bus_reg) {
		regulator_disable(info->bus_reg);
		mdelay(5); /* in IC SPEC, need dealy 5ms after 1.8v power off */
	}
disable_bus_reg:
	if (info->pwr_reg)
		regulator_disable(info->pwr_reg);

exit:
	return retval;
}

static void st_hw_reset(struct ts_kit_device_data *chip_data)
{
	/* reset chip */
	if (chip_data->ts_platform_data->reset_gpio > 0){
		gpio_direction_output(chip_data->ts_platform_data->reset_gpio, 0);
		mdelay(15); /* ST need more than 15ms to keep reset low in SPEC */
		gpio_direction_output(chip_data->ts_platform_data->reset_gpio, 1);
		mdelay(70);
	}
}

static int fts_set_gpio(struct fts_ts_info *info) {
	st_hw_reset(info->chip_data);
	setResetGpio(info->chip_data->ts_platform_data->reset_gpio);
	return OK;
}

static int st_calibrate(void)
{
	int ret = 0;
	int i =0 ;
	int reval = 0;
	struct fts_ts_info *info = fts_get_info();
	info->check_MutualRawGap_after_callibrate = FTS_TRUE;
repeat:
	/* inval is calibrate times and etc. msg */
	ret = fts_system_reset();
	if (ret < 0) {
		TS_LOG_ERR("%s system reset fail\n", __func__);
		info->check_MutualRawGap_after_callibrate = FTS_FALSE;
		return ret;
	}

	ret = fts_calibrate(PRESSURE_CALIBRATE_TYPE);
	if (ret) {
		TS_LOG_ERR("%s: PRESSURE_CALIBRATE_TYPE error\n", __func__);
	}
	ret |= fts_calibrate(TOUCH_CALIBRATE_TYPE);
	if (ret) {
		TS_LOG_ERR("%s: TOUCH_CALIBRATE_TYPE error\n", __func__);
	}
	ret |= cleanUp(0);					//system reset
	ret |= fts_mode_handler(fts_info, 0);			//enable the features and the sensing
	ret |= fts_enableInterrupt();
	if (ret<0) {
		TS_LOG_ERR("Cannot initialize the hardware device after st_calibration\n");
	}

	TS_LOG_INFO("%s:touch calibrate times :%d\n", __func__,i);
	reval = check_MutualRawResGap();
	if(reval){
		if(i < 3){
			i++;
			goto repeat;
		}
	}

	info->check_MutualRawGap_after_callibrate = FTS_FALSE;
	return ret;
}



static int st_oem_info_switch(struct ts_oem_info_param *info)
{
	TS_LOG_INFO("[%s] -> %d\n",__func__,__LINE__);

	int read_len = 0;
	int ret = 0;

	struct fts_ts_info *st_info = fts_get_info();

	if(info == NULL)
	{
		TS_LOG_ERR("%s: info is NULL return \n", __func__);
		return -EIO;
	}
	switch (info->op_action) {
	case TS_ACTION_READ:
		if(st_info->barcode_status < 0)
			return -EIO;
		memset(info->data, 0, sizeof(info->data));
		memcpy(info->data + LOCKDOWN_2D_BAR_INFO_HEAD_LEN ,
				st_info->barcode + LOCKDOWN_2D_BAR_INFO_HEAD_LEN,FTS_BARCODE_SIZE);
		info->data[0] = TYPE_ID_2DBARCODE;
		info->data[1] = TYPE_ID_2DBARCODE_LENGTH;
		info->data[FTS_BARCODE_SIZE + LOCKDOWN_2D_BAR_INFO_HEAD_LEN] = '\0';
		info->length = TYPE_ID_2DBARCODE_LENGTH;
		TS_LOG_INFO("%s: bar_2d_data=%s\n", __func__, info->data);
		break;
	case TS_ACTION_WRITE:
		TS_LOG_INFO("[%s]info->data[0] -> %d\n",__func__,info->data[0] );
		break;
	default:
		TS_LOG_ERR("invalid status\n");
		break;
	}
	return 0;
}

int st_get_debug_data(struct ts_diff_data_info* info, struct ts_cmd_node* out_cmd)
{
	int ret = 0;
	int size = 0;
	int count = 0;
	SelfSenseData ssCompData;
	short *ss_sense_frame=NULL;
	short *ss_force_frame=NULL;
	SelfSenseFrame SS_Frame;
	SelfSenseData comData;

	if(NULL == info || NULL == out_cmd)
	{
		TS_LOG_ERR("the pointer error: NULL\n");
		return -EINVAL;
	}
	memset(info->buff , 0 , sizeof(info->buff));
	memset(&ssCompData , 0 ,sizeof(ssCompData));
	memset(&SS_Frame , 0 ,sizeof(SS_Frame));
	memset(&comData , 0 ,sizeof(comData));

	switch (info->debug_type) {
	case READ_FORCE_DATA:
		fts_disableInterrupt();
		//diff
		size = getSSFrame(ADDR_NORM_HOVER_FORCE, &ss_force_frame);//strength force data
		TS_LOG_INFO("********Hover_Tx_diff********\n");
		short_to_infobuf(info->buff, ss_force_frame,size , BUFF_SEEK_BIT);
		count = count + size + BUFF_SEEK_BIT;

		size = getSSFrame(ADDR_NORM_HOVER_SENSE, &ss_sense_frame);//strength sense data
		TS_LOG_INFO("********Hover_Rx_diff********\n");
		short_to_infobuf(info->buff, ss_sense_frame,size , count);
		count = count + size;
		//raw
		size = getSSFrame2(SS_HOVER, &SS_Frame);// include raw force and sense data
		TS_LOG_INFO("********Hover_Rawdata********\n");
		short_to_infobuf(info->buff, SS_Frame.force_data,SS_Frame.header.force_node , count);
		count = count + SS_Frame.header.force_node ;
		short_to_infobuf(info->buff, SS_Frame.sense_data,SS_Frame.header.sense_node , count);
		count = count + SS_Frame.header.sense_node ;

		ret = readSelfSenseCompensationData(SS_HOVER, &ssCompData);// include a f_ix1(global force) and s_ix1(global sense) and ix2_fm(every force channel) and  ix2_sn (every sense channel)data
		if(ret < 0 )
		{
			TS_LOG_ERR("read self sense com data SS_HOVER error\n");
			ret = -EINVAL;
			break;
		}
		TS_LOG_INFO("********Hover_compensation********\n");

		uchar_to_infobuf(info->buff, ssCompData.ix2_fm,ssCompData.header.force_node  , count);
		count = count + ssCompData.header.force_node ;

		uchar_to_infobuf(info->buff, ssCompData.ix2_sn,ssCompData.header.sense_node  , count);
		count = count + ssCompData.header.sense_node ;

		uchar_to_infobuf(info->buff, ssCompData.cx2_fm, ssCompData.header.force_node , count);
		count = count + ssCompData.header.force_node ;

		uchar_to_infobuf(info->buff,ssCompData.cx2_sn, ssCompData.header.sense_node , count);
		count = count + ssCompData.header.sense_node ;

		ret = readSelfSenseCompensationData(SS_KEY, &comData);
		if(ret < 0 )
		{
			TS_LOG_ERR("read self sense com data SS_KEY error\n");
			ret = -EINVAL;
			break;
		}
		TS_LOG_INFO("********ForceKey_Compensation********\n");

		uchar_to_infobuf(info->buff,comData.ix2_fm, comData.header.force_node , count);
		count = count + comData.header.force_node ;

		uchar_to_infobuf(info->buff,comData.ix2_sn,comData.header.sense_node, count);
		count = count + comData.header.sense_node ;

		uchar_to_infobuf(info->buff,comData.cx2_fm,comData.header.force_node, count);
		count = count + comData.header.force_node ;

		uchar_to_infobuf(info->buff,comData.cx2_sn,comData.header.sense_node, count);
		count = count + comData.header.sense_node ;

		TS_LOG_INFO("********ForceKey_Compensation_Value********\n");

		info->buff[count] = forcekeyvalue(comData.f_ix1 ,comData.ix2_fm[0] );
		count = count +1;

		info->buff[count] = forcekeyvalue(comData.f_ix1 ,comData.ix2_fm[1] );
		count = count +1;

		fts_enableInterrupt();
		info->buff[0] = count;
		break;
	default:
		ret = -EINVAL;
		TS_LOG_ERR("%s:debug_type mis match\n", __func__);
		break;
	}

	return ret;
}

static int st_get_capacitance_test_type(struct ts_test_type_info *info)
{
	int error = NO_ERR;
	struct fts_ts_info *data = fts_get_info();

	if (!info) {
		TS_LOG_ERR("%s:info=%ld\n", __func__, PTR_ERR(info));
		error = -ENOMEM;
		return error;
	}
	switch (info->op_action) {
	case TS_ACTION_READ:
		memcpy(info->tp_test_type, data->chip_data->tp_test_type,
		       TS_CAP_TEST_TYPE_LEN);
		TS_LOG_INFO("read_chip_get_test_type=%s,\n",
			    info->tp_test_type);
		break;
	case TS_ACTION_WRITE:
		break;
	default:
		TS_LOG_ERR("invalid status: %s", info->tp_test_type);
		error = -EINVAL;
		break;
	}
	return error;
}

static int st_holster_switch(struct ts_holster_info *info)
{
	int retval = NO_ERR;

	TS_LOG_INFO("holster switch(%d) action: %d\n", info->holster_switch, info->op_action);
	if (!info) {
		TS_LOG_ERR("synaptics_holster_switch: info is Null\n");
		retval = -ENOMEM;
		return retval;
	}

	switch (info->op_action) {
		case TS_ACTION_WRITE:
			retval = featureEnableDisable(info->holster_switch, FEAT_COVER);
			if (retval < 0) {
				TS_LOG_ERR("set holster switch(%d), failed: %d\n",
					   info->holster_switch, retval);
			}
			break;
		case TS_ACTION_READ:
			TS_LOG_INFO
				("invalid holster switch(%d) action: TS_ACTION_READ\n",
				 info->holster_switch);
			break;
		default:
			TS_LOG_INFO("invalid holster switch(%d) action: %d\n", info->holster_switch, info->op_action);
			retval = -EINVAL;
			break;
	}
	return retval;
}

static int st_glove_switch(struct ts_glove_info *info)
{
	int retval = NO_ERR;

	TS_LOG_INFO("%s +\n", __func__);
	if (!info) {
		TS_LOG_ERR("st_glove_switch: info is Null\n");
		retval = -ENOMEM;
		return retval;
	}
	switch (info->op_action) {
		case TS_ACTION_READ:
			TS_LOG_INFO("read_glove_switch=%d, 1:on 0:off\n",info->glove_switch);
			break;
		case TS_ACTION_WRITE:
			TS_LOG_INFO("write_glove_switch=%d\n",info->glove_switch);
			if ((GLOVE_SWITCH_ON != info->glove_switch)
				&& (GLOVE_SWITCH_OFF != info->glove_switch)) {
				TS_LOG_ERR("write wrong state: switch = %d\n", info->glove_switch);
				retval = -EFAULT;
				break;
			}
			retval = featureEnableDisable(info->glove_switch,FEAT_GLOVE);
			if (retval < 0) {
				TS_LOG_ERR("set glove switch(%d), failed : %d", info->glove_switch,
					   retval);
			}
			break;
		default:
			TS_LOG_ERR("invalid switch status: %d", info->glove_switch);
			retval = -EINVAL;
			break;
	}
	TS_LOG_INFO("%s -\n", __func__);
	return retval;
}

static int st_roi_switch(struct ts_roi_info *info)
{
	if (!info) {
		TS_LOG_ERR("sec_ts_roi_switch: info is Null\n");
		return -ENOMEM;
	}

	switch (info->op_action) {
	case TS_ACTION_WRITE:
		TS_LOG_ERR("%s:set roi switch:%d\n", __func__, info->roi_switch);
		st_roi_switch_on = info->roi_switch;
		break;
	case TS_ACTION_READ:
	default:
		return 0;
	}

	return 0;
}

static unsigned char *st_ts_roi_rawdata(void)
{
#ifdef ROI

	if (st_roi_switch_on)  {
		if (st_roi_data_staled)  {  /* roi_data may be refreshing now, wait it for some time(30ms). */
			if (!wait_for_completion_interruptible_timeout(&st_roi_data_done, msecs_to_jiffies(30))) {
				TS_LOG_DEBUG("%s:wait roi data timeout\n", __func__);
				st_roi_data_staled = 0;  /* Never wait again if data refreshing gets timeout. */
				memset(st_roi_data, 0, sizeof(st_roi_data));
			}
		}
	}

	return (unsigned char *)st_roi_data;
#else
	return NULL;
#endif
}

#define ST_ROI_ADDR_DATA_CMD_LEN 3
static u16 st_roi_addr_get(void)
{
	int ret = 0;
	unsigned char regAdd[] = {0xD0, 0x00, 0x62, 0x03};
	unsigned char data[ST_ROI_ADDR_DATA_CMD_LEN] = {0};

	ret = fts_readCmd(regAdd, sizeof(regAdd)-1, data, ST_ROI_ADDR_DATA_CMD_LEN);
	if(ret){
		TS_LOG_ERR("Cannot read Offset Address for f_cnt and s_cnt 2\n");
		return -1;
	}
	/* data[0] is dummy byte which should be dropped */
	TS_LOG_DEBUG("roi addr is 0x%x, 0x%x ,0x%x, 0x%x\n",
		data[1] | (data[2] << 8), data[0], data[1], data[2]);

	return ((data[2] << 8) | data[1]);
}

void st_ts_work_after_input_kit(void)
{
#ifdef ROI
	int ret = 0;
	unsigned char regAdd[4] = {0};
	unsigned char data[99] = {0};
	u16 roi_addr = 0;

	TS_LOG_DEBUG("%s +\n", __func__);
	roi_addr = st_roi_addr_get();

	/* roi data read command */
	regAdd[0] = 0xD0;
	regAdd[1] = (roi_addr & 0xFF00) >> 8;
	regAdd[2] = (roi_addr & 0xFF);
	regAdd[3] = 0x50;

	if (st_roi_switch_on)  {
		if (st_roi_data_staled == 0)  {  /* roi_data is up to date now. */
			return;
		}

		/* We are about to refresh roi_data. To avoid stale output, use a completion to block possible readers. */
		reinit_completion(&st_roi_data_done);

		/* Request sensorhub to report fingersense data. */
		/* preread_fingersense_data(); */

		ret = fts_readCmd(regAdd, sizeof(regAdd) - 1, data, sizeof(data)/sizeof(data[0]));
		if (ret < 0) {
			TS_LOG_ERR("F12 Failed to read roi data, retval= %d\n",
					ret);
		} else {
			/* roi_data pointer will be exported to app by function synaptics_roi_rawdata, so there's a race
			*  condition while app reads roi_data and our driver refreshes it at the same time. We use local
			*   variable(roi_tmp) to decrease the conflicting time window. Thus app will get consistent data,
			*   except when we are executing the following memcpy.
			*/
			memcpy(&st_roi_data[4], &data[1], sizeof(data)/sizeof(data[0]) -1);
		}

		st_roi_data_staled = 0;
		complete_all(&st_roi_data_done);  /* If anyone has been blocked by us, wake it up. */
	}
#endif
}

static int st_after_resume(void *feature_info)
{
	int retval = NO_ERR;
	struct fts_ts_info *fts_info = fts_get_info();
	struct ts_feature_info *info = &fts_info->chip_data->ts_platform_data->feature_info;

	TS_LOG_INFO("%s +\n", __func__);

	retval = featureEnableDisable(info->glove_info.glove_switch, FEAT_GLOVE);
	if (retval) {
		TS_LOG_ERR("retore glove switch(%d) failed: %d\n",
			info->glove_info.glove_switch, retval);
	}

	retval = featureEnableDisable(info->holster_info.holster_switch, FEAT_COVER);
	if (retval < 0) {
		TS_LOG_ERR("retore holster switch(%d), failed: %d\n",
			   info->holster_info.holster_switch, retval);
	}

	fts_enable_force_key();

	return retval;
}

static int st_before_suspend(void)
{
	int retval = NO_ERR;

	TS_LOG_INFO("%s +\n", __func__);
	TS_LOG_INFO("%s -\n", __func__);
	return retval;
}

static int st_chip_get_info(struct ts_chip_info_param *info)
{
	int retval = NO_ERR;
	struct fts_ts_info *ts_info = fts_get_info();
	
	snprintf(info->ic_vendor, sizeof(info->ic_vendor), "st");
	snprintf(info->mod_vendor, sizeof(info->ic_vendor), ts_info->project_id);
	snprintf(info->fw_vendor, sizeof(info->fw_vendor),
		 "%x.%x", ftsInfo.u16_fwVer, ftsInfo.u16_cfgId);

	return retval;
}


static int st_fw_update_sd(void)
{
	struct fts_ts_info *st_info = fts_get_info();

	fts_fw_update_auto(st_info, true);

	return 0;
}

static int st_fw_update_boot(char *file_name)
{
	struct fts_ts_info *st_info = fts_get_info();

	return fts_fw_update_auto(st_info, false);
}

static int st_irq_bottom_half(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd)
{
	/*
	 * to avoid reading all FIFO, we read the first event and
	 * then check how many events left in the FIFO
	 */

	struct fts_ts_info *info = fts_get_info();
	int touch_evnet_count = 0;

	TS_LOG_DEBUG("%s: +\n", __func__);

	info->fingers_info = &out_cmd->cmd_param.pub_params.algo_param.info;

	out_cmd->command = TS_INPUT_ALGO;
	out_cmd->cmd_param.pub_params.algo_param.algo_order =
		info->chip_data->algo_id;

	fts_event_handler(info, &touch_evnet_count);

	if(NULL == info->fingers_info){
		TS_LOG_ERR("fingers_info is NULL pointer\n");
		return NO_ERR;
	}

	/*
	 * If no touch report event, shoud not do input report
	 */
	if(!touch_evnet_count) {
		out_cmd->command = TS_INVAILD_CMD;
		return NO_ERR;
	}

	TS_LOG_DEBUG("%s:g_pressure=%d\n", __func__, g_pressure);
	if(g_pressure)
		info->fingers_info->fingers[0].pressure = g_pressure;
	else
		info->fingers_info->fingers[0].pressure = 1;

	TS_LOG_DEBUG("%s: -\n", __func__);
	return NO_ERR;
}



static int st_parse_dts(struct device_node *device, struct ts_kit_device_data *chip_data)
{
	int retval  = NO_ERR;
	const char *raw_data_dts = NULL;
	int index = 0;
	int array_len = 0;
	struct fts_ts_info *ts = fts_get_info();
	u32 value = 0;

	TS_LOG_INFO("%s st parse dts start\n", __func__);

	retval = of_property_read_u32(device, ST_IRQ_CFG, &chip_data->irq_config);
	if (retval) {
		TS_LOG_ERR("get irq config failed\n");
		retval = -EINVAL;
		goto err;
	}

	retval = of_property_read_u32(device, "i2c_address", &chip_data->slave_addr);
	if (retval) {
		TS_LOG_ERR("get i2c address failed, use default 0x49\n");
		chip_data->slave_addr = 0x49;
	}

	retval = of_property_read_u32(device, ST_ALGO_ID, &chip_data->algo_id);
	if (retval) {
		TS_LOG_ERR("get algo id failed\n");
		retval = -EINVAL;
		goto err;
	}

	retval = of_property_read_u32(device, ST_X_MAX, &chip_data->x_max);
	if (retval) {
		TS_LOG_ERR("get device x_max failed\n");
		retval = -EINVAL;
		goto err;
	}
	retval = of_property_read_u32(device, ST_Y_MAX, &chip_data->y_max);
	if (retval) {
		TS_LOG_ERR("get device y_max failed\n");
		retval = -EINVAL;
		goto err;
	}
	retval = of_property_read_u32(device, ST_X_MAX_MT, &chip_data->x_max_mt);
	if (retval) {
		TS_LOG_ERR("get device x_max failed\n");
		retval = -EINVAL;
		goto err;
	}
	retval = of_property_read_u32(device, ST_Y_MAX_MT, &chip_data->y_max_mt);
	if (retval) {
		TS_LOG_ERR("get device y_max failed\n");
		retval = -EINVAL;
		goto err;
	}

	retval = of_property_read_u32(device, ST_VCI_GPIO_TYPE,
				&chip_data->vci_gpio_type);
	if (retval) {
		TS_LOG_ERR("get device ST_VCI_GPIO_TYPE failed\n");
		retval = -EINVAL;
		goto err;
	}

	retval = of_property_read_u32(device, ST_VCI_REGULATOR_TYPE,
				&chip_data->vci_regulator_type);
	if (retval) {
		TS_LOG_ERR("get device ST_VCI_REGULATOR_TYPE failed\n");
		retval = -EINVAL;
		goto err;
	}

	if (chip_data->vci_regulator_type) {
		if(of_property_read_u32(device, ST_AVDD_VOL_VALUE,
				&ts->avdd_value)) {
			TS_LOG_ERR("AVDD_VOL_VALUE not found. use default 3.3v\n");
			ts->avdd_value = 3300000;
		}
	}

	retval = of_property_read_u32(device, ST_VDDIO_GPIO_TYPE,
				&chip_data->vddio_gpio_type);
	if (retval) {
		TS_LOG_ERR("get device ST_VDDIO_GPIO_TYPE failed\n");
		retval = -EINVAL;
		goto err;
	}

	retval = of_property_read_u32(device, ST_VDDIO_REGULATOR_TYPE,
				&chip_data->vddio_regulator_type);
	if (retval) {
		TS_LOG_ERR("get device ST_VDDIO_REGULATOR_TYPE failed\n");
		retval = -EINVAL;
		goto err;
	}

	if (chip_data->vddio_regulator_type) {
		if(of_property_read_u32(device, ST_IOVDD_VOL_VALUE,
				&ts->iovdd_value)) {
			TS_LOG_ERR("IOVDD_VOL_VALUE not found, use default 1.8v\n");
			ts->iovdd_value = 1800000;
		}
	}

	retval = of_property_read_u32(device, "supported_func_indicater",
				&chip_data->supported_func_indicater);
	if (retval) {
		TS_LOG_INFO("get supported_func_indicater = 0\n" );
		chip_data->supported_func_indicater = 0;
	}

	retval = of_property_read_u32(device, "self_cap_test_support",
				&chip_data->self_cap_test);
	if (retval) {
		TS_LOG_INFO("get self_cap_test support = 0\n" );
		chip_data->self_cap_test = 0;
	}

	retval = of_property_read_string(device, "fake_project_id", &ts->fake_project_id);
	if (retval) {
		TS_LOG_INFO("fake_project_id not config\n" );
		ts->fake_project_id = "";
	}

	/*0 is power supplied by gpio, 1 is power supplied by ldo*/
	if (1 == chip_data->vci_gpio_type) {
		chip_data->vci_gpio_ctrl = of_get_named_gpio(device, ST_VCI_GPIO_CTRL, 0);
		if (!gpio_is_valid(chip_data->vci_gpio_ctrl)) {
			TS_LOG_ERR("power gpio is not valid\n");
		}
	}
	if (1 == chip_data->vddio_gpio_type) {
		chip_data->vddio_gpio_ctrl = of_get_named_gpio(device, ST_VDDIO_GPIO_CTRL, 0);
		if (!gpio_is_valid(chip_data->vddio_gpio_ctrl)) {
			TS_LOG_ERR("%s:power gpio is not valid\n", __func__);
		}
	}

	retval = of_property_read_string(device, "tp_test_type",
				 (const char **)&chip_data->tp_test_type);
	if (retval) {
		TS_LOG_INFO
		    ("get device tp_test_type not exit,use default value\n");
		strncpy(chip_data->tp_test_type,
			"Normalize_type:judge_different_reslut",
			TS_CAP_TEST_TYPE_LEN);
		retval = 0;
	}

	/*0 is cover without glass, 1 is cover with glass that need glove mode*/
	retval = of_property_read_u32(device, ST_COVER_FORCE_GLOVE,
				&chip_data->cover_force_glove);
	if (retval) {
		TS_LOG_INFO("get device COVER_FORCE_GLOVE failed,use default!\n");
		/* if not define in dtsi,set 0 to disable it */
		chip_data->cover_force_glove = 0;
		retval = 0;
	}

	if (!of_property_read_u32(device, "roi_supported", &value)) {
		TS_LOG_ERR("get chip specific roi_supported = %d\n", value);
		chip_data->ts_platform_data->feature_info.roi_info.roi_supported = (u8)value;
	} else {
		TS_LOG_ERR("can not get roi_supported value, default support\n");
		chip_data->ts_platform_data->feature_info.roi_info.roi_supported = 1;
	}

	if (!of_property_read_u32(device, "st,forcekey_test_supported", &value)) {
		TS_LOG_INFO("forcekey_test_supported = %d\n", value);
		chip_data->forcekey_test_support = value;
	} else {
		TS_LOG_ERR("forcekey_test not supported \n");
		chip_data->forcekey_test_support = 0;
	}

	if (!of_property_read_u32(device, "use_lcdkit_power_notify", &value))
		chip_data->use_lcdkit_power_notify = value;
	else
		chip_data->use_lcdkit_power_notify = 0;

	TS_LOG_INFO("%s:use_lcdkit_power_notify = %d\n", __func__,
					chip_data->use_lcdkit_power_notify);

	array_len = of_property_count_strings(device, "raw_data_limit");
	if (array_len <= 0 || array_len > RAWDATA_LIMIT_NUM) {
		TS_LOG_ERR("raw_data_limit length invaild or dts number is larger than:%d\n", array_len);
	}

	for(index = 0; index < array_len; index++){
		retval = of_property_read_string_index(device, "raw_data_limit", index, &raw_data_dts);
		if (retval) {
			TS_LOG_ERR("read index = %d,raw_data_limit = %s,retval = %d error,\n", index, raw_data_dts, retval);
		}

		ts->st_raw_limit_buf[index] = simple_strtol(raw_data_dts, NULL, 10);
		TS_LOG_INFO("rawdatabuf[%d] = %d\n", index, ts->st_raw_limit_buf[index]);
	}

	TS_LOG_INFO("irq_config=%d, algo_id=%d, x_max=%d, y_max=%d, x_mt=%d,y_mt=%d\n", \
		chip_data->irq_config, chip_data->algo_id,
		chip_data->x_max, chip_data->y_max, chip_data->x_max_mt, chip_data->y_max_mt);
err:
	return retval;
}

int st_get_project_id(struct fts_ts_info *fts_info)
{
	int read_len;
	int ret;

	ret = readLockDownInfo(PROJECT_ID_TYPE, fts_info->project_id,
			ST_PROJECT_ID_LEN, &read_len);
	if (ret < OK) {
		TS_LOG_ERR("%s: project_id read failed, use fake id\n", __func__);
		strncpy(fts_info->project_id, fts_info->fake_project_id,
				strlen(fts_info->fake_project_id));
		return -EIO;
	}

	fts_info->project_id[ST_PROJECT_ID_LEN] = '\0';
	TS_LOG_INFO("%s: project_id=%s\n", __func__, fts_info->project_id);
	return 0;
}

int st_get_2dbarcode(struct fts_ts_info *fts_info)
{
	int read_len = 0;
	int ret = 0;
	ret = readLockDownInfo(BARCODE_TYPE, fts_info->barcode,
					FTS_BARCODE_SIZE + LOCKDOWN_2D_BAR_INFO_HEAD_LEN, &read_len);
	if (ret < OK) {
		TS_LOG_ERR("%s: barcode read failed\n", __func__);
		memset(fts_info->barcode , 0 ,FTS_BARCODE_SIZE + LOCKDOWN_2D_BAR_INFO_HEAD_LEN + 1);
		return -EIO;
	}

	fts_info->barcode[FTS_BARCODE_SIZE + LOCKDOWN_2D_BAR_INFO_HEAD_LEN] = '\0';
	TS_LOG_INFO("%s: barcode=%s\n", __func__, fts_info->barcode);
	return 0;
}


static int st_init_chip(void)
{
	int retval;
	struct fts_ts_info *info = fts_get_info();

	TS_LOG_INFO("%s: in\n", __func__);

	info->resume_bit = 1;

	st_get_project_id(info);

	snprintf(info->chip_data->chip_name, MAX_STR_LEN, "st");
	snprintf(info->chip_data->module_name, MAX_STR_LEN, info->project_id);
	snprintf(info->chip_data->version_name, MAX_STR_LEN,"%x.%x", ftsInfo.u16_fwVer, ftsInfo.u16_cfgId);

	info->barcode_status = st_get_2dbarcode(info);

#ifdef PHONE_GESTURE
	mutex_init(&gestureMask_mutex);
#endif

#ifdef SCRIPTLESS
	/*I2C cmd*/
	if (fts_cmd_class == NULL)
		fts_cmd_class = class_create(THIS_MODULE, FTS_TS_DRV_NAME);
	info->i2c_cmd_dev = device_create(fts_cmd_class,
			NULL, DCHIP_ID_0, info, "fts_i2c");
	if (IS_ERR(info->i2c_cmd_dev)) {
		TS_LOG_ERR("%s: Failed to create device for the sysfs!\n", __func__);
	}

	dev_set_drvdata(info->i2c_cmd_dev, info);

	retval = sysfs_create_group(&info->i2c_cmd_dev->kobj,
			&i2c_cmd_attr_group);
	if (retval) {
		TS_LOG_ERR("%s: Failed to create sysfs group!\n", __func__);
	}

#endif

	/* sysfs stuff */
	retval = sysfs_create_group(&info->i2c_cmd_dev->kobj, &fts_attr_group);
	if (retval) {
	TS_LOG_ERR("%s: Cannot create sysfs structure!\n", __func__);
	}

#ifdef DRIVER_TEST
	if (fts_cmd_class == NULL)
		fts_cmd_class = class_create(THIS_MODULE, FTS_TS_DRV_NAME);
		info->test_cmd_dev = device_create(fts_cmd_class,
			NULL, DCHIP_ID_0, info, "fts_driver_test");
	if (IS_ERR(info->test_cmd_dev)) {
		TS_LOG_ERR("%s: Failed to create device for the sysfs!\n", __func__);
	}

	dev_set_drvdata(info->test_cmd_dev, info);

	retval = sysfs_create_group(&info->test_cmd_dev->kobj,
	    &test_cmd_attr_group);
	if (retval) {
		TS_LOG_ERR("%s: Failed to create sysfs group!\n", __func__);
	}
#endif

#if defined (CONFIG_TEE_TUI)
	strncpy(tee_tui_data.device_name, "st", strlen("st"));
	tee_tui_data.device_name[strlen("st")] = '\0';
#endif

	TS_LOG_INFO("%s: -\n", __func__);
	return 0;
}

static int st_input_config(struct input_dev *input_dev)
{
	struct fts_ts_info *st_info = fts_get_info();

	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(BTN_TOUCH, input_dev->keybit);
	set_bit(BTN_TOOL_FINGER, input_dev->keybit);

	set_bit(TS_DOUBLE_CLICK, input_dev->keybit);
	set_bit(TS_SLIDE_L2R, input_dev->keybit);
	set_bit(TS_SLIDE_R2L, input_dev->keybit);
	set_bit(TS_SLIDE_T2B, input_dev->keybit);
	set_bit(TS_SLIDE_B2T, input_dev->keybit);
	set_bit(TS_CIRCLE_SLIDE, input_dev->keybit);
	set_bit(TS_LETTER_c, input_dev->keybit);
	set_bit(TS_LETTER_e, input_dev->keybit);
	set_bit(TS_LETTER_m, input_dev->keybit);
	set_bit(TS_LETTER_w, input_dev->keybit);
	set_bit(TS_PALM_COVERED, input_dev->keybit);

	set_bit(TS_TOUCHPLUS_KEY0, input_dev->keybit);
	set_bit(TS_TOUCHPLUS_KEY1, input_dev->keybit);
	set_bit(TS_TOUCHPLUS_KEY2, input_dev->keybit);
	set_bit(TS_TOUCHPLUS_KEY3, input_dev->keybit);
	set_bit(TS_TOUCHPLUS_KEY4, input_dev->keybit);

#ifdef INPUT_PROP_DIRECT
	set_bit(INPUT_PROP_DIRECT, input_dev->propbit);
#endif
#if ANTI_FALSE_TOUCH_USE_PARAM_MAJOR_MINOR
	input_set_abs_params(input_dev, ABS_MT_WIDTH_MAJOR, 0, 100, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_WIDTH_MINOR, 0, 100, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_ORIENTATION, 0, 255, 0, 0);
#else
	input_set_abs_params(input_dev, ABS_MT_DISTANCE, 0, 100, 0, 0);
#endif
	input_set_abs_params(input_dev, ABS_X,
				0, st_info->chip_data->x_max_mt - 1, 0, 0);
	input_set_abs_params(input_dev, ABS_Y,
				0, st_info->chip_data->y_max_mt - 1, 0, 0);
	input_set_abs_params(input_dev, ABS_PRESSURE, 0, 255, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TRACKING_ID, 0, 15, 0, 0);

	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0,
				st_info->chip_data->x_max_mt - 1, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0,
				st_info->chip_data->y_max_mt - 1, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_PRESSURE, 0, 100, 0, 0);
#ifdef REPORT_2D_W
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, MAX_ABS_MT_TOUCH_MAJOR, 0, 0);
#endif
	st_info->input_dev = input_dev;

	return NO_ERR;
}

static int st_irq_top_half(struct ts_cmd_node *cmd)
{
	cmd->command = TS_INT_PROCESS;
	TS_LOG_DEBUG("st irq top half called\n");
	return NO_ERR;
}

static void st_shutdown(void)
{
	struct fts_ts_info *info = fts_get_info();

	TS_LOG_INFO("%s +\n", __func__);
	/*
	 * ST power off sequence : reset low(delay 1ms) -> 1.8 power off(delay 5ms)
	 * 3.3 power off
	 */
	gpio_set_value(info->chip_data->ts_platform_data->reset_gpio, 0);
	fts_enable_reg(info, false);

	TS_LOG_INFO("%s -\n", __func__);
	return;
}

static int st_chip_detect(struct ts_kit_platform_data *data)
{
	int retval = NO_ERR;
	struct fts_ts_info *ts = fts_get_info();
	TS_LOG_INFO("st chip detect called\n");

	if (!data) {
		TS_LOG_ERR("%s: ts_kit_platform_data is NULL\n", __func__);
		return -ENOMEM;
	}

	ts->chip_data->ts_platform_data = data;
	st_parse_dts(ts->chip_data->cnode, ts->chip_data);
	ts->dev = &data->ts_dev->dev;
	data->ts_dev->dev.of_node = ts->chip_data->cnode;
	data->client->addr = ts->chip_data->slave_addr;

	retval = fts_get_reg(ts, 1);
	if (retval){
		TS_LOG_ERR("st regulator get fail\n");
	}

	fts_enable_reg(ts, 1);
	fts_set_gpio(ts);

	retval = fts_init(ts);
	if (retval) {
		TS_LOG_ERR("st chip detect fail\n");
		goto detect_fail;
	}
	TS_LOG_INFO("st detect success\n");

	openChannel(data->client);

	init_completion(&st_roi_data_done);

	return 0;

detect_fail:
	fts_get_reg(ts, 0);
	if (ts->chip_data){
		TS_LOG_INFO("free chip data\n");
		kfree(ts->chip_data);
		ts->chip_data = NULL;
		}
	if(ts){
		TS_LOG_INFO("free memery for st_info\n");
		if (ts->event_dispatch_table) {
			kfree(ts->event_dispatch_table);
			ts->event_dispatch_table = NULL;
		}

		kfree(ts);
		ts = NULL;
	}
	return retval;
}

static int st_get_testdata(struct ts_rawdata_info *info, struct ts_cmd_node *out_cmd)
{
	int retval = 0;
	struct fts_ts_info *fts_info = fts_get_info();
	st_get_rawdata_aftertest(info,INIT_MP);
	retval |= cleanUp(0);					//system reset
	retval |= fts_mode_handler(fts_info, 0);			//enable the features and the sensing
        msleep(500);
	TS_LOG_INFO("%s:MS RAW DATA TESTsleep 500ms\n",__func__);
        retval |= fts_enable_force_key();                       //enable forcekey,because  fingerprint need this function in mmi
	retval |= fts_enableInterrupt();
	if (retval<0) {
		TS_LOG_ERR("Cannot initialize the hardware device\n");
		return retval;
	}
	return NO_ERR;
}

int check_MutualRawResGap(void)
{
	int ret = 0;
	struct ts_rawdata_info *info = NULL;
	info = (struct ts_rawdata_info *)vmalloc(sizeof(struct ts_rawdata_info));
	if(!info){
		TS_LOG_ERR(" %s:kzalloc failed\n", __func__);
		return -ENOMEM;
	}
	memset(info, 0,sizeof(struct ts_rawdata_info) );
	ret = st_get_testdata(info,NULL);
	TS_LOG_INFO(" %s: %s\n", __func__,info->result);
	if(strstr(info->result ,"-2F")){
		TS_LOG_ERR("%s: check rawgap failed !\n", __func__);
		ret = -EINVAL;
	}
	if(info){
		vfree(info);
		info = NULL;
	}
	return  ret;
}


struct ts_device_ops ts_kit_st_ops = {
	.chip_detect = st_chip_detect,
	.chip_init = st_init_chip,
	.chip_parse_config = st_parse_dts,
	.chip_input_config = st_input_config,
	.chip_irq_top_half = st_irq_top_half,
	.chip_irq_bottom_half = st_irq_bottom_half,
	.chip_fw_update_boot = st_fw_update_boot,
	.chip_fw_update_sd = st_fw_update_sd,
	.chip_get_info = st_chip_get_info,
	.chip_before_suspend = st_before_suspend,
	.chip_suspend = st_suspend,
	.chip_resume = st_resume,
	.chip_after_resume = st_after_resume,
	.chip_get_rawdata = st_get_testdata,
	.chip_glove_switch = st_glove_switch,
	.chip_shutdown = st_shutdown,
	.chip_holster_switch = st_holster_switch,
	.chip_roi_switch = st_roi_switch,
	.chip_roi_rawdata = st_ts_roi_rawdata,
	.chip_get_capacitance_test_type = st_get_capacitance_test_type,
	.chip_calibrate = st_calibrate,
	.chip_work_after_input = st_ts_work_after_input_kit,
	.oem_info_switch = st_oem_info_switch,
	.chip_get_debug_data = st_get_debug_data,
	.chip_reset = NULL,
};

static int __init st_ts_module_init(void)
{
	bool found = false;
	struct device_node *child = NULL;
	struct device_node *root = NULL;
	int error = NO_ERR;
	struct fts_ts_info *st_info;
	TS_LOG_ERR(" %s: called\n", __func__);

	st_info = fts_get_info();

	st_info = kzalloc(sizeof(struct fts_ts_info), GFP_KERNEL);
	if (!st_info) {
		TS_LOG_ERR("st get ts_info mem fail\n");
		return -ENOMEM;
	}

	st_info->chip_data = kzalloc(sizeof(struct ts_kit_device_data), GFP_KERNEL);
	if (!st_info->chip_data) {
		TS_LOG_ERR("Failed to alloc mem for struct chip_data\n");
		error =  -ENOMEM;
		goto out;
	}


	fts_set_info(st_info);

	root = of_find_compatible_node(NULL, NULL, "huawei,ts_kit");
	if (!root) {
		TS_LOG_ERR("huawei_ts, find_compatible_node huawei,ts_kit error\n");
		error = -EINVAL;
		goto out;
	}

	for_each_child_of_node(root, child) {
		if (of_device_is_compatible(child, "st_ts")) {
			found = true;
			break;
		}
	}
	if (!found) {
		TS_LOG_ERR(" not found chip st_ts child node  !\n");
		error = -EINVAL;
		goto out;
	}

	st_info->chip_data->cnode = child;
	st_info->chip_data->ops = &ts_kit_st_ops;

	error = huawei_ts_chip_register(st_info->chip_data);
	if (error) {
		TS_LOG_ERR(" sec_ts chip register fail !\n");
		goto out;
	}
	TS_LOG_ERR("st_ts chip_register! err=%d\n", error);
	return error;
out:
	if (st_info->chip_data)
		kfree(st_info->chip_data);
	if (st_info) {
		kfree(st_info);
		fts_set_info(NULL);
	}
	st_info = NULL;
	return error;
}

static void __exit st_ts_module_exit(void)
{
	TS_LOG_INFO("st_ts_module_exit called here\n");

	return;
}

late_initcall(st_ts_module_init);
module_exit(st_ts_module_exit);


MODULE_DESCRIPTION("STMicroelectronics MultiTouch IC Driver");
MODULE_AUTHOR("STMicroelectronics, Inc.");
MODULE_LICENSE("GPL");
