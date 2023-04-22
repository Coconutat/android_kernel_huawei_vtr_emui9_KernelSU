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

#include "tcpci.h"
#include "tusb422_common.h"

#ifndef CONFIG_TUSB422

	#ifdef __MSP432P401R__
		#include "msp432.h"
	#else
		#include "msp430.h"
	#endif
	#include "smbus.h"
	#include "tcpm.h"
	#include "tcpm_hal.h"
	#include "tusb422.h"
	#include "uart.h"
	#include <stdbool.h>
	#include <stdint.h>
	#include <string.h>

	#define ENABLE_PEC   0


	#ifdef __MSP430_HAS_USCI__
extern bool HAL_Timeout_Event(void);
	#endif

static uint8_t rx_buff[SMB_MAX_PACKET_SIZE];	 /*! Buffer for TCPC response */


SMBus sSMBMaster[TOTAL_NUM_SMBUS_MASTERS];	 /*! SMBus Master Struct  */


typedef struct
{
	smbus_interface_t  intf;
	uint8_t            slave_addr;
} tcpc_device_config_t;


tcpc_device_config_t tcpc_dev_config[NUM_TCPC_DEVICES];


	#define SMBUS_TIMEOUT   10000

static int8_t _tcpc_read(unsigned int port,
						 uint8_t reg,
						 void *data,
						 uint8_t len)
{
	int8_t ret;
	SMBus *smbus_intf = &sSMBMaster[tcpc_dev_config[port].intf];

	if (len != 1 && len != 2)
	{
		return TCPM_STATUS_PARAM_ERROR;
	}

	ret = SMBus_masterReadByteWord32(smbus_intf,
									 tcpc_dev_config[port].slave_addr,
									 reg,
									 rx_buff,
									 len);

	if (ret == SMBUS_RET_OK)
	{
		if ((SMBus_masterWaitUntilDone(smbus_intf, SMBUS_TIMEOUT) != SMBUS_RET_OK) ||
			(SMBus_getState(smbus_intf) != SMBus_State_OK))
		{
			SMBus_masterReset(smbus_intf); // Force a reset, so we can retry later
			ret = TCPM_STATUS_SMBUS_ERROR;
		}
	}

	if (ret == SMBUS_RET_OK)
	{
		if (len == 1)
		{
			// Return 8-bits
			*((uint8_t *)data) =  rx_buff[0];
		}
		else
		{
			// Format the result as 16-bits
			*((uint16_t *)data) =  rx_buff[0] | (rx_buff[1] << 8);
		}
		ret = TCPM_STATUS_OK;
	}
	else
	{
		ret = TCPM_STATUS_SMBUS_ERROR;
	}

	return ret;
}

int8_t tcpc_read8(unsigned int port,
				  uint8_t reg,
				  uint8_t *data)
{
	return _tcpc_read(port, reg, data, 1);
}


int8_t tcpc_read16(unsigned int port,
				   uint8_t reg,
				   uint16_t *data)
{
	return _tcpc_read(port, reg, data, 2);
}


int8_t tcpc_read_block(unsigned int port,
					   uint8_t reg,
					   uint8_t *data,
					   unsigned int len)
{
	int8_t ret;
	SMBus *smbus_intf = &sSMBMaster[tcpc_dev_config[port].intf];

	ret = SMBus_masterReadBlock(smbus_intf,
								tcpc_dev_config[port].slave_addr,
								reg,
								rx_buff);

	if (ret == SMBUS_RET_OK)
	{
		if ((SMBus_masterWaitUntilDone(smbus_intf, SMBUS_TIMEOUT) != SMBUS_RET_OK) ||
			(SMBus_getState(smbus_intf) != SMBus_State_OK))
		{
			SMBus_masterReset(smbus_intf); // Force a reset, so we can retry later
			ret = TCPM_STATUS_SMBUS_ERROR;
		}
	}

	if (ret == SMBUS_RET_OK)
	{
		//*len = SMBus_getRxPayloadAvailable(smbus_intf);
		// Copy the data.
		memcpy(data, rx_buff, MIN(len, SMBus_getRxPayloadAvailable(smbus_intf)));

		ret = TCPM_STATUS_OK;
	}
	else
	{
		ret = TCPM_STATUS_SMBUS_ERROR;
	}

	return ret;
}


static int8_t _tcpc_write(unsigned int port,
						  uint8_t reg,
						  uint16_t data,
						  uint8_t len)
{
	int8_t ret;
	uint8_t tx_buff[2];
	SMBus *smbus_intf = &sSMBMaster[tcpc_dev_config[port].intf];

	if (len != 1 && len != 2)
	{
		return TCPM_STATUS_PARAM_ERROR;
	}

	if (len == 1)
	{
		tx_buff[0] = data;
	}
	else
	{
		tx_buff[0] = data & 0xFF;
		tx_buff[1] = (data & 0xFF00) >> 8;
	}

	ret =  SMBus_masterWriteByteWord32(smbus_intf,
									   tcpc_dev_config[port].slave_addr,
									   reg,
									   tx_buff,
									   len);

	if (ret == SMBUS_RET_OK)
	{
		if ((SMBus_masterWaitUntilDone(smbus_intf, SMBUS_TIMEOUT) != SMBUS_RET_OK) ||
			(SMBus_getState(smbus_intf) != SMBus_State_OK))
		{
			SMBus_masterReset(smbus_intf); // Force a reset, so we can retry later
			ret = TCPM_STATUS_SMBUS_ERROR;
		}
	}

	if (ret == SMBUS_RET_OK)
	{
		ret = TCPM_STATUS_OK;
	}
	else
	{
		ret = TCPM_STATUS_SMBUS_ERROR;
	}

	return ret;
}

int8_t tcpc_write8(unsigned int port,
				   uint8_t reg,
				   uint8_t data)
{
	return _tcpc_write(port, reg, (uint16_t)data, 1);
}

int8_t tcpc_write16(unsigned int port,
					uint8_t reg,
					uint16_t data)
{
	return _tcpc_write(port, reg, data, 2);
}


int8_t tcpc_write_block(unsigned int port,
						uint8_t reg,
						uint8_t *data,
						uint8_t len)
{
	int8_t ret;
	SMBus *smbus_intf = &sSMBMaster[tcpc_dev_config[port].intf];

	ret = SMBus_masterWriteBlock(smbus_intf,
								 tcpc_dev_config[port].slave_addr,
								 reg,
								 data,
								 len);

	if (ret == SMBUS_RET_OK)
	{
		if ((SMBus_masterWaitUntilDone(smbus_intf, SMBUS_TIMEOUT) != SMBUS_RET_OK) ||
			(SMBus_getState(smbus_intf) != SMBus_State_OK))
		{
			SMBus_masterReset(smbus_intf); // Force a reset, so we can retry later
			ret = TCPM_STATUS_SMBUS_ERROR;
		}
	}

	if (ret == SMBUS_RET_OK)
	{
		ret = TCPM_STATUS_OK;
	}
	else
	{
		ret = TCPM_STATUS_SMBUS_ERROR;
	}

	return ret;
}



void tcpc_modify8(unsigned int port,
				  uint8_t reg,
				  uint8_t clr_mask,
				  uint8_t set_mask)
{
	uint8_t val;
	uint8_t new_val;

	if (tcpc_read8(port, reg, &val) == TCPM_STATUS_OK)
	{
		new_val = val & ~clr_mask;
		new_val |= set_mask;

		if (new_val != val)
		{
			tcpc_write8(port, reg, new_val);
		}
	}

	return;
}

void tcpc_modify16(unsigned int port,
				   uint8_t reg,
				   uint16_t clr_mask,
				   uint16_t set_mask)
{
	uint16_t val;
	uint16_t new_val;

	if (tcpc_read16(port, reg, &val) == TCPM_STATUS_OK)
	{
		new_val = val & ~clr_mask;
		new_val |= set_mask;

		if (new_val != val)
		{
			tcpc_write16(port, reg, new_val);
		}
	}

	return;
}


void tcpc_config(unsigned int port, smbus_interface_t intf, uint8_t slave_addr)
{
	tcpc_dev_config[port].intf = intf;
	tcpc_dev_config[port].slave_addr = slave_addr;

	return;
}


void tcpc_init(void)
{
	unsigned int i;

	for (i = SMBUS_MASTER0; i < TOTAL_NUM_SMBUS_MASTERS; i++)
	{
		// Initialize interface used for TCPC communication
#ifdef __MSP432P401R__
		SMBus_masterInit(&sSMBMaster[i],
						 tcpm_hal_get_i2c_base_addr(i),
						 (MCLK_MHZ * 1000000),
						 SMBus_Freq_1MHz);
#else
		SMBus_masterInit(&sSMBMaster[i],
						 tcpm_hal_get_i2c_base_addr(i),
						 (MCLK_MHZ * 1000000),
						 SMBus_Freq_400KHz);
#endif


#if ENABLE_PEC
		SMBus_enablePEC(&sSMBMaster[i]);
#endif

		// Enable SMBus Interrupts, after initializing I2C
		SMBus_masterEnableInt(&sSMBMaster[i]);
	}

	return;
}


//*****************************************************************************
//
//! I2C Event
//!
//! Event called by HAL when I2C TX, RX, Start, Stop and errors are detected
//!
//! \return  true to wake-up MCU, false to stay in LPMx
//
// *****************************************************************************
bool HAL_I2C_Event(void)
{
	// Process SMBus ISR as Master
	SMBus_masterProcessInt(&sSMBMaster[SMBUS_MASTER0]);

	return(false);
}


//*****************************************************************************
//
//! Timer Timeout Event
//!
//! Event called by HAL when Timeout timer is detected for devices without eUSCI
//!
//! \return  true to wake-up MCU, false to stay in LPMx
//
// *****************************************************************************
	#ifdef __MSP430_HAS_USCI__
bool HAL_Timeout_Event(void)
{
	SMBus_State ret;
	// Process SMBus ISR as Master
	ret = SMBus_masterProcessTimeoutInt(&sSMBMaster[SMBUS_MASTER0]);
	if (ret != SMBus_State_OK)
	{
		CRIT("Error: SMBUS timeout! state %u.\n", ret);
	}
	return(false);
}
	#endif

#endif

uint8_t tcpc_reg_role_ctrl_set(bool drp, tcpc_role_rp_val_t rp_val, tcpc_role_cc_t cc1, tcpc_role_cc_t cc2)
{
	return(((drp) ? TCPC_ROLE_CTRL_DRP : 0) |
		   ((rp_val) << TCPC_ROLE_CTRL_RP_VALUE_SHIFT) |
		   ((cc2) << TCPC_ROLE_CTRL_CC2_SHIFT) |
		   (cc1));
}
