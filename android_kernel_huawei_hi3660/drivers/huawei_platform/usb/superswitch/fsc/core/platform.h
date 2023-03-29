/* platform.h
 *
 * THIS FILE DEFINES EVERY PLATFORM-DEPENDENT ELEMENT THAT THE CORE REQUIRES.
 * 
 * INSTRUCTIONS FOR THIS FILE:
 * 1. Modify this file with a definition of Generic Type Definitions
 * (FSC_S8, FSC_U32, etc) Either by include or putting directly in this file.
 * 2. Include this as a header file for your platform.c and implement the 
 * function headers as defined below.
 * 
 * It is the driver-writer's responsibility to implement each function
 * stub and to allocate/initialize/reserve sufficient system resources.
 * 
 */
#ifndef _FSC_PLATFORM_H_
#define _FSC_PLATFORM_H_

/* PLATFORM_ARM
 *
 * This platform is for the ARM M0.
 */
#ifdef PLATFORM_ARM
#include "../Platform_ARM/FSCTypes.h"

#define kMSTimeFactor   1000  /* ARM platform timer set for 1us */
#endif // PLATFORM_ARM

/* FSC_PLATFORM_LINUX
 * 
 * This platform is for the Linux kernel driver.
 */
#ifdef FSC_PLATFORM_LINUX
#include "../Platform_Linux/FSCTypes.h"

#define kMSTimeFactor   1     /* Linux platform converts sched_clock to ms */
#endif // FSC_PLATFORM_LINUX

/*******************************************************************************
 * Function:        platform_log
 * Input:           Port ID - 0 if one port system
 *                  Message string - null terminated.
 *                  Optional value to append to the log message
 * Return:          N/A
 * Description:     Platform specific debug log method.
 ******************************************************************************/
void FUSB3601_platform_log(FSC_U8 port, const char *str, FSC_S32 value);

/*******************************************************************************
 * Function:        platform_get_device_irq_state
 * Input:           Port ID - 0 if one port system
 * Return:          Boolean.  TRUE = Interrupt Active
 * Description:     Get the state of the INT_N pin.  INT_N is active low.  This
 *                  function handles that by returning TRUE if the pin is
 *                  pulled low indicating an active interrupt signal.
 ******************************************************************************/
FSC_BOOL FUSB3601_platform_get_device_irq_state(FSC_U8 port);
 
/* Temporary GPIO Src Vbus control */
void FUSB3601_platform_set_vbus_output(FSC_BOOL enable);
void FUSB3601_platform_set_debug_output(FSC_BOOL enable);

/*******************************************************************************
 * Function:        platform_i2c_write
 * Input:           SlaveAddress - Slave device bus address
 *                  RegisterAddress - Internal register address
 *                  DataLength - Length of data to transmit
 *                  Data - Buffer of char data to transmit
 * Return:          Error state
 * Description:     Write a char buffer to the I2C peripheral.
 ******************************************************************************/
FSC_BOOL FUSB3601_platform_i2c_write(FSC_U8 SlaveAddress,
                            FSC_U8 RegisterAddress,
                            FSC_U8 DataLength,
                            FSC_U8* Data);

/*******************************************************************************
 * Function:        platform_i2c_read
 * Input:           SlaveAddress - Slave device bus address
 *                  RegisterAddress - Internal register address
 *                  DataLength - Length of data to attempt to read
 *                  Data - Buffer for received char data
 * Return:          Error state.
 * Description:     Read char data from the I2C peripheral.
 ******************************************************************************/
FSC_BOOL FUSB3601_platform_i2c_read( FSC_U8 SlaveAddress,
                            FSC_U8 RegisterAddress,
                            FSC_U8 DataLength,
                            FSC_U8* Data);

/*****************************************************************************
* Function:        platform_enable_timer
* Input:           enable - TRUE to enable platform timer, FALSE to disable
* Return:          None
* Description:     Enables or disables platform timer
******************************************************************************/
void FUSB3601_platform_enable_timer(FSC_BOOL enable);

/******************************************************************************
 * Function:        platform_delay
 * Input:           delayCount - Number of microseconds to wait
 * Return:          None
 * Description:     Perform a blocking software delay in intervals of 1us
 ******************************************************************************/
void FUSB3601_platform_delay(FSC_U32 microseconds);

/******************************************************************************
 * Function:        platform_current_time
 * Input:           None
 * Return:          Current system time value in microseconds
 * Description:     Provide a running system clock for timer implementations
 ******************************************************************************/
FSC_U32 FUSB3601_platform_current_time(void);

/******************************************************************************
 * Function:        platform_timestamp
 * Input:           None
 * Return:          Encoded timestamp: 0xSSSSMMMM
 * Description:     Provide a timestamp encoded into 32 bits.
 *                  MSB's are seconds, LSB's are 10ths of milliseconds.
 ******************************************************************************/
FSC_U32 FUSB3601_platform_timestamp(void);

/******************************************************************************
 * Function:        platform_set_vconn
 * Input:           enable - True/False to control VCONN boost
 * Return:          None
 * Description:     Provide an interface to enable VCONN boost
 ******************************************************************************/
void FUSB3601_platform_set_vconn(FSC_BOOL enable);

#endif  // _FSC_PLATFORM_H_

