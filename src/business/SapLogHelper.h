#ifndef _SAP_LOG_HELPER_H_
#define _SAP_LOG_HELPER_H_
#include "LogManager.h"


const int SAP_STACK_MODULE=7;
const int SAP_VIRTUAL_MODULE = 11;
const int SAP_STORE_FORWARD_MODULE = 12;
const int SAPPER_STACK_MODULE=21;

DEFINE_MODULE_XLOG(SAP_STACK_MODULE,SS_XLOG)
#define SS_SLOG(Level,Event) SLOG(SAP_STACK_MODULE,Level,Event)

DEFINE_MODULE_XLOG(SAP_VIRTUAL_MODULE,SV_XLOG)
#define SV_SLOG(Level,Event) SLOG(SAP_VIRTUAL_MODULE,Level,Event)

DEFINE_MODULE_XLOG(SAP_STORE_FORWARD_MODULE,SF_XLOG)
#define SF_SLOG(Level,Event) SLOG(SAP_STORE_FORWARD_MODULE,Level,Event)

DEFINE_MODULE_XLOG(SAPPER_STACK_MODULE,SP_XLOG)
#define SP_SLOG(Level,Event) SLOG(SAPPER_STACK_MODULE,Level,Event)

inline bool IsEnableLevel(int nModule,const XLOG_LEVEL level)
{
	sdo::common::ILog *pImp=sdo::common::LogManager::instance()->get_log_module(nModule);
    if(pImp!=NULL&&pImp->is_enable_level(level))
		return true;
	else 
		return false;
}
#endif
