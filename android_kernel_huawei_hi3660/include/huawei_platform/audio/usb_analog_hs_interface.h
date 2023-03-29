#ifndef __USB_ANALOG_HS_INTERFACE__
#define __USB_ANALOG_HS_INTERFACE__

/* codec operate used by anc_hs driver*/
struct usb_analog_hs_codec_ops {
    bool (*check_headset_in)(void *);
    void (*plug_in_detect)(void *);
    void (*plug_out_detect)(void *);
    int (*get_headset_type)(void *);
    void (*hs_high_resistence_enable)(void *, bool);
};

#define IOCTL_USB_ANA_HS_GET_MIC_SWITCH_STATE               _IOR('U', 0x01, __u32)
#define IOCTL_USB_ANA_HS_GND_FB_CONNECT                     _IO('U', 0x02)
#define IOCTL_USB_ANA_HS_GND_FB_DISCONNECT                  _IO('U', 0x03)
#define IOCTL_USB_ANA_HS_GET_CONNECT_LINEIN_R_STATE         _IOR('U', 0x04, __u32)

struct usb_analog_hs_dev {
    char *name;
    struct usb_analog_hs_codec_ops ops;
};

enum usb_analog_hs_plugin_state {
    USB_ANA_HS_PLUG_OUT = 0,
    USB_ANA_HS_PLUG_IN,
    /** for aux switch in display port **/
    DP_PLUG_IN,
    DP_PLUG_IN_CROSS,
    DP_PLUG_OUT,
    DIRECT_CHARGE_IN,
    DIRECT_CHARGE_OUT
};


enum usb_analog_hs_gpio_type {
    USB_ANALOG_HS_GPIO_SOC           = 0,
    USB_ANALOG_HS_GPIO_CODEC         = 1,
};

enum usb_analog_hs_register_status {
    USB_ANALOG_HS_NOT_REGISTER       = 0,
    USB_ANALOG_HS_ALREADY_REGISTER   = 1,
};

struct usb_analog_hs_ops {
    int (*usb_ana_hs_check_headset_pluged_in)(void);
    int (*usb_ana_hs_dev_register)(struct usb_analog_hs_dev *dev, void *codec_data);
    bool (*check_usb_ana_hs_support)(void);
    void (*usb_ana_hs_plug_in_out_handle)(int hs_state);
    void (*usb_ana_hs_mic_swtich_change_state)(void);
};

#ifdef CONFIG_USB_ANALOG_HS_INTERFACE
/**begin: these Functions is call by extern module: MBHC**/
extern int usb_analog_hs_check_headset_pluged_in(void);
extern int usb_analog_hs_dev_register(struct usb_analog_hs_dev *dev, void *codec_data);
extern bool check_usb_analog_hs_support(void);
extern void usb_analog_hs_plug_in_out_handle(int hs_state);
extern void usb_ana_hs_mic_swtich_change_state(void);
/**end: these Functions is call by extern module: MBHC**/

/*begin: these Functions is call by inner usb ana hs module*/
extern int usb_analog_hs_ops_register(struct usb_analog_hs_ops *ops);
/*end: these Functions is call by inner usb ana hs module*/

#else
static inline int usb_analog_hs_check_headset_pluged_in(void)
{
    return USB_ANA_HS_PLUG_OUT;
}

static inline int usb_analog_hs_dev_register(struct usb_analog_hs_dev *dev, void *codec_data)
{
    return 0;
}

static inline bool check_usb_analog_hs_support(void)
{
    return false;
}

static inline void usb_analog_hs_plug_in_out_handle(int hs_state)
{
    return;
}

static inline void usb_ana_hs_mic_swtich_change_state(void)
{
    return;
}

static inline int usb_analog_hs_ops_register(struct usb_analog_hs_ops *ops)
{
    return 0;
}

#endif

#endif //USB_ANALOG_HS_INTERFACE
