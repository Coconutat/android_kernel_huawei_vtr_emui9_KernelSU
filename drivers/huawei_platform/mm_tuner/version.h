/***************************************************************************//**
 *
 *  @file		version.h
 *
 *  @brief		define version of mm_tuner driver
 *
 *  @data		2015.04.30
 *
 *  @author	K.Okawa(IoT1)
 *
 ****************************************************************************//*
 * Copyright (c) 2015 Socionext Inc.
 *******************************************************************************
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 ******************************************************************************/
/*..+....1....+....2....+....3....+....4....+....5....+....6....+....7....+...*/
#ifndef _MMTUNER_VERSION_H
#define _MMTUNER_VERSION_H

#ifndef MMTUENR_DEVICE
#define MMTUNER_DEVICE	(0x53)	//!< Target device id. (MN885xx)
#endif
#ifndef MMTUNER_MAJOR
#define MMTUNER_MAJOR	(0)
#endif
#ifndef MMTUNER_MINOR
#define MMTUNER_MINOR	(4)
#endif
#ifndef MMTUNER_HOTFIX
#define MMTUNER_HOTFIX	(1)
#endif
/*
 * MMTUNER_RC is the release candidate suffix.
 * Should normally be empty.
 */
#ifndef MMTUNER_RC
#define MMTUNER_RC		""
#endif

#ifndef MMTUNER_DESC
#define MMTUNER_DESC		""
#endif

#endif
/*******************************************************************************
 * Copyright (c) 2015 Socionext Inc.
 ******************************************************************************/
