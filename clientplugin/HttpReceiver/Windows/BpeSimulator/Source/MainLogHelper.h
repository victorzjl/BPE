#ifndef _MAIN_LOG_HELPER_H_
#define _MAIN_LOG_HELPER_H_

#include "LogManager.h"

const int MAIN_MODULE = 30;
DEFINE_MODULE_XLOG(MAIN_MODULE, MAIN_XLOG)
#define MAIN_SLOG(Level,Event) SLOG(MAIN_MODULE,Level,Event)


#endif //_MAIN_LOG_HELPER_H_