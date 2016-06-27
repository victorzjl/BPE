#ifndef SDK_LOG_HELPER_H
#define SDK_LOG_HELPER_H
#include "LogManager.h"
#include "SapLogHelper.h"
#ifdef WIN32
#define snprintf _snprintf
#endif
const int SDK_MODULE = 41;
DEFINE_MODULE_XLOG(SDK_MODULE,SDK_XLOG)
#define SDK_SLOG(Level, Event) SLOG(SDK_MODULE, Level, Event)

#endif

