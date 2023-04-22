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

#include "tusb422_common.h"
#include "tcpm.h"
#ifdef CONFIG_TUSB422_PAL
	#include "usb_pd_pal.h"
#endif
#ifndef CONFIG_TUSB422
	#include "timer.h"
	#include "tcpm_hal.h"
#endif

#ifdef CONFIG_TUSB422_PAL

#ifdef CONFIG_CONTEXTHUB_PD
#include <linux/hisi/contexthub/tca.h>
#include <huawei_platform/usb/hw_pd_dev.h>

extern void dp_aux_switch_op(uint32_t value);
extern void dp_aux_uart_switch_enable(void);
extern void pd_dpm_send_event(enum pd_dpm_cable_event_type event);

uint8_t g_polarity;
uint8_t g_mux_type;
extern int support_dp;
#endif
#endif
#ifdef CONFIG_TUSB422

int8_t tcpc_read8(unsigned int port, uint8_t reg, uint8_t *data)
{
	return tusb422_read(reg, data, 1);
}

int8_t tcpc_read16(unsigned int port, uint8_t reg, uint16_t *data)
{
	return tusb422_read(reg, data, 2);
}

int8_t tcpc_read_block(unsigned int port, uint8_t reg, uint8_t *data,
					   unsigned int len)
{
	return tusb422_read(reg, data, len);
}

int8_t tcpc_write8(unsigned int port, uint8_t reg, uint8_t data)
{
	return tusb422_write(reg, &data, 1);
};

int8_t tcpc_write16(unsigned int port, uint8_t reg, uint16_t data)
{
	return tusb422_write(reg, &data, 2);
}

int8_t tcpc_write_block(unsigned int port, uint8_t reg, uint8_t *data, uint8_t len)
{
	return tusb422_write(reg, data, len);
}

// Modifies an 8-bit register.
void tcpc_modify8(unsigned int port,
				  uint8_t reg,
				  uint8_t clr_mask,
				  uint8_t set_mask)
{
	tusb422_modify_reg(reg, clr_mask, set_mask);
}

// Modifies an 16-bit register.
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

int timer_start(struct tusb422_timer_t *timer,
				unsigned int timeout_ms,
				void (*function)(unsigned int))
{
	tusb422_set_timer_func(*function);
	tusb422_start_timer(timeout_ms);

	return 0;
}

void timer_cancel(struct tusb422_timer_t *timer)
{
	tusb422_clr_timer_func();
	tusb422_stop_timer();
}

#endif


void tcpm_msleep(int msecs)
{
#ifdef CONFIG_TUSB422
	tusb422_msleep(msecs);
#else /* For MSP430/432 platform */
	msleep(msecs);
#endif
}

void tcpm_source_vconn(uint8_t port, bool enable)
{
#ifdef CONFIG_TUSB422_PAL
	usb_pd_pal_source_vconn(port, enable);
#elif defined CONFIG_TUSB422 /* For BeagleBone Black EVM platform */
#else /* For MSP430/432 platform */
#endif
	return;
}
void tcpm_source_vbus(uint8_t port, bool usb_pd, uint16_t mv)
{
	tcpm_src_vbus_enable(port, mv);
#ifdef CONFIG_TUSB422_PAL

	usb_pd_pal_source_vbus(port, usb_pd, mv, 0);  /* using 0 for current since the platform doesn't use this data */

#elif defined CONFIG_TUSB422 /* For BeagleBone Black EVM platform */
	if (mv == 5000)
	{
		tusb422_set_vbus(VBUS_SEL_SRC_5V);
	}
	else
	{
		tusb422_set_vbus(VBUS_SEL_SRC_HI_VOLT);
	}
#else /* For MSP430/432 platform */
	if (mv == 5000)
	{
		tcpm_hal_vbus_enable(port, VBUS_SRC_5V);
	}
	else
	{
		tcpm_hal_vbus_enable(port, VBUS_SRC_HI_VOLT);
	}
#endif

	return;
}

void tcpm_source_vbus_disable(uint8_t port)
{
#ifdef CONFIG_TUSB422_PAL
	usb_pd_pal_disable_vbus(port);
#elif defined CONFIG_TUSB422 /* For BeagleBone Black EVM platform */
	tusb422_clr_vbus(VBUS_SEL_SRC_5V);
	tusb422_clr_vbus(VBUS_SEL_SRC_HI_VOLT);
#else /* For MSP430/432 platform */
	tcpm_hal_vbus_disable(port, VBUS_SRC_5V);
	tcpm_hal_vbus_disable(port, VBUS_SRC_HI_VOLT);
#endif
	return;
}

void tcpm_sink_vbus(uint8_t port, bool usb_pd, uint16_t mv, uint16_t ma)
{
	tcpm_snk_vbus_enable(port);
#ifdef CONFIG_TUSB422_PAL
	usb_pd_pal_sink_vbus(port, usb_pd, mv, ma);
#elif defined CONFIG_TUSB422 /* For BeagleBone Black EVM platform */
	tusb422_set_vbus(VBUS_SEL_SNK);
#else /* For MSP430/432 platform */
	tcpm_hal_vbus_enable(port, VBUS_SNK);
#endif
	return;
}

void tcpm_sink_vbus_batt(uint8_t port, uint16_t min_mv, uint16_t max_mv, uint16_t mw)
{
#ifdef CONFIG_TUSB422_PAL
	usb_pd_pal_sink_vbus_batt(port, min_mv, max_mv, mw);
#elif defined CONFIG_TUSB422 /* For BeagleBone Black EVM platform */
#else /* For MSP430/432 platform */
#endif
	return;
}

void tcpm_sink_vbus_vari(uint8_t port, uint16_t min_mv, uint16_t max_mv, uint16_t ma)
{
#ifdef CONFIG_TUSB422_PAL
	usb_pd_pal_sink_vbus_vari(port, min_mv, max_mv, ma);
#elif defined CONFIG_TUSB422 /* For BeagleBone Black EVM platform */
#else /* For MSP430/432 platform */
#endif
	return;
}

void tcpm_sink_vbus_disable(uint8_t port)
{
#ifdef CONFIG_TUSB422_PAL
	usb_pd_pal_disable_vbus(port);
#elif defined CONFIG_TUSB422 /* For BeagleBone Black EVM platform */
	tusb422_clr_vbus(VBUS_SEL_SNK);
#else /* For MSP430/432 platform */
	tcpm_hal_vbus_disable(port, VBUS_SNK);
#endif
	return;
}

void tcpm_mux_control(uint8_t port, uint8_t data_role, mux_ctrl_t ctrl, uint8_t polarity)
{
#ifdef CONFIG_TUSB422_PAL
	// << Call platform functions to configure mux here >>
#ifdef CONFIG_CONTEXTHUB_PD
	if (!support_dp) {
		return;
	}

	g_polarity = polarity;
	switch (ctrl)
	{
		case MUX_DISABLE:
			return;
		case MUX_USB:
			return;
		case MUX_DP_2LANE:
			g_mux_type = TCPC_USB31_AND_DP_2LINE;
			break;
		case MUX_DP_4LANE:
			g_mux_type = TCPC_DP;
			break;
		default:
			break;
	}

	/* add aux switch */
	dp_aux_switch_op(polarity);
	/* add aux uart switch*/
	dp_aux_uart_switch_enable();

	struct pd_dpm_combphy_event event;
	event.dev_type = TCA_ID_RISE_EVENT;
	event.irq_type = TCA_IRQ_HPD_OUT;
	event.mode_type = TCPC_NC;
	event.typec_orien = polarity;

	pd_dpm_handle_combphy_event(event);
	pd_dpm_set_combphy_status(g_mux_type);

	event.dev_type = TCA_ID_FALL_EVENT;
	event.irq_type = TCA_IRQ_HPD_IN;
	event.mode_type = g_mux_type;
	pd_dpm_handle_combphy_event(event);

	pr_info("\nhuawei_pd %s pd_event_notify , mux_type = %d\n", __func__, g_mux_type);

#endif
#elif defined CONFIG_TUSB422 /* For BeagleBone Black EVM platform */
#else /* For MSP430/432 platform */

	// 0 = UNFLIPPED, 1 = FLIPPED.
	tcpm_hal_set_mux_polarity(polarity);

	switch (ctrl)
	{
		case MUX_DISABLE:
			CRIT("Mux: Disable\n");
			// Power down mux.
			if (data_role == PD_DATA_ROLE_UFP)
			{
				// For TUSB460 Sink Board.
				tcpm_hal_set_mux_enable(0);
			}
			else /* DFP */
			{
				// For TUSB1046.  AM_SEL = CTL0, EN = CTL1.
				tcpm_hal_set_mux_am_sel(0);
				tcpm_hal_set_mux_enable(0);
			}
			break;

		case MUX_USB:
			CRIT("Mux: USB\n");
			if (data_role == PD_DATA_ROLE_UFP)
			{
			}
			else /* DFP */
			{
				// For TUSB1046. One Port USB3.1.
				tcpm_hal_set_mux_am_sel(1);
				tcpm_hal_set_mux_enable(0);
			}
			break;

		case MUX_DP_2LANE:
			CRIT("Mux: DP 2-lane\n");
			if (data_role == PD_DATA_ROLE_UFP)
			{
			}
			else /* DFP */
			{
				// For TUSB1046.  One Port USB 3.1 + 2 Lane DP.
				tcpm_hal_set_mux_am_sel(1);
				tcpm_hal_set_mux_enable(1);
			}
			break;

		case MUX_DP_4LANE:
			CRIT("Mux: DP 4-lane\n");
			if (data_role == PD_DATA_ROLE_UFP)
			{
				// For TUSB460 Sink Board.
				tcpm_hal_set_mux_am_sel(1);
				tcpm_hal_set_mux_enable(1);
			}
			else /* DFP */
			{
				// For TUSB1046. 4 Lane DP.
				tcpm_hal_set_mux_am_sel(0);
				tcpm_hal_set_mux_enable(1);
			}
			break;

		case MUX_AUDIO:
			CRIT("Mux: Audio\n");
			break;

		default:
			break;
	}

#endif

	return;
}

void tcpm_hpd_out_control(uint8_t port, hpd_state_t state)
{
	DEBUG("HPD_OUT: %u\n", val);
#ifdef CONFIG_TUSB422_PAL
	// << Call platform functions to send HPD to DisplayPort source here >>
#ifdef CONFIG_CONTEXTHUB_PD
	if (!support_dp) {
		return;
	}
	struct pd_dpm_combphy_event event;
	event.mode_type = g_mux_type;
	event.typec_orien = g_polarity;

	if (state == HPD_LOW)
	{
		event.dev_type = TCA_DP_OUT;
		event.irq_type = TCA_IRQ_HPD_OUT;
		pd_dpm_handle_combphy_event(event);
		pd_dpm_set_last_hpd_status(false);
		pd_dpm_send_event(DP_CABLE_OUT_EVENT);
	}
	else if (state == HPD_HIGH)
	{
		event.dev_type = TCA_DP_IN;
		event.irq_type = TCA_IRQ_HPD_IN;
		pd_dpm_handle_combphy_event(event);
		pd_dpm_set_last_hpd_status(true);
		pd_dpm_send_event(DP_CABLE_IN_EVENT);
	}
	else /* HPD_IRQ */
	{
		event.dev_type = TCA_DP_IN;
		event.irq_type = TCA_IRQ_SHORT;
		pd_dpm_handle_combphy_event(event);
	}

#endif

#elif defined CONFIG_TUSB422 /* For BeagleBone Black EVM platform */
#else /* For MSP430/432 platform */
	if (state == HPD_IRQ)
	{
		tcpm_hal_hpd_out_control(port, 0);
		tcpm_msleep(1);
		tcpm_hal_hpd_out_control(port, 1);
	}
	else
	{
		tcpm_hal_hpd_out_control(port, (state == HPD_LOW) ? 0 : 1);
	}
#endif
	return;
}

uint8_t tcpm_get_hpd_in(uint8_t port)
{
	uint8_t hpd = 0;

#ifdef CONFIG_TUSB422_PAL
	// << Call platform functions to get HPD status from DisplayPort sink here >>
#elif defined CONFIG_TUSB422 /* For BeagleBone Black EVM platform */
#else
	hpd = tcpm_hal_get_hpd_in(port);
#endif

	return hpd;
}
