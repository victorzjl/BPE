#ifndef _ADMIN_LOG_HELPER_H_
#define _ADMIN_LOG_HELPER_H_
#include "LogManager.h"

const int ADMIN_MODULE=45;
DEFINE_MODULE_XLOG(ADMIN_MODULE,A_XLOG)
#define A_SLOG(Level,Event) SLOG(ADMIN_MODULE,Level,Event)

#endif
