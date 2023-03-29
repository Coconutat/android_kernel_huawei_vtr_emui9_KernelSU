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

#include "tcpm.h"
#include "tusb422_common.h"
#include "usb_pd.h"
#include "usb_pd_policy_engine.h"
#include "usb_pd_protocol.h"
#include "version.h"
#ifdef CONFIG_TUSB422
	#include <linux/string.h>
#else
	#include <string.h>
#endif

#define PDO_DUAL_ROLE_POWER_BIT           ((uint32_t)0x01 << 29)
#define SRC_PDO_USB_SUSPEND_BIT           ((uint32_t)0x01 << 28)
#define SNK_PDO_HIGHER_CAPABILITY_BIT     ((uint32_t)0x01 << 28)
#define PDO_EXTERNALLY_POWERED_BIT        ((uint32_t)0x01 << 27)
#define PDO_USB_COMM_CAPABLE_BIT          ((uint32_t)0x01 << 26)
#define PDO_DUAL_ROLE_DATA_BIT            ((uint32_t)0x01 << 25)
#define SRC_PDO_UNCHUNKED_EXT_MSG_SUP_BIT ((uint32_t)0x01 << 24)


static usb_pd_port_config_t pd_port_config[NUM_TCPC_DEVICES];

usb_pd_port_config_t* usb_pd_pm_get_config(unsigned int port)
{
	return &pd_port_config[port];
}

uint32_t get_data_object(uint8_t *obj_data)
{
	return((((uint32_t)obj_data[3]) << 24) |
		   (((uint32_t)obj_data[2]) << 16) |
		   (((uint32_t)obj_data[1]) << 8) |
		   (((uint32_t)obj_data[0])));
}


//------------------------------------------------------------------------------
// calc_power and calc_current
//
//   Voltage is in 50mV units.
//   Current is in 10mA units.
//   Power is in 250mW units.
//------------------------------------------------------------------------------

static uint16_t calc_power(uint32_t voltage_50mv, uint32_t current_10ma)
{
	return (voltage_50mv * current_10ma) / 500;
}

static uint16_t calc_current(uint32_t power_250mw, uint32_t voltage_50mv)
{
	return (power_250mw * 500) / voltage_50mv;
}


typedef struct
{
	enum supply_type_t supply_type;
	uint16_t min_voltage;
	uint16_t max_voltage;
	uint16_t max_current;
	uint16_t max_power;
	bool cap_mismatch;
} pdo_offer_t;

//------------------------------------------------------------------------------------------
// Returns true if the new_offer is better than the current_offer, else returns false.
//------------------------------------------------------------------------------------------
static bool better_offer(pdo_offer_t *new_offer, pdo_offer_t *current_offer, pdo_priority_t pdo_priority)
{
	// Compare offers based on user defined priority (voltage, current, or power).
	if (pdo_priority == PRIORITY_VOLTAGE)
	{
		if (new_offer->min_voltage > current_offer->min_voltage)
		{
			return true;
		}
		else if (new_offer->min_voltage == current_offer->min_voltage)
		{
			if (new_offer->max_voltage > current_offer->max_voltage)
			{
				return true;
			}
		}
	}
	else if (pdo_priority == PRIORITY_CURRENT)
	{
		if (new_offer->max_current > current_offer->max_current)
		{
			return true;
		}
		else if (new_offer->max_current == current_offer->max_current)
		{
			if (new_offer->min_voltage > current_offer->min_voltage)
			{
				return true;
			}
		}
	}
	else /* PRIORITY_POWER */
	{
		if (new_offer->max_power > current_offer->max_power)
		{
			return true;
		}
		else if (new_offer->max_power == current_offer->max_power)
		{
			if (new_offer->min_voltage > current_offer->min_voltage)
			{
				return true;
			}
		}
	}

	// Both offers are equal in terms of user defined priority
	// so pick based on supply type:  Fixed > Variable > Battery.
	if (new_offer->supply_type != current_offer->supply_type)
	{
		if (new_offer->supply_type == SUPPLY_TYPE_FIXED)
		{
			return true;
		}
		else if (current_offer->supply_type == SUPPLY_TYPE_FIXED)
		{
			return false;
		}
		else if (new_offer->supply_type == SUPPLY_TYPE_VARIABLE)
		{
			return true;
		}
	}

	return false;
}

void usb_pd_pm_evaluate_src_caps(unsigned int port)
{
	usb_pd_port_config_t *config = usb_pd_pm_get_config(port);
	usb_pd_port_t *dev = usb_pd_pe_get_device(port);
	uint8_t num_offered_pdos;
	uint8_t snk_pdo_idx;
	uint8_t src_pdo_idx;
	uint8_t *pdo_data = dev->rx_msg_buf;
	uint32_t pdo;
	bool acceptable_pdo;
	pdo_offer_t offer[2];
	uint8_t     offer_idx = 0;
	pdo_offer_t *selected_offer;
	pdo_offer_t *new_offer;

	// Divide the Rx'd msg length by 4 to get the number of source PDOs offered.
	num_offered_pdos = dev->rx_msg_data_len >> 2;

	// Initialize RDO object position to zero (invalid value).
	dev->object_position = 0;

	// Initialize offer pointers.
	new_offer = &offer[offer_idx];
	selected_offer = NULL;

	// Evaluate each PDO offered in source caps.
	for (src_pdo_idx = 0; src_pdo_idx < num_offered_pdos; src_pdo_idx++)
	{
		// Using get_data_object() instead of casting to 32-bit pointer
		// in case pdo_data pointer is not 4-byte aligned.
		pdo = get_data_object(&pdo_data[src_pdo_idx << 2]);

		INFO("PDO[%u] = 0x%08x\n", src_pdo_idx, pdo);

		if (src_pdo_idx == 0)
		{
			dev->remote_externally_powered = (pdo & PDO_EXTERNALLY_POWERED_BIT) ? true : false;
			INFO("Remote SRC_CAPS externally powered bit is %s.\n", (dev->remote_externally_powered) ? "set" : "not set");
		}

		// Extract the offer params from the source PDO.
		new_offer->supply_type = (enum supply_type_t)PDO_SUPPLY_TYPE(pdo);
		new_offer->min_voltage = PDO_MIN_VOLTAGE(pdo);

		if (new_offer->supply_type == SUPPLY_TYPE_FIXED)
		{
			new_offer->max_voltage = new_offer->min_voltage;
		}
		else
		{
			new_offer->max_voltage = PDO_MAX_VOLTAGE(pdo);
		}

		if (new_offer->supply_type == SUPPLY_TYPE_BATTERY)
		{
			new_offer->max_power = PDO_MAX_CURRENT_OR_POWER(pdo);
			new_offer->max_current = calc_current(new_offer->max_power, new_offer->max_voltage);
		}
		else
		{
			new_offer->max_current = PDO_MAX_CURRENT_OR_POWER(pdo);
			new_offer->max_power = calc_power(new_offer->max_voltage, new_offer->max_current);
		}

		CRIT("PDO-%u %s, ", src_pdo_idx + 1,
			 (new_offer->supply_type == SUPPLY_TYPE_FIXED) ? "Fixed" :
			 (new_offer->supply_type == SUPPLY_TYPE_VARIABLE) ? "Vari" : "Batt");
		CRIT("%u - ", PDO_VOLT_TO_MV(new_offer->min_voltage));
		CRIT("%u mV, ", PDO_VOLT_TO_MV(new_offer->max_voltage));
		CRIT("%u mA, ", PDO_CURR_TO_MA(new_offer->max_current));
		CRIT("%u mW\n", PDO_PWR_TO_MW(new_offer->max_power));

		for (snk_pdo_idx = 0; snk_pdo_idx < config->num_snk_pdos; snk_pdo_idx++)
		{
			// Make sure the SrcCap and SinkCap supply type match:
			// A Fixed Supply SinkCap PDO can only consider a Fixed Supply SrcCap PDO
			// A Variable Supply SinkCap PDO can consider a Fixed or Variable Supply SrcCap PDO
			// A Battery Supply SinkCap PDO can consider any Supply SrcCap PDO
			if ((new_offer->supply_type != config->snk_caps[snk_pdo_idx].SupplyType) &&
				(config->snk_caps[snk_pdo_idx].SupplyType != SUPPLY_TYPE_BATTERY) &&
				((new_offer->supply_type == SUPPLY_TYPE_BATTERY) &&
				 (config->snk_caps[snk_pdo_idx].SupplyType == SUPPLY_TYPE_VARIABLE)))
			{
				continue;
			}

			if (config->snk_caps[snk_pdo_idx].SupplyType == SUPPLY_TYPE_FIXED)
			{
				config->snk_caps[snk_pdo_idx].MaxV = config->snk_caps[snk_pdo_idx].MinV;
			}

			acceptable_pdo = false;

			// Determine whether PDO is acceptable.
			if ((new_offer->max_voltage <= config->snk_caps[snk_pdo_idx].MaxV) &&
				(new_offer->min_voltage >= config->snk_caps[snk_pdo_idx].MinV))
			{
				if (config->snk_caps[snk_pdo_idx].SupplyType == SUPPLY_TYPE_BATTERY)
				{
					if (new_offer->max_power >= config->snk_caps[snk_pdo_idx].OperationalPower)
					{
						acceptable_pdo = true;

						// Check for capability mismatch.
						new_offer->cap_mismatch = (new_offer->max_power < config->snk_caps[snk_pdo_idx].MaxOperatingPower) ? true : false;
					}
				}
				else /* Fixed or Variable Sink Cap */
				{
					if (new_offer->max_current >= config->snk_caps[snk_pdo_idx].OperationalCurrent)
					{
						acceptable_pdo = true;

						// Check for capability mismatch.
						new_offer->cap_mismatch = (new_offer->max_current < config->snk_caps[snk_pdo_idx].MaxOperatingCurrent) ? true : false;
					}
				}
			}

			// Select this PDO if
			// Firstly, it is acceptable
			// [AND] Secondly,
			// no PDO has been selected yet [OR]
			// the capability is a better match  [OR]
			// it is a better offer based on user-defined priority.
			if (acceptable_pdo &&
				((selected_offer == NULL) ||
				 (selected_offer->cap_mismatch && !new_offer->cap_mismatch) ||
				  better_offer(new_offer, selected_offer, config->pdo_priority)))
			{
				selected_offer = new_offer;
				dev->selected_pdo = pdo;
				dev->object_position = src_pdo_idx + 1;
				dev->selected_snk_pdo_idx = snk_pdo_idx;

				// Switch pointer to current offer.
				offer_idx ^= 1;
				new_offer = &offer[offer_idx];
			}
		} /* End: Sink PDO loop */
	} /* End: Source PDO loop */

	if (selected_offer == NULL)
	{
		// No acceptable PDO was found. Use first PDO (vSafe5V).
		dev->object_position = 1;
		dev->selected_pdo = get_data_object(&pdo_data[0]);
		dev->selected_snk_pdo_idx = 0;
	}

	CRIT("PDO-%u 0x%08x selected\n", dev->object_position, dev->selected_pdo);
	DEBUG("selected_snk_pdo_idx: %u\n", dev->selected_snk_pdo_idx);

	return;
}


void build_src_caps(unsigned int port)
{
	unsigned int n;
	usb_pd_port_config_t *config = &pd_port_config[port];
	usb_pd_port_t *dev = usb_pd_pe_get_device(port);
	tcpc_device_t *typec_dev = tcpm_get_device(port);

	// Clear PDOs.
	for (n = 0; n < config->num_src_pdos; n++)
	{
		dev->src_pdo[n] = 0;
	}

	if (typec_dev->role == ROLE_DRP)
	{
		dev->src_pdo[0] |= PDO_DUAL_ROLE_POWER_BIT;
	}

	if (config->usb_suspend_supported)
	{
		dev->src_pdo[0] |= SRC_PDO_USB_SUSPEND_BIT;
	}

	if (dev->externally_powered)
	{
		dev->src_pdo[0] |= PDO_EXTERNALLY_POWERED_BIT;
	}

	if (config->usb_comm_capable)
	{
		dev->src_pdo[0] |= PDO_USB_COMM_CAPABLE_BIT;
	}

	if (config->dual_role_data)
	{
		dev->src_pdo[0] |= PDO_DUAL_ROLE_DATA_BIT;
	}

	if (config->unchunked_msg_support)
	{
		dev->src_pdo[0] |= SRC_PDO_UNCHUNKED_EXT_MSG_SUP_BIT;
	}

	for (n = 0; n < config->num_src_pdos; n++)
	{
		// Note: Source PDOs my need to be hidden or modified based on voltage
		// and current limits of the cable.

		// PDO(n)[31:30] = SourceCapabilities.PDO(n).SupplyType.
		dev->src_pdo[n] |= ((uint32_t)(config->src_caps[n].SupplyType) & 0x03) << 30;

		if (config->src_caps[n].SupplyType == SUPPLY_TYPE_FIXED)
		{
			// PDO(n)[21:20] = SourceCapabilities.PDO(n).PeakCurrent.
			dev->src_pdo[n] |= ((uint32_t)(config->src_caps[n].PeakI) & 0x03) << 20;
		}
		else /* Variable or Battery */
		{
			// PDO(n)[29:20] = SourceCapabilities.PDO(n).MaximumVoltage.
			dev->src_pdo[n] |= ((uint32_t)(config->src_caps[n].MaxV) & 0x3FF) << 20;
		}

		// PDO(n)[19:10] = SourceCapabilities.PDO(n).MinimumVoltage.
		dev->src_pdo[n] |= ((uint32_t)(config->src_caps[n].MinV) & 0x3FF) << 10;

		if (config->src_caps[n].SupplyType == SUPPLY_TYPE_BATTERY)
		{
			// PDO(n)[9:0] = SourceCapabilities.PDO(n).MaximumPower.
			dev->src_pdo[n] |= ((uint32_t)(config->src_caps[n].MaxPower) & 0x3FF);
		}
		else /* Fixed or Variable */
		{
			// PDO(n)[9:0] = SourceCapabilities.PDO(n).MaximumCurrent.
			dev->src_pdo[n] |= ((uint32_t)(config->src_caps[n].MaxI) & 0x3FF);
		}
	}

	return;
}


void build_snk_caps(unsigned int port)
{
	unsigned int n;
	usb_pd_port_config_t *config = &pd_port_config[port];
	usb_pd_port_t *dev = usb_pd_pe_get_device(port);
	tcpc_device_t *typec_dev = tcpm_get_device(port);

	// Clear PDOs.
	for (n = 0; n < config->num_snk_pdos; n++)
	{
		dev->snk_pdo[n] = 0;
	}

	if (typec_dev->role == ROLE_DRP)
	{
		dev->snk_pdo[0] |= PDO_DUAL_ROLE_POWER_BIT;
	}

	if (config->higher_capability)
	{
		dev->snk_pdo[0] |= SNK_PDO_HIGHER_CAPABILITY_BIT;
	}

	if (dev->externally_powered)
	{
		dev->snk_pdo[0] |= PDO_EXTERNALLY_POWERED_BIT;
	}

	if (config->usb_comm_capable)
	{
		dev->snk_pdo[0] |= PDO_USB_COMM_CAPABLE_BIT;
	}

	if (config->dual_role_data)
	{
		dev->snk_pdo[0] |= PDO_DUAL_ROLE_DATA_BIT;
	}

	dev->snk_pdo[0] |= ((uint32_t)config->fast_role_swap_support & 0x03) << 23;

	for (n = 0; n < config->num_snk_pdos; n++)
	{
		// SinkPDO(n)(31:30) = pSupplyType(n).
		dev->snk_pdo[n] |= ((uint32_t)(config->snk_caps[n].SupplyType) & 0x03) << 30;

		// SinkPDO(n)(19:10) = pMinimumVoltage(n).
		dev->snk_pdo[n] |= ((uint32_t)(config->snk_caps[n].MinV) & 0x3FF) << 10;

		if (config->snk_caps[n].SupplyType != SUPPLY_TYPE_FIXED)
		{
			// SinkPDO(n)(29:20) = pMaximumVoltage(n).
			dev->snk_pdo[n] |= ((uint32_t)(config->snk_caps[n].MaxV) & 0x3FF) << 20;
		}

		if (config->snk_caps[n].SupplyType == SUPPLY_TYPE_BATTERY)
		{
			// SinkPDO(n)(9:0) = pOperationalPower(n).
			dev->snk_pdo[n] |= ((uint32_t)(config->snk_caps[n].OperationalPower) & 0x3FF);
		}
		else /* Fixed or Variable */
		{
			// SinkPDO(n)(9:0) = pOperationalCurrent(n).
			dev->snk_pdo[n] |= ((uint32_t)(config->snk_caps[n].OperationalCurrent) & 0x3FF);
		}
	}

	return;
}

typedef enum
{
	RDO_GIVEBACK_FLAG         = ((uint32_t)1 << 27),
	RDO_CAPABILITY_MISMATCH   = ((uint32_t)1 << 26),
	RDO_USB_COMM_CAPABLE      = ((uint32_t)1 << 25),
	RDO_NO_USB_SUSPEND        = ((uint32_t)1 << 24),
	RDO_UNCHUNKED_MSG_SUPPORT = ((uint32_t)1 << 23)
} rdo_bits_t;

void build_rdo(unsigned int port)
{
	uint32_t rdo = 0;
	usb_pd_port_t *dev = usb_pd_pe_get_device(port);
	usb_pd_port_config_t *config = usb_pd_pm_get_config(port);

	rdo |= ((uint32_t)dev->object_position) << 28;

	if (config->giveback_flag)
	{
		rdo |= RDO_GIVEBACK_FLAG;
	}

	if ((dev->object_position == 1) && config->higher_capability)
	{
		// Sink requires voltage greater than 5V.
		rdo |= RDO_CAPABILITY_MISMATCH;
	}

	if (config->usb_comm_capable)
	{
		rdo |= RDO_USB_COMM_CAPABLE;
	}

	if (config->no_usb_suspend)
	{
		rdo |= RDO_NO_USB_SUSPEND;
	}

	if (config->unchunked_msg_support)
	{
		rdo |= RDO_UNCHUNKED_MSG_SUPPORT;
	}

	if (PDO_SUPPLY_TYPE(dev->selected_pdo) == SUPPLY_TYPE_BATTERY)
	{
		if (PDO_MAX_CURRENT_OR_POWER(dev->selected_pdo) < config->snk_caps[dev->selected_snk_pdo_idx].MaxOperatingPower)
		{
			// Sink requires higher power for full operation.
			rdo |= RDO_CAPABILITY_MISMATCH;

			rdo |= PDO_MAX_CURRENT_OR_POWER(dev->selected_pdo) << 10;
		}
		else
		{
			rdo |= ((uint32_t)config->snk_caps[dev->selected_snk_pdo_idx].OperationalPower & 0x3FF) << 10;
		}

		if (config->giveback_flag)
		{
			rdo |= config->snk_caps[dev->selected_snk_pdo_idx].MinOperatingPower & 0x3FF;
		}
		else
		{
			rdo |= config->snk_caps[dev->selected_snk_pdo_idx].MaxOperatingPower & 0x3FF;
		}
	}
	else /* Fixed or Variable supply */
	{
		if (PDO_MAX_CURRENT_OR_POWER(dev->selected_pdo) < config->snk_caps[dev->selected_snk_pdo_idx].MaxOperatingCurrent)
		{
			// Sink requires higher current for full operation.
			rdo |= RDO_CAPABILITY_MISMATCH;

			rdo |= PDO_MAX_CURRENT_OR_POWER(dev->selected_pdo) << 10;
		}
		else
		{
			rdo |= ((uint32_t)config->snk_caps[dev->selected_snk_pdo_idx].OperationalCurrent & 0x3FF) << 10;
		}

		if (config->giveback_flag)
		{
			rdo |= config->snk_caps[dev->selected_snk_pdo_idx].MinOperatingCurrent & 0x3FF;
		}
		else
		{
			rdo |= config->snk_caps[dev->selected_snk_pdo_idx].MaxOperatingCurrent & 0x3FF;
		}
	}

	dev->rdo = rdo;

	return;
}


void usb_pd_print_version(void)
{
	CRIT(" ________  _________  ____ ___  ___\n");
	CRIT("/_  __/ / / / __/ _ )/ / /|_  ||_  |\n");
	CRIT(" / / / /_/ /\\ \\/ _  /_  _/ __// __/\n");
	CRIT("/_/  \\____/___/____/ /_//____/____/\n");
	CRIT("\n");
	PRINT("PD Stack v%u.%02u\n", PD_LIB_VERSION_MAJOR, PD_LIB_VERSION_MINOR);
	CRIT("=====================================\n");
	return;
}


void usb_pd_init(const usb_pd_port_config_t *port_config)
{
	unsigned int port;

	usb_pd_prl_init();

	memcpy(pd_port_config, port_config, sizeof(pd_port_config));

	for (port = 0; port < NUM_TCPC_DEVICES; port++)
	{
		usb_pd_pe_init(port, &pd_port_config[port]);
	}

	return;
}
