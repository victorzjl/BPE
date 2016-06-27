/*
 * =====================================================================================
 *
 *       Filename:  LogManager.h
 *
 *    Description:  log manager
 *
 *        Version:  1.0
 *        Created:  07/29/2009 12:43:36 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zong Jinliang (zjl), 
 *        Company:  SNDA
 *
 * =====================================================================================
 */
#ifndef _SDO_COMMON_LOG_MANAGER_H_
#define _SDO_COMMON_LOG_MANAGER_H_
#include "ILog.h"
#include <sstream>
namespace sdo{
	namespace common{
		const int MAX_LOG_MODULES=128;
		class XLOG_EXPORT LogManager
		{
		public:
			static LogManager * instance();
			void init(const char *filename,bool isAuto);
			void register_log_module(int loc,const char *name);
			void unregister_log_module(int loc);
			ILog * get_log_module(int loc);
			void write_fmt(int loc,const XLOG_LEVEL level,const char *fmt, ...);
			void write_buf(int loc,const XLOG_LEVEL level,const char *buf);
			void destroy();
		private:
			LogManager();
			static LogManager *instance_;
			ILog * logModules_[MAX_LOG_MODULES];
		};
	}

}
void inline XLOG_INIT(const char *filename,bool isAuto=false)
{
	sdo::common::LogManager::instance()->init(filename,isAuto);
}
#define XLOG_REGISTER(module,name) sdo::common::LogManager::instance()->register_log_module(module,name)
#define XLOG sdo::common::LogManager::instance()->write_fmt
#define XLOG_BUFFER sdo::common::LogManager::instance()->write_buf
#define SLOG(Module,Level,Event) \
	do{ \
		sdo::common::ILog *pImp=sdo::common::LogManager::instance()->get_log_module(Module); \
		if(pImp!=NULL&&pImp->is_enable_level(Level)) \
		{ \
			std::ostringstream _SLOG_BUF_INTERNAL_; \
			_SLOG_BUF_INTERNAL_<<Event; \
			pImp->write_fmt(Level,_SLOG_BUF_INTERNAL_.str().c_str()); \
		} \
	}while(0)

#define XLOG_UNREGISTER(module) sdo::common::LogManager::instance()->unregister_log_module(module) 
#define XLOG_DESTROY sdo::common::LogManager::instance()->destroy


#define DEFINE_MODULE_XLOG(module,xlog_func) \
    inline void xlog_func(const XLOG_LEVEL level,const char *fmt, ...) \
{ \
    sdo::common::ILog *pImp=sdo::common::LogManager::instance()->get_log_module(module); \
    if(pImp!=NULL&&pImp->is_enable_level(level)) \
    { \
        va_list list; \
        va_start( list, fmt );  \
        pImp->write_fmt(level,fmt,list); \
        va_end( list ); \
    } \
}
#endif


