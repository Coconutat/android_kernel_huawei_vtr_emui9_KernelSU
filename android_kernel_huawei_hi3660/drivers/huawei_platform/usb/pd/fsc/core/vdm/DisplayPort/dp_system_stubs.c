#ifdef FSC_HAVE_DP

#include "../../platform.h"
#include "interface_dp.h"

#include <linux/hisi/contexthub/tca.h>
#include "../../core.h"
#include <huawei_platform/usb/hw_pd_dev.h>


static TCPC_MUX_CTRL_TYPE g_mux_type = TCPC_DP;

#ifdef CONFIG_CONTEXTHUB_PD
extern int support_dp;
extern void dp_aux_switch_op(uint32_t value);
extern void dp_aux_uart_switch_enable(void);
extern int pd_dpm_handle_combphy_event(struct pd_dpm_combphy_event event);
extern void pd_dpm_set_last_hpd_status(bool hpd_status);
extern void pd_dpm_send_event(enum pd_dpm_cable_event_type event);
#endif


#define MODE_DP_PIN_C 0x04
#define MODE_DP_PIN_D 0x08
#define MODE_DP_PIN_E 0x10
#define MODE_DP_PIN_F 0x20


void informStatus(DisplayPortStatus_t stat) //response  for DP status update request, refer tcpci_report_hpd_state in tcpi.h
{
	/* TODO: 'system' should implement this */
	/* this function is called to inform the 'system' of the DP status of the port partner */
    int ret = 0;
#ifdef CONFIG_CONTEXTHUB_PD
    struct pd_dpm_combphy_event event = {0};
#endif
    DpPpStatus.word = stat.word;

#ifdef CONFIG_CONTEXTHUB_PD
    if (!support_dp) {
        return;
    }

    if(DpPpConfig.word != 0){
        event.dev_type = TCA_DP_IN;
        event.irq_type = TCA_IRQ_HPD_IN;
        event.mode_type = g_mux_type;
        event.typec_orien = core_get_cc_orientation();

        pr_info("\n%s + state = %d,irq = %d\n", __func__,stat.HpdState ,stat.IrqHpd);

        if (!stat.HpdState) {
            event.dev_type = TCA_DP_OUT;
            event.irq_type = TCA_IRQ_HPD_OUT;
            ret = pd_dpm_handle_combphy_event(event);
            pd_dpm_set_last_hpd_status(false);
            pd_dpm_send_event(DP_CABLE_OUT_EVENT);
        }
        else
        {
            event.dev_type = TCA_DP_IN;
            ret = pd_dpm_handle_combphy_event(event);
            pd_dpm_set_last_hpd_status(true);
            pd_dpm_send_event(DP_CABLE_IN_EVENT);
        }

        if (stat.IrqHpd) {
            event.irq_type = TCA_IRQ_SHORT;
            ret = pd_dpm_handle_combphy_event(event);
        }

        pr_info("\n%s - ret = %d\n", __func__, ret);
    }
#endif

        pr_info("\n %s,%d\n",__func__, __LINE__);
}

void informConfigResult (FSC_BOOL success)
{
    /* TODO: 'system' should implement this */
    /* this function is called when a config message is either ACKd or NAKd by the other side */
#ifdef CONFIG_CONTEXTHUB_PD
    FSC_U8 fsc_polarity = 0;
    FSC_U32 pin_assignment = 0;
    int ret  = 0;
#endif

    if (success == FALSE){
        pr_info("\n %s,%d\n",__func__, __LINE__);
        return;
    }
    else
    {
        pr_info("\n %s,%d\n",__func__, __LINE__);
        DpPpConfig.word = DpPpRequestedConfig.word;
#ifdef CONFIG_CONTEXTHUB_PD
        /* add aux switch */
        if (!support_dp) {
            return;
        }
        fsc_polarity = core_get_cc_orientation();
        dp_aux_switch_op(fsc_polarity);
        /* add aux uart switch*/
        dp_aux_uart_switch_enable();

        switch (DpPpRequestedConfig.Conf) {
        case DP_CONF_UFP_D:
            pin_assignment = DpPpRequestedConfig.DfpPa & 0xff;
            break;
        case DP_CONF_DFP_D:
            pin_assignment = DpPpRequestedConfig.UfpPa & 0xff;
            break;
        }

        if(MODE_DP_PIN_C == pin_assignment || MODE_DP_PIN_E == pin_assignment){
            g_mux_type = TCPC_DP;
        }
        else if(MODE_DP_PIN_D == pin_assignment || MODE_DP_PIN_F == pin_assignment){
            g_mux_type = TCPC_USB31_AND_DP_2LINE;
        }

        struct pd_dpm_combphy_event event;
        event.dev_type = TCA_ID_RISE_EVENT;
        event.irq_type = TCA_IRQ_HPD_OUT;
        event.mode_type = TCPC_NC;
        event.typec_orien = core_get_cc_orientation();

        ret = pd_dpm_handle_combphy_event(event);
        pd_dpm_set_combphy_status(g_mux_type);

        event.dev_type = TCA_ID_FALL_EVENT;
        event.irq_type = TCA_IRQ_HPD_IN;
        event.mode_type = g_mux_type;
        event.typec_orien = core_get_cc_orientation();
        ret = pd_dpm_handle_combphy_event(event);

        if(DpPpStatus.HpdState){
            informStatus(DpPpStatus);
        }

        pr_info("\nhuawei_pd %s pd_event_notify ret = %d, mux_type = %d\n", __func__, ret, g_mux_type);
#endif
    }
}

void updateStatusData(void)
{
	/* TODO: 'system' should implement this */
	/* this function is called to get an update of our status - to be sent to the port partner */
}

FSC_BOOL DpReconfigure(DisplayPortConfig_t config)
{
    /* TODO: 'system' should implement this */
    /* called with a DisplayPort configuration to do! */
    /* return TRUE if/when successful, FALSE otherwise */
    DpConfig.word = config.word;
    /* must actually change configurations here before returning TRUE! */
    return TRUE;
}

#endif // FSC_HAVE_DP
