#ifndef _COH_LOG_HELP_H_
#define _COH_LOG_HELP_H_
#include "LogManager.h"

const int COH_STACK_MODULE=6;
DEFINE_MODULE_XLOG(COH_STACK_MODULE,CS_XLOG)
#define CS_SLOG(Level,Event) SLOG(COH_STACK_MODULE,Level,Event)

#endif
