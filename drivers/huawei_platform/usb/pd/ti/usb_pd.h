/*
 * TUSB422 Power Delivery
 *
 * Author: Brian Quach <brian.quach@ti.com>
 *
 * Copyright (C) 2016 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef __USB_PD_H__
#define __USB_PD_H__

// All values in this header are defined by the USB PD spec.

#define PD_REV20 0x01  /* Rev 2.0 */
#define PD_REV30 0x02  /* Rev 3.0 */

#define PD_SPEC_REV  PD_REV20

#define PD_PWR_ROLE_SNK 0
#define PD_PWR_ROLE_SRC 1

#define PD_DATA_ROLE_UFP  0
#define PD_DATA_ROLE_DFP  1

#ifdef CABLE_PLUG
	#define N_RETRY_COUNT    0   /* No retry allowed for cable plugs per USBPD3 spec */
#else
	#if PD_SPEC_REV == PD_REV30 /* Rev 3.0 */
		#define N_RETRY_COUNT    2   /* nRetryCount=2 per USBPD3 spec */
	#else /* Rev 2.0 */
		#define N_RETRY_COUNT    3   /* nRetryCount=3 per USBPD3 spec */
	#endif
#endif

/* Control msg when number of data objects is zero */
typedef enum
{
	CTRL_MSG_TYPE_GOOD_CRC        = 0x01,
	CTRL_MSG_TYPE_GOTO_MIN        = 0x02, /* SOP only */
	CTRL_MSG_TYPE_ACCEPT          = 0x03,
	CTRL_MSG_TYPE_REJECT          = 0x04, /* SOP only */
	CTRL_MSG_TYPE_PING            = 0x05, /* SOP only */
	CTRL_MSG_TYPE_PS_RDY          = 0x06, /* SOP only */
	CTRL_MSG_TYPE_GET_SRC_CAP     = 0x07, /* SOP only */
	CTRL_MSG_TYPE_GET_SNK_CAP     = 0x08, /* SOP only */
	CTRL_MSG_TYPE_DR_SWAP         = 0x09, /* SOP only */
	CTRL_MSG_TYPE_PR_SWAP         = 0x0A, /* SOP only */
	CTRL_MSG_TYPE_VCONN_SWAP      = 0x0B, /* SOP only */
	CTRL_MSG_TYPE_WAIT            = 0x0C, /* SOP only */
	CTRL_MSG_TYPE_SOFT_RESET      = 0x0D,
	CTRL_MSG_TYPE_NOT_SUPPORTED   = 0x10, /* PD v3.0 */
	CTRL_MSG_TYPE_GET_SRC_CAP_EXT = 0x11, /* PD v3.0 - SOP only */
	CTRL_MSG_TYPE_GET_STATUS      = 0x12, /* PD v3.0 - SOP only */
	CTRL_MSG_TYPE_FR_SWAP         = 0x13  /* PD v3.0 - SOP only */
} msg_hdr_ctrl_msg_type_t;

/* Data msg when number of data objects is non-zero */
typedef enum
{
	DATA_MSG_TYPE_SRC_CAPS    = 0x01,  /* SOP only */
	DATA_MSG_TYPE_REQUEST     = 0x02,  /* SOP only */
	DATA_MSG_TYPE_BIST        = 0x03,
	DATA_MSG_TYPE_SNK_CAPS    = 0x04,  /* SOP only */
	DATA_MSG_TYPE_BATT_STATUS = 0x05,  /* SOP only */
	DATA_MSG_TYPE_ALERT       = 0x06,  /* SOP only */
	DATA_MSG_TYPE_VENDOR      = 0x0F
} msg_hdr_data_msg_type_t;

typedef enum
{
	EXT_MSG_TYPE_SRC_CAPS_EXT       = 0x01,	  /* SOP only */
	EXT_MSG_TYPE_STATUS             = 0x02,	  /* SOP only */
	EXT_MSG_TYPE_GET_BATT_CAP       = 0x03,	  /* SOP only */
	EXT_MSG_TYPE_GET_BATT_STATUS    = 0x04,	  /* SOP only */
	EXT_MSG_TYPE_BATT_CAPABILITIES  = 0x05,	  /* SOP only */
	EXT_MSG_TYPE_GET_MANUF_INFO     = 0x06,
	EXT_MSG_TYPE_MANUF_INFO         = 0x07,
	EXT_MSG_TYPE_SECURITY_REQUEST   = 0x08,
	EXT_MSG_TYPE_SECURITY_RESPONSE  = 0x09,
	EXT_MSG_TYPE_FW_UPDATE_REQUEST  = 0x0A,
	EXT_MSG_TYPE_FW_UPDATE_RESPONSE = 0x0B
} ext_msg_hdr_msg_type_t;


#define STRUCT_VDM_VER_1P0 0x00  /* Ver 1.0 */
#define STRUCT_VDM_VER_2P0 0x01  /* Ver 2.0 */

#define STRUCTURED_VDM_VER  STRUCT_VDM_VER_1P0

typedef enum
{
	VDM_PD_SID          = 0xFF00,
	VDM_DISPLAYPORT_VID = 0xFF01,
	VDM_HDMI_VID        = 0xFF04,
	VDM_THUNDERBOLT_VID = 0x8087,
	VDM_APPLE_VID       = 0x05AC
} svid_t;

typedef enum
{
	VDM_INITIATOR = 0,
	VDM_RESP_ACK  = 1,
	VDM_RESP_NACK = 2,
	VDM_RESP_BUSY = 3
} vdm_command_type_t;

typedef enum
{
	VDM_CMD_DISCOVER_IDENTITY = 0x01,
	VDM_CMD_DISCOVER_SVIDS    = 0x02,
	VDM_CMD_DISCOVER_MODES    = 0x03,
	VDM_CMD_ENTER_MODE        = 0x04,
	VDM_CMD_EXIT_MODE         = 0x05,
	VDM_CMD_ATTENTION         = 0x06,
	VDM_CMD_SVID_SPECIFIC_MIN = 0x10,
	VDM_CMD_SVID_SPECIFIC_MAX = 0x1F
} vdm_command_t;
typedef enum
{
	VDM_CMD_DP_STATUS_UPDATE  = 0x10, /* DisplayPort specific commands */
	VDM_CMD_DP_CONFIG,
	VDM_CMD_DP_LIMIT
} vdm_command_dp_t;
typedef enum
{
	VDM_CMD_HDMI_STATUS_UPDATE  = 0x10,
	VDM_CMD_HDMI_CEC_WRITE,
	VDM_CMD_HDMI_CEC_READ,
	VDM_CMD_HDMI_DDC_WRITE,
	VDM_CMD_HDMI_DDC_READ,
	VDM_CMD_HDMI_LIMIT
} vdm_command_hdmi_t;

typedef enum
{
	VDM_TYPE_UNSTRUCTURED = 0,
	VDM_TYPE_STRUCTURED   = 1
} vdm_type_t;

/* Data role field is reserved for SOP'/SOP" */
#define USB_PD_HDR_GEN_BYTE0(data_role, msg_type)  ( ((PD_SPEC_REV) << 6) | ((data_role) << 5) | (msg_type) )
#define USB_PD_HDR_GEN_BYTE1(ext, num_data_obj, msg_id, power_role)  ( ((ext) << 7) | ((num_data_obj) << 4) | ((msg_id & 0x07) << 1) | (power_role) )

/* Ext = 0 */
#define USB_PD_HDR_GEN(data_role, msg_type, num_data_obj, msg_id, power_role)  ((USB_PD_HDR_GEN_BYTE1(0, num_data_obj, msg_id, power_role) << 8) | (USB_PD_HDR_GEN_BYTE0(data_role, msg_type)))

#define USB_PD_HDR_GET_DATA_LEN(header)  ((((header) >> 12) & 0x07) << 2)  /* number of data objects x 4-bytes per object */
#define USB_PD_HDR_GET_MSG_ID(header)    (((header) >> 9) & 0x07)
#define USB_PD_HDR_GET_MSG_TYPE(header)  ((header) & 0x1F)
#define USB_PD_HDR_GET_DATA_ROLE(header) (((header) >> 5) & 0x01)
#define USB_PD_HDR_GET_SPEC_REV(header)  (((header) >> 6) & 0x03)

#define USB_PD_VDM_HDR_GEN(svid, vdm_type, obj_pos, cmd_type, cmd) \
	(((uint32_t)(svid) << 16) | ((uint32_t)(vdm_type) << 15) | ((STRUCTURED_VDM_VER) << 13) | ((obj_pos) << 8) | ((cmd_type) << 6) | (cmd))

#define USB_PD_VDM_HDR_GET_SVID(vdm_header)     (((vdm_header) >> 16) & 0xFFFF)
#define USB_PD_VDM_IS_STRUCT_TYPE(vdm_header)   ((vdm_header) & 0x8000)
#define USB_PD_VDM_HDR_GET_VER(vdm_header)      (((vdm_header) >> 13) & 0x03)
#define USB_PD_VDM_HDR_GET_OBJ_POS(vdm_header)  (((vdm_header) >> 8) & 0x07)
#define USB_PD_VDM_HDR_GET_CMD_TYPE(vdm_header) (((vdm_header) >> 6) & 0x03)
#define USB_PD_VDM_HDR_GET_CMD(vdm_header)      ((vdm_header) & 0x1F)

#define USB_PD_VDM_MODAL_OPERATION(vdm_id_hdr)  ((vdm_id_hdr) & ((uint32_t)1 << 26))

#define PDO_MAX_CURRENT_OR_POWER(pdo) ((pdo) & 0x3FF)
#define PDO_MIN_VOLTAGE(pdo)          (((pdo) >> 10) & 0x3FF)
#define PDO_MAX_VOLTAGE(pdo)          (((pdo) >> 20) & 0x3FF)
#define PDO_SUPPLY_TYPE(pdo)          ((pdo) >> 30)

#define RDO_OPERATIONAL_CURRENT_OR_POWER(rdo)     (((rdo) >> 10) & 0x3FF)
#define RDO_MIN_OPERATIONAL_CURRENT_OR_POWER(rdo) ((rdo) & 0x3FF)

#define PDO_VOLT_TO_MV(pdo_v)  ((pdo_v) * 50)
#define PDO_CURR_TO_MA(pdo_i)  ((pdo_i) * 10)
#define PDO_PWR_TO_MW(pdo_p)   ((pdo_p) * 250)


#endif
