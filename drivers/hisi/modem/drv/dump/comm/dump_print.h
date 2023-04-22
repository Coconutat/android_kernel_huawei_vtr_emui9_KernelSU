#ifndef __DUMP_PRINT_H__
#define __DUMP_PRINT_H__
#include "bsp_om.h"
#include "bsp_trace.h"


/**************************************************************************
  OTHERS∂®“Â
**************************************************************************/
#define dump_debug(fmt, ...)    (bsp_trace(BSP_LOG_LEVEL_ERROR,   BSP_MODU_DUMP, "[MODEM_DUMP]: <%s> "fmt, __FUNCTION__, ##__VA_ARGS__))
#define dump_warning(fmt, ...)  (bsp_trace(BSP_LOG_LEVEL_WARNING, BSP_MODU_DUMP, "[MODEM_DUMP]: <%s> "fmt, __FUNCTION__, ##__VA_ARGS__))
#define dump_error(fmt, ...)    (bsp_trace(BSP_LOG_LEVEL_ERROR,   BSP_MODU_DUMP, "[MODEM_DUMP]: <%s> "fmt, __FUNCTION__, ##__VA_ARGS__))
#define dump_fetal(fmt, ...)    (bsp_trace(BSP_LOG_LEVEL_FATAL,   BSP_MODU_DUMP, "[MODEM_DUMP]: <%s> "fmt, __FUNCTION__, ##__VA_ARGS__))

#endif
