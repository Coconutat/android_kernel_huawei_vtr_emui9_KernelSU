/*
 * timer.c
 *
 * Implements a timer class for time tracking against a system clock
 */

#include "timer.h"
#include "platform.h"

void FUSB3601_TimerStart(struct TimerObj *obj, FSC_U32 time) {
  /* Grab the current time stamp and store the wait period. */
  /* Time must be > 0 */
  obj->starttime_ = FUSB3601_platform_current_time();
  obj->period_ = time;
}

void FUSB3601_TimerRestart(struct TimerObj *obj) {
  /* Grab the current time stamp for the next period. */
  obj->starttime_ = FUSB3601_platform_current_time();
}

void FUSB3601_TimerDisable(struct TimerObj *obj) {
  /* Zero means disabled */
  obj->starttime_ = obj->period_ = 0;
}

FSC_BOOL FUSB3601_TimerDisabled(struct TimerObj *obj) {
  /* Zero means disabled */
  return (obj->period_ == 0) ? TRUE : FALSE;
}

FSC_BOOL FUSB3601_TimerExpired(struct TimerObj *obj) {
  FSC_U32 currenttime = FUSB3601_platform_current_time();
  FSC_U32 endtime = obj->starttime_ + obj->period_;
  FSC_BOOL result = FALSE;

  if (FUSB3601_TimerDisabled(obj)) {
      /* Disabled */
      result = FALSE;
  }
  else if (endtime > obj->starttime_) {
    /* No roll-over expected */
    if (currenttime == obj->starttime_) {
      result = FALSE;   /* Still waiting */
    }
    else if (currenttime > obj->starttime_ && currenttime < endtime) {
      result = FALSE;   /* Still waiting */
    }
    else if (currenttime > obj->starttime_ && currenttime > endtime) {
      result = TRUE;    /* Done! */
    }
    else {
      result = TRUE;    /* Ticker_ rolled over - Done! */
    }
  }
  else {
    /* Roll-over expected */
    if (currenttime == obj->starttime_) {
      result = FALSE;   /* Still waiting */
    }
    else if (currenttime > obj->starttime_ && currenttime > endtime) {
      result = FALSE;   /* Still waiting */
    }
    else if (currenttime < obj->starttime_ && currenttime < endtime) {
      result = FALSE;   /* Ticker_ rolled over - Still waiting */
    }
    else {
      result = TRUE;    /* Ticker_ rolled over - Done! */
    }
  }

//  /* The timer has outlived it's usefulness */
//  if(result == TRUE) TimerDisable(obj);
  return result;
}

FSC_U32 FUSB3601_TimerRemaining(struct TimerObj *obj)
{
  FSC_U32 currenttime = FUSB3601_platform_current_time();
  FSC_U32 endtime = obj->starttime_ + obj->period_;

  if (FUSB3601_TimerDisabled(obj) || FUSB3601_TimerExpired(obj)) {
    return 0;
  }

  /* Check for rollover... */
  if (endtime < currenttime) {
    return (0xFFFFFFFF - currenttime) - endtime;
  }
  else {
    return endtime - currenttime;
  }
}

