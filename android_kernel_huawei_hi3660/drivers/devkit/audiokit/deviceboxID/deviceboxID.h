/*
 * deviceboxID.h -- deviceboxID ALSA SoC Audio driver
 *
 * Copyright 2011-2012 Maxim Integrated Products
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _DEVICEBOXID_H
#define _DEVICEBOXID_H

enum{
	SPEAKER_ID		= 0,
	RECEIVER_ID		= 1,
	BOX_3rd_ID		= 2,
	BOX_4th_ID		= 3,
};

int deviceboxID_read(int out_id);

#endif
