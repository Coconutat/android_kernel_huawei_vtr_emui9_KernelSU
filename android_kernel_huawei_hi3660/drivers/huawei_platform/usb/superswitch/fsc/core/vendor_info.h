/*
 * File:   vendor_info.h
 * Author: W0017688
 *
 * Created on June 2, 2016, 11:17 AM
 */

#ifndef VENDOR_INFO_H
#define	VENDOR_INFO_H

#ifdef	__cplusplus
extern "C" {
#endif

/** Helpers **/
#define YES 1
#define NO 0
#define SWAP(X) ((X) ? 0 : 1)
/*************/

/******************************************************************************/
/************************* Vendor Info ****************************************/
/******************************************************************************/

/* Leave any fields that do not apply as their defaults */
/* All fields marked as unimplemented are supported by device, but are not
 part of the example code. Features can be implemented on request. */


/* Intro Tab */
#define $VIF_Specification "Revision 1.00, Version 1.0"
#define $VIF_Producer "USB-IF Vendor Info File Generator, Version 1.1.0.2"
#define UUT_Device_Type 4        // DRP
#define $Vendor_Name "ON Semiconductor"
#define $Product_Name "FUSB302"
#define $Version_Info "A_REVA"
#define $TID "0"
#define $Port_Label "0"

/* General PD Settings Tab */
#define PD_Specification_Revision 1         //; Revision 2.0 /* Must be 1 */
#define SOP_Capable YES                     /* Always YES */
#define SOP_P_Capable YES
#define SOP_PP_Capable NO
#define SOP_P_Debug_Capable NO              /* Not Currently Implemented */
#define SOP_PP_Debug_Capable NO             /* Not Currently Implemented */
#define USB_Comms_Capable NO
#define DR_Swap_To_DFP_Supported YES
#define DR_Swap_To_UFP_Supported YES
#define Unconstrained_Power YES             //NOTE: This was previously called Externally_Powered
#define VCONN_Swap_To_On_Supported YES
#define VCONN_Swap_To_Off_Supported YES
#define Responds_To_Discov_SOP YES
#define Attempts_Discov_SOP YES


/* USB Type-C Tab */
#define Type_C_State_Machine 2            //; 2: DRP
#define Type_C_Supports_Vconn_Powered_Accessory YES
#define Type_C_Implements_Try_SRC NO
#define Type_C_Implements_Try_SNK YES
#define RP_Value 0                      //; Default
#define Type_C_Is_VCONN_Powered_Accessory NO    /* 'YES' Not Currently Implemented */
#define Type_C_Is_Debug_Target_SRC YES //TODO
#define Type_C_Is_Debug_Target_SNK YES //TODO
#define Type_C_Can_Act_As_Host NO               /* Not Controlled by this driver */
#define Type_C_Host_Speed 0                     /* Not Controlled by this driver */
#define Type_C_Can_Act_As_Device NO             /* Not Controlled by this driver */
#define Type_C_Device_Speed 0                   /* Not Controlled by this driver */
#define Type_C_Is_Alt_Mode_Controller NO        /* Not Controlled by this driver */
#define Type_C_Is_Alt_Mode_Device NO            /* Not Controlled by this driver */
#define Type_C_Power_Source 0                   /* Not Controlled by this driver */
#define Type_C_BC_1_2_Support 0                 /* Not Controlled by this driver */
#define Type_C_Battery_Powered NO               /* Not Controlled by this driver */
#define Type_C_Port_On_Hub NO                   /* Not Controlled by this driver */
#define Type_C_Supports_Audio_Accessory YES
#define Captive_Cable NO
#define Type_C_Sources_VCONN YES


/* Source Tab */
#define PD_Power_as_Source 500
#define USB_Suspend_May_Be_Cleared YES
#define Sends_Pings NO                      /* Not Currently Implemented */
#define Num_Src_PDOs 1 //TODO: 1+

#define Src_PDO_Supply_Type1 0             //; 0: Fixed
#define Src_PDO_Peak_Current1 0            //; 0: 100% IOC
#define Src_PDO_Voltage1 100
#define Src_PDO_Max_Current1 10
#define Src_PDO_Min_Voltage1 0             //; 0 V
#define Src_PDO_Max_Voltage1 0             //; 0 V
#define Src_PDO_Max_Power1 0             //; 0 W

#define Src_PDO_Supply_Type2 0             //; 0: Fixed
#define Src_PDO_Peak_Current2 0            //; 0: 100% IOC
#define Src_PDO_Voltage2 0
#define Src_PDO_Max_Current2 0
#define Src_PDO_Min_Voltage2 0             //; 0 V
#define Src_PDO_Max_Voltage2 0             //; 0 V
#define Src_PDO_Max_Power2 0             //; 0 W

#define Src_PDO_Supply_Type3 0             //; 0: Fixed
#define Src_PDO_Peak_Current3 0            //; 0: 100% IOC
#define Src_PDO_Voltage3 0
#define Src_PDO_Max_Current3 0
#define Src_PDO_Min_Voltage3 0             //; 0 V
#define Src_PDO_Max_Voltage3 0             //; 0 V
#define Src_PDO_Max_Power3 0             //; 0 W

#define Src_PDO_Supply_Type4 0             //; 0: Fixed
#define Src_PDO_Peak_Current4 0            //; 0: 100% IOC
#define Src_PDO_Voltage4 0
#define Src_PDO_Max_Current4 0
#define Src_PDO_Min_Voltage4 0             //; 0 V
#define Src_PDO_Max_Voltage4 0             //; 0 V
#define Src_PDO_Max_Power4 0             //; 0 W

#define Src_PDO_Supply_Type5 0             //; 0: Fixed
#define Src_PDO_Peak_Current5 0            //; 0: 100% IOC
#define Src_PDO_Voltage5 0
#define Src_PDO_Max_Current5 0
#define Src_PDO_Min_Voltage5 0             //; 0 V
#define Src_PDO_Max_Voltage5 0             //; 0 V
#define Src_PDO_Max_Power5 0             //; 0 W

#define Src_PDO_Supply_Type6 0             //; 0: Fixed
#define Src_PDO_Peak_Current6 0            //; 0: 100% IOC
#define Src_PDO_Voltage6 0
#define Src_PDO_Max_Current6 0
#define Src_PDO_Min_Voltage6 0             //; 0 V
#define Src_PDO_Max_Voltage6 0             //; 0 V
#define Src_PDO_Max_Power6 0             //; 0 W

#define Src_PDO_Supply_Type7 0             //; 0: Fixed
#define Src_PDO_Peak_Current7 0            //; 0: 100% IOC
#define Src_PDO_Voltage7 0
#define Src_PDO_Max_Current7 0
#define Src_PDO_Min_Voltage7 0             //; 0 V
#define Src_PDO_Max_Voltage7 0             //; 0 V
#define Src_PDO_Max_Power7 0             //; 0 W

/* Sink Tab */
#define PD_Power_as_Sink 900
#define No_USB_Suspend_May_Be_Set NO
#define GiveBack_May_Be_Set NO
#define Higher_Capability_Set NO
#define Num_Snk_PDOs 1 //TODO: 1+

#define Snk_PDO_Supply_Type1 0          //; 0: Fixed
#define Snk_PDO_Voltage1 100
#define Snk_PDO_Op_Current1 10
#define Snk_PDO_Min_Voltage1 0             //; 0 V
#define Snk_PDO_Max_Voltage1 0             //; 0 V
#define Snk_PDO_Op_Power1 0             //; 0 W

#define Snk_PDO_Supply_Type2 0            //; 0: Fixed
#define Snk_PDO_Voltage2 180
#define Snk_PDO_Op_Current2 10
#define Snk_PDO_Min_Voltage2 0             //; 0 V
#define Snk_PDO_Max_Voltage2 0             //; 0 V
#define Snk_PDO_Op_Power2 0             //; 0 W

#define Snk_PDO_Supply_Type3 0            //; 0: Fixed
#define Snk_PDO_Voltage3 0
#define Snk_PDO_Op_Current3 0
#define Snk_PDO_Min_Voltage3 0             //; 0 V
#define Snk_PDO_Max_Voltage3 0             //; 0 V
#define Snk_PDO_Op_Power3 0             //; 0 W

#define Snk_PDO_Supply_Type4 0            //; 0: Fixed
#define Snk_PDO_Voltage4 0
#define Snk_PDO_Op_Current4 0
#define Snk_PDO_Min_Voltage4 0             //; 0 V
#define Snk_PDO_Max_Voltage4 0             //; 0 V
#define Snk_PDO_Op_Power4 0             //; 0 W

#define Snk_PDO_Supply_Type5 0            //; 0: Fixed
#define Snk_PDO_Voltage5 0
#define Snk_PDO_Op_Current5 0
#define Snk_PDO_Min_Voltage5 0             //; 0 V
#define Snk_PDO_Max_Voltage5 0             //; 0 V
#define Snk_PDO_Op_Power5 0             //; 0 W

#define Snk_PDO_Supply_Type6 0            //; 0: Fixed
#define Snk_PDO_Voltage6 0
#define Snk_PDO_Op_Current6 0
#define Snk_PDO_Min_Voltage6 0             //; 0 V
#define Snk_PDO_Max_Voltage6 0             //; 0 V
#define Snk_PDO_Op_Power6 0             //; 0 W

#define Snk_PDO_Supply_Type7 0            //; 0: Fixed
#define Snk_PDO_Voltage7 0
#define Snk_PDO_Op_Current7 0
#define Snk_PDO_Min_Voltage7 0             //; 0 V
#define Snk_PDO_Max_Voltage7 0             //; 0 V
#define Snk_PDO_Op_Power7 0             //; 0 W

/* Dual Role Tab */
#define Accepts_PR_Swap_As_Src YES
#define Accepts_PR_Swap_As_Snk YES
#define Requests_PR_Swap_As_Src NO
#define Requests_PR_Swap_As_Snk NO

/* SOP Discovery - Part One Tab */
#define Structured_VDM_Version_SOP 0            //; 0: V1.0
#define XID_SOP 0
#define Data_Capable_as_USB_Host_SOP YES
#define Data_Capable_as_USB_Device_SOP YES
#define Product_Type_SOP 2            //; 0: Undefined, 2: Peripheral, 5: AMA
#define Modal_Operation_Supported_SOP YES
#define USB_VID_SOP 0x12D1
#define PID_SOP 0x107E
#define bcdDevice_SOP 0x0000

/* SOP Discovery - Part Two Tab */
#define SVID_fixed_SOP YES
#define Num_SVIDs_min_SOP 1
#define Num_SVIDs_max_SOP 1                 /* Currently Implements Up To 1 */

#define SVID1_SOP 0xFF01
#define SVID1_modes_fixed_SOP YES
#define SVID1_num_modes_min_SOP 1
#define SVID1_num_modes_max_SOP 1           /* Currently Implements Up To 1 */

#define SVID1_mode1_enter_SOP YES

/* AMA Tab */
#define AMA_HW_Vers 0x1
#define AMA_FW_Vers 0x1
#define AMA_VCONN_reqd NO
#define AMA_VCONN_power 0            //; 0: 1W
#define AMA_VBUS_reqd NO
#define AMA_Superspeed_Support 1            //; 0: USB 2.0 only

/******************************************************************************/

#ifdef	__cplusplus
}
#endif

#endif	/* VENDOR_INFO_H */
