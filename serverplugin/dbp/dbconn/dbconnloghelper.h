#ifndef _DB_CONN_LOG_HELPER_H_
#define _DB_CONN_LOG_HELPER_H_

#include "LogManager.h"
#include "AsyncVirtualServiceLog.h"
const int DB_CONN_LOG_MODULE=71;

#define DC_XLOG  SV_XLOG
#define DC_SLOG(Level,Event) SLOG(DB_CONN_LOG_MODULE,Level,Event)

const int DB_CONN_AGENT_LOG_MODULE=71;
#define DCA_XLOG  AVS_XLOG

#define DCA_SLOG(Level,Event) SLOG(DB_CONN_AGENT_LOG_MODULE,Level,Event)


#endif

