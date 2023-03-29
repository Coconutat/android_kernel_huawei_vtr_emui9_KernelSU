

#ifndef __OAL_PCIE_REG_H__
#define __OAL_PCIE_REG_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*PCIe Config Register*/
#define  HI_PCI_IATU_VIEWPORT_OFF                        (0x900)
#define  HI_PCI_IATU_OUTBOUND      (0)
#define  HI_PCI_IATU_INBOUND       (1)

#define HI_PCI_IATU_BOUND_BASE_OFF      (0x904) /*4.70a*/

#define HI_PCI_IATU_INBOUND_BASE_OFF(i)  (0x100+((i)*0x200)) /*5.00a*/
#define HI_PCI_IATU_OUTBOUND_BASE_OFF(i) (0x000+((i)*0x200))

#define  HI_PCI_IATU_REGION_CTRL_1_OFF_OUTBOUND_I(base)        (0x0+(base))
#define  HI_PCI_IATU_REGION_CTRL_1_OFF_INBOUND_I(base)         (0x0+(base))/*same as outbound, use viewport select*/
#define  HI_PCI_IATU_REGION_CTRL_2_OFF_INBOUND_I(base)         (0x4+(base))
#define  HI_PCI_IATU_REGION_CTRL_2_OFF_OUTBOUND_I(base)        (0x4+(base))
#define  HI_PCI_IATU_LWR_BASE_ADDR_OFF_INBOUND_I(base)         (0x8+(base))
#define  HI_PCI_IATU_LWR_BASE_ADDR_OFF_OUTBOUND_I(base)        (0x8+(base))
#define  HI_PCI_IATU_UPPER_BASE_ADDR_OFF_OUTBOUND_I(base)      (0xc+(base))
#define  HI_PCI_IATU_UPPER_BASE_ADDR_OFF_INBOUND_I(base)       (0xc+(base))
#define  HI_PCI_IATU_LIMIT_ADDR_OFF_OUTBOUND_I(base)           (0x10+(base))
#define  HI_PCI_IATU_LIMIT_ADDR_OFF_INBOUND_I(base)            (0x10+(base))
#define  HI_PCI_IATU_LWR_TARGET_ADDR_OFF_OUTBOUND_I(base)      (0x14+(base))
#define  HI_PCI_IATU_LWR_TARGET_ADDR_OFF_INBOUND_I(base)       (0x14+(base))
#define  HI_PCI_IATU_UPPER_TARGET_ADDR_OFF_OUTBOUND_I(base)    (0x18+(base))
#define  HI_PCI_IATU_UPPER_TARGET_ADDR_OFF_INBOUND_I(base)     (0x18+(base))
#define  HI_PCI_IATU_REGION_CTRL_3_OFF_OUTBOUND_I(base)        (0x1c+(base))
#define  HI_PCI_IATU_REGION_CTRL_3_OFF_INBOUND_I(base)         (0x1c+(base))
#define  HI_PCI_IATU_UPPR_LIMIT_ADDR_OFF_INBOUND_I(base)       (0x20+(base))
#define  HI_PCI_IATU_IATU_UPPR_LIMIT_ADDR_OFF_OUTBOUND_I(base) (0x20+(base))

typedef union { 
    struct {
        //LSB
        oal_uint32      region_index          :31;              // 0:30
        oal_uint32      region_dir            :1;               // 31
        //MSB
    }bits;
    oal_uint32 AsDword;
} IATU_VIEWPORT_OFF;

typedef union { 
    struct {
        //LSB
        oal_uint32  type :5; //0:4
        oal_uint32  tc   :3; //5:7
        oal_uint32  td   :1; //8
        oal_uint32  attr :2; //9:10
        oal_uint32  ido  :1; //11
        oal_uint32  th   :1; //12
        oal_uint32  inc_region_size  :1; //13
        oal_uint32  reserved14 :2;  //14:15
        oal_uint32  at    :2;   //16:17
        oal_uint32  ph    :2;   //18:19
        oal_uint32  func_num :3; //20:22
        oal_uint32  reserved23 :9; //23:31
        //MSB
    }bits;
    oal_uint32 AsDword;
} IATU_REGION_CTRL_1_OFF;

typedef union { 
    struct {
        //LSB
        oal_uint32  msg_code         :8; //0:7
        oal_uint32  bar_num          :3; //8:10
        oal_uint32  rsvdp11          :2; //11:12
        oal_uint32  msg_type_match_mode  :1; //13
        oal_uint32  tc_match_en          :1; //14
        oal_uint32  td_match_en          :1; //15
        oal_uint32  attr_match_en        :1; //16
        oal_uint32  th_match_en          :1; //17
        oal_uint32  at_match_en          :1; //18
        oal_uint32  func_num_match_en    :1; //19
        oal_uint32  vf_match_en          :1; //20
        oal_uint32  msg_code_match_en    :1; //21
        oal_uint32  ph_match_en          :1; //22
        oal_uint32  single_addr_loc_trans_en : 1; //23
        oal_uint32  response_code        :2;  //24:25
        oal_uint32  vfbar_match_mode_en  :1;  //26
        oal_uint32  fuzzy_type_match_code :1;  //27
        oal_uint32  cfg_shift_mode        :1;  //28
        oal_uint32  invert_mode           :1;  //29
        oal_uint32  match_mode            :1;  //30
        oal_uint32  region_en             :1;  //31
        //MSB
    }bits;
    oal_uint32 AsDword;
} IATU_REGION_CTRL_2_OFF;

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif

