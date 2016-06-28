#ifndef _SAP_LOG_HELPER_H_
#define _SAP_LOG_HELPER_H_
#include "LogManager.h"
#include "AsyncVirtualClientLog.h"

#define SS_XLOG HTTP_REC_XLOG
#define SS_SLOG HTTP_REC_SLOG

#define SP_XLOG HTTP_REC_XLOG
#define SP_SLOG HTTP_REC_SLOG

#define SAP_STACK_MODULE     HTTP_REC_MODULE
#define SAPPER_STACK_MODULE  HTTP_REC_MODULE

//const int SAP_STACK_MODULE=7;
//DEFINE_MODULE_XLOG(SAP_STACK_MODULE,SS_XLOG)
//#define SS_SLOG(Level,Event) SLOG(SAP_STACK_MODULE,Level,Event)
//
//const int SAPPER_STACK_MODULE=21;
//DEFINE_MODULE_XLOG(SAPPER_STACK_MODULE,SP_XLOG)
//#define SP_SLOG(Level,Event) SLOG(SAPPER_STACK_MODULE,Level,Event)

inline bool IsEnableLevel(int nModule,const XLOG_LEVEL level)
{
	sdo::common::ILog *pImp=sdo::common::LogManager::instance()->get_log_module(nModule);
    if(pImp!=NULL&&pImp->is_enable_level(level))
		return true;
	else 
		return false;
}
#endif

