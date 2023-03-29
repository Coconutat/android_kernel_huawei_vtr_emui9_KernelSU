#ifndef __FUSB_PLATFORM_HELPERS_H_
#define __FUSB_PLATFORM_HELPERS_H_
#include "FSCTypes.h"
#define INIT_DELAY_MS 500   /* Time to wait before init'ing the device (ms) */
#define RETRIES_I2C   3     /* Number of retries for I2C reads/writes */

int get_dpd_enable(void);
/*******************************************************************************
* Function:        fusb_InitializeGPIO
* Input:           none
* Return:          0 on success, error code otherwise
* Description:     Initializes the platform's GPIO
*******************************************************************************/
FSC_S32 FUSB3601_fusb_InitializeGPIO(void);

/*******************************************************************************
* Function:        fusb_GPIO_Set_VBus5v/Other
* Input:           set: true to set pin, false to clear
* Return:          none
* Description:     Sets or clears the VBus5V/Other gpio pin
*******************************************************************************/
void FUSB3601_fusb_GPIO_Set_VBus5v(FSC_BOOL set);
FSC_BOOL FUSB3601_fusb_GPIO_Get_VBus5v(void);
void FUSB3601_fusb_GPIO_Set_Vconn(FSC_BOOL set);
FSC_BOOL FUSB3601_fusb_GPIO_Get_Vconn(void);

void FUSB3601_fusb_GPIO_Set_VBusOther(FSC_BOOL set);
FSC_BOOL FUSB3601_fusb_GPIO_Get_VBusOther(void);

/*******************************************************************************
* Function:        fusb_GPIO_Get_VBus5v/Other/IntN
* Input:           none
* Return:          true if set, false if clear
* Description:     Returns the value of the GPIO pin
FSC_BOOL fusb_GPIO_Get_VBus5v(void);
FSC_BOOL fusb_GPIO_Get_VBusOther(void);
*******************************************************************************/
FSC_BOOL FUSB3601_fusb_GPIO_Get_IntN(void);

#ifdef FSC_DEBUG
/*******************************************************************************
* Function:        fusb_GPIO_Set_SM_Toggle
* Input:           set - true to set high, false to set low
* Return:          none
* Description:     Sets or clears the GPIO pin
*******************************************************************************/
void FUSB3601_dbg_fusb_GPIO_Set_SM_Toggle(FSC_BOOL set);

/*******************************************************************************
* Function:        fusb_GPIO_Get_SM_Toggle
* Input:           none
* Return:          true if set, false if clear
* Description:     Returns the value of the GPIO pin
*******************************************************************************/
FSC_BOOL FUSB3601_dbg_fusb_GPIO_Get_SM_Toggle(void);
#endif  /* FSC_DEBUG */

/*******************************************************************************
* Function:        fusb_GPIO_Cleanup
* Input:           none
* Return:          none
* Description:     Frees any GPIO resources we have claimed (eg. INT_N, VBus5V)
*******************************************************************************/
void FUSB3601_fusb_GPIO_Cleanup(void);

/*******************************************************************************
* Function:        fusb_I2C_WriteData
* Input:           address - Destination register for write data
*                  length - Number of data bytes to write
*                  data - Data to write to the register at address
* Return:          true on success, false otherwise
* Description:     Write an unsigned byte to the I2C peripheral.
*                  Blocking, with retries
*******************************************************************************/
FSC_BOOL FUSB3601_fusb_I2C_WriteData(FSC_U8 address, FSC_U8 length, FSC_U8* data);

/*******************************************************************************
* Function:        fusb_I2C_ReadData
* Input:           address - Source register for read data
*                  data - Output storage
* Return:          true on success, false otherwise
* Description:     Read an unsigned byte from the I2C peripheral.
*                  Blocking, with retries
*******************************************************************************/
FSC_BOOL FUSB3601_fusb_I2C_ReadData(FSC_U8 address, FSC_U8* data);

/*******************************************************************************
* Function:        fusb_I2C_ReadBlockData
* Input:           address - Source register for read data
*                  length - Number of sequential bytes to read
*                  data - Output buffer
* Return:          true on success, false otherwise
* Description:     Read a block of unsigned bytes from the I2C peripheral.
*******************************************************************************/
FSC_BOOL FUSB3601_fusb_I2C_ReadBlockData(FSC_U8 address, FSC_U8 length, FSC_U8* data);
int FUSB3601_fusb_i2c_write_mask(u8 reg, u8 MASK, u8 SHIFT, u8 value);
#ifdef FSC_DEBUG
/*******************************************************************************
* Function:        fusb_Sysfs_Init
* Input:           none
* Return:          none
* Description:     Initializes the sysfs objects (used for user-space access)
*******************************************************************************/
void FUSB3601_fusb_Sysfs_Init(void);
#endif /* FSC_DEBUG */

/*******************************************************************************
* Function:        fusb_InitializeCore
* Input:           none
* Return:          none
* Description:     Calls core initialization functions
*******************************************************************************/
void FUSB3601_fusb_InitializeCore(void);

/*******************************************************************************
* Function:        fusb_IsDeviceValid
* Input:           none
* Return:          true if device is valid, false otherwise
* Description:     Verifies device contents via I2C read
*******************************************************************************/
FSC_BOOL FUSB3601_fusb_IsDeviceValid(void);

/*******************************************************************************
* Function:        fusb_reset
* Input:           none
* Return:          none
* Description:     reset fusb3601 by assert SW_RES
*******************************************************************************/
FSC_BOOL FUSB3601_fusb_reset(void);
FSC_BOOL FUSB3601_fusb_reset_with_adc_reset(void);

/*******************************************************************************
* Function:        fusb_InitChipData
* Input:           none
* Return:          none
* Description:     Initializes our driver chip struct data
*******************************************************************************/
void FUSB3601_fusb_InitChipData(void);

/*******************************************************************************
* Function:        fusb_EnableInterrupts
* Input:           none
* Return:          0 on success, error code on failure
* Description:     Initializaes and enables the INT_N interrupt.
*                  NOTE: The interrupt may be triggered at any point once
*                  this is called.
*                  NOTE: The Int_N GPIO must be intialized prior to using
*                  this function.
*******************************************************************************/
FSC_S32 FUSB3601_fusb_EnableInterrupts(void);
void FUSB3601_ConfigurePortType(FSC_U8, struct Port *port);
void FUSB3601_core_set_try_snk(struct Port *port);
void FUSB3601_set_driver_shutdown_flag(int flag);
#ifdef CONFIG_DUAL_ROLE_USB_INTF
FSC_S32 FUSB3601_dual_role_phy_init(void);
#endif
#endif /* __FUSB_PLATFORM_HELPERS_H_ */

