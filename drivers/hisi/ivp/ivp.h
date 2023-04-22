#ifndef _IVP_H_
#define _IVP_H_
//This is used to compat  mmap for Android 32Bit to Kernel 64Bit.
//Must compat with IVP DTS

#define SIZE_1MB                      ( 1 * 1024 * 1024)
#define SIZE_2MB                      ( 2 * 1024 * 1024)
#define MASK_1MB                      ( SIZE_1MB - 1)
#define SIZE_4K                       ( 4 * 1024 )
#define IVP_MMAP_SHIFT                (4)

#define IVP_MODULE_NAME               "hisi-ivp"
#define IVP_REGULATOR                 "hisi-ivp"
#define IVP_MEDIA_REGULATOR           "hisi-ivp-media"

#define IVP_CLK_LEVEL_DEFAULT         0
#define IVP_CLK_LEVEL_ULTRA_LOW       4 //set ultra_low value 4 tempo, will change all macros value with camera3 together
#define IVP_CLK_LEVEL_LOW             1
#define IVP_CLK_LEVEL_MEDIUM          2
#define IVP_CLK_LEVEL_HIGH            3

//dts info
#define OF_IVP_SECTION_NAME           "section_mem"
#define OF_IVP_SECTION_NODE_NAME      "ivp_mem_section"
#define OF_IVP_DDR_MEM_NAME           "ddr_mem"
#define OF_IVP_SHARE_MEM_NAME         "share_mem"
#define OF_IVP_LOG_MEM_NAME           "log_mem"

#define OF_IVP_CLK_NAME                  "hisi-ivp-clk"
#define OF_IVP_CLK_RATE_NAME             "ivp-clk-rate"
#define OF_IVP_MIDDLE_CLK_RATE_NAME      "ivp-middle-clk-rate"
#define OF_IVP_LOW_CLK_RATE_NAME         "ivp-low-clk-rate"
#define OF_IVP_ULTRA_LOW_CLK_RATE_NAME   "ivp-ultra-low-clk-rate"
#define OF_IVP_LOW_CLK_PU_RATE_NAME      "ivp-lowfrq-pu-clk-rate"
#define OF_IVP_LOW_TEMP_RATE_NAME        "ivp-lowtemp-clk-rate"
#define OF_IVP_LOWFREQ_CLK_RATE_NAME     "lowfrq-pd-clk-rate"

#define OF_IVP_DYNAMIC_MEM               "ivp-dynamic-mem"
#define OF_IVP_DYNAMIC_MEM_SEC_SIZE      "ivp-dynamic-mem-section-size"
#define OF_IVP_SEC_SUPPORT               "ivp-sec-support-flag"

#define IVP_IOCTL_SECTCOUNT           _IOR('v', 0x70, unsigned int)
#define IVP_IOCTL_SECTINFO            _IOWR('v', 0x71, struct ivp_sect_info)
#define IVP_IOCTL_DSP_RUN             _IOW('v', 0x72, unsigned int)
#define IVP_IOCTL_DSP_SUSPEND         _IOW('v', 0x73, unsigned int)
#define IVP_IOCTL_DSP_RESUME          _IOW('v', 0x74, unsigned int)
#define IVP_IOCTL_DSP_STOP            _IOW('v', 0x75, unsigned int)
#define IVP_IOCTL_QUERY_RUNSTALL      _IOR('v', 0x76, unsigned int)
#define IVP_IOCTL_QUERY_WAITI         _IOR('v', 0x77, unsigned int)
#define IVP_IOCTL_TRIGGER_NMI         _IOW('v', 0x78, unsigned int)
#define IVP_IOCTL_WATCHDOG            _IOR('v', 0x79, unsigned int)
#define IVP_IOCTL_WATCHDOG_SLEEP      _IOR('v', 0x7a, unsigned int)

#define IVP_IOCTL_SMMU_INVALIDATE_TLB _IOW('v', 0x7b, unsigned int)
#define IVP_IOCTL_BM_INIT             _IOW('v', 0x7c, unsigned int)
#define IVP_IOCTL_CLK_LEVEL           _IOW('v', 0x7d, unsigned int)
#define IVP_IOCTL_POWER_UP            _IOW('v', 0x7e, unsigned int)


#define IVP_IOCTL_IPC_FLUSH_ENABLE    _IOWR('v', 0x89, unsigned int)
#define IVP_IOCTL_LOAD_FIRMWARE       _IOW('v',  0x8A, struct ivp_image_info)

enum SEC_MODE {
    NOSEC_MODE = 0,
    SECURE_MODE = 1
};

struct ivp_sect_info {
    char name[64];
    unsigned int index;
    unsigned int len;
    unsigned int ivp_addr;
    unsigned int reserved;
    union {
        unsigned long acpu_addr;
        char compat32[8];
    };
};
struct ivp_image_info {
    char name[64];
    unsigned int length;
};
#endif /* IVP_H_ */
