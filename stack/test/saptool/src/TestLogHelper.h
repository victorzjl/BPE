#ifndef _TEST_LOG_HELPER_H_
#define _TEST_LOG_HELPER_H_
#include "LogManager.h"

const int TEST_MODULE=33;
DEFINE_MODULE_XLOG(TEST_MODULE,TEST_XLOG)
#define TEST_SLOG(Level,Event) SLOG(TEST_MODULE,Level,Event)

#endif

