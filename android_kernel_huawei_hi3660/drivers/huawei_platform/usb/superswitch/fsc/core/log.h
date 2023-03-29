/*
 * log.h
 *
 * Defines a log implementation for use per channel.
 */
#ifndef FSCPM_LOG_H_
#define FSCPM_LOG_H_

#ifdef FSC_LOGGING

#include "platform.h"
#include "PDTypes.h"

#define FSC_LOG_SIZE  1024

#define STATE_ENTRY_LENGTH 5

struct Log {
  /* Type-C State Log */
  FSC_U8 tc_data_[FSC_LOG_SIZE];
  FSC_U8 tc_writeindex_;
  FSC_U8 tc_readindex_;
  FSC_BOOL tc_overrun_;

  /* PolicyEngine State Log */
  FSC_U8 pe_data_[FSC_LOG_SIZE];
  FSC_U8 pe_writeindex_;
  FSC_U8 pe_readindex_;
  FSC_BOOL pe_overrun_;

  /* Protocol PD Message Log */
  FSC_U8 pd_data_[FSC_LOG_SIZE];
  FSC_U8 pd_writeindex_;
  FSC_U8 pd_readindex_;
  FSC_BOOL pd_overrun_;
};

void FUSB3601_LogClear(struct Log *obj);

/* Write functionality */
FSC_BOOL FUSB3601_WriteTCState(struct Log *obj, FSC_U32 timestamp, FSC_U8 state);
FSC_BOOL FUSB3601_WritePEState(struct Log *obj, FSC_U32 timestamp, FSC_U8 state);

FSC_BOOL FUSB3601_WritePDToken(struct Log *obj, FSC_BOOL transmitter,
                      USBPD_BufferTokens_t token);
FSC_BOOL FUSB3601_WritePDMsg(struct Log *obj, sopMainHeader_t header,
                    doDataObject_t *dataobject,
                    FSC_BOOL transmitter, FSC_U8 soptoken);

/* Read functionality */
FSC_U32 FUSB3601_LogLength(void);

/* ReadLog takes a buffer and a size and returns the number of bytes written */
/* Call with a buffer bigger than LogLength() or until ReadLog returns 0. */
FSC_U32 FUSB3601_ReadTCLog(struct Log *obj, FSC_U8 *data, FSC_U32 datalength);
FSC_U32 FUSB3601_ReadPELog(struct Log *obj, FSC_U8 *data, FSC_U32 datalength);
FSC_U32 FUSB3601_ReadPDLog(struct Log *obj, FSC_U8 *data, FSC_U32 datalength);

FSC_BOOL FUSB3601_get_pd_overflow(struct Log *obj);
FSC_U32  FUSB3601_get_pd_bytes(struct Log *obj);

#endif /* FSC_LOGGING */

#endif /* FSCPM_LOG_H_ */

