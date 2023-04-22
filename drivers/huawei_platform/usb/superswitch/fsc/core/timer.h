/*
 * timer.h
 *
 * Defines a timer class for time tracking against a system clock
 */
#ifndef FSCPM_TIMER_H_
#define FSCPM_TIMER_H_

#include "platform.h"

/* Constant timer values. Defined in milliseconds * a factor to align
 * the value with the system timer resolution.
 */
#define ktAMETimeout            (900 * kMSTimeFactor)
#define ktCCDebounce            (120 * kMSTimeFactor)
#define ktPDDebounce            (12 * kMSTimeFactor)
#define ktDRPTry                (125 * kMSTimeFactor)
#define ktDRPTryWait            (500 * kMSTimeFactor)
#define ktErrorRecovery         (50 * kMSTimeFactor)
#define ktDeviceToggle          (3 * kMSTimeFactor)
#define ktTOG2                  (30 * kMSTimeFactor)
#define ktIllegalCable          (150 * kMSTimeFactor)
#define ktLoopReset				(100 * kMSTimeFactor)
#define ktCableCheck			(50 * kMSTimeFactor)
#define ktSourceDetach          (8 * kMSTimeFactor)
#define ktVDMWait				(60 * kMSTimeFactor)
#define ktTxTimeout				(10 * kMSTimeFactor)

#define ktNoResponse            (5000 * kMSTimeFactor)
#define ktSenderResponse        (25 * kMSTimeFactor)
#define ktSenderResponseShort        (20 * kMSTimeFactor)
#define ktTypeCSendSourceCap    (150 * kMSTimeFactor)
#define ktSinkWaitCap           (500 * kMSTimeFactor)
#define ktTypeCSinkWaitCap      (500 * kMSTimeFactor)
#define ktSrcTransition         (30 * kMSTimeFactor)
#define ktHardResetComplete     (5 * kMSTimeFactor)
#define ktPSHardReset           (25 * kMSTimeFactor)
#define ktPSHardResetMax        (35 * kMSTimeFactor)
#define ktPSHardResetOverhead   (3 * kMSTimeFactor)
#define ktPSTransition          (500 * kMSTimeFactor)
#define ktPSSourceOff           (835 * kMSTimeFactor)
#define ktPSSourceOn            (435 * kMSTimeFactor)
#define ktVCONNSourceOn         (90 * kMSTimeFactor)
#define ktVCONNOnDelay          (10 * kMSTimeFactor)
#define ktBISTContMode          (50 * kMSTimeFactor)
#define ktSwapSourceStart       (25 * kMSTimeFactor)
#define ktSrcRecover            (730 * kMSTimeFactor)
#define ktSrcRecoverMax         (1000 * kMSTimeFactor)
#define ktGoodCRCDelay          (1 * kMSTimeFactor)
#define kt5To12VTransition      (8 * kMSTimeFactor)
#define ktSafe0V                (650 * kMSTimeFactor)
#define ktSrcTurnOn             (275 * kMSTimeFactor)
#define ktSrcStartupVbus        (150 * kMSTimeFactor)
#define ktSrcTransitionSupply   (350 * kMSTimeFactor)
#define ktSnkTransDefVbus       (150 * kMSTimeFactor)  /* 1.5s for VBus */

#ifdef FSC_HAVE_VDM
#define ktVDMSenderResponse      (27 * kMSTimeFactor)
#define ktVDMWaitModeEntry       (50 * kMSTimeFactor)
#define ktVDMWaitModeExit        (50 * kMSTimeFactor)
#endif /* FSC_HAVE_VDM */

/* Struct object to contain the timer related members */
struct TimerObj {
  FSC_U32 starttime_;         /* Time-stamp when timer started */
  FSC_U32 period_;            /* Timer period */
};

/* Start the timer using the argument in microseconds. */
/* time must be greater than 0. */
void FUSB3601_TimerStart(struct TimerObj *obj, FSC_U32 time);

/* Restart the timer using the last used delay value. */
void FUSB3601_TimerRestart(struct TimerObj *obj);

/* Set time and period to zero to indicate no current period. */
void FUSB3601_TimerDisable(struct TimerObj *obj);
FSC_BOOL FUSB3601_TimerDisabled(struct TimerObj *obj);

/* Returns true when the time passed to Start is up. */
FSC_BOOL FUSB3601_TimerExpired(struct TimerObj *obj);

/* Returns the time remaining in microseconds, or zero if disabled/done. */
FSC_U32 FUSB3601_TimerRemaining(struct TimerObj *obj);

#endif /* FSCPM_TIMER_H_ */

