/*
 * =====================================================================================
 *
 *       Filename:  LogFromLog4cplus.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/28/2009 12:55:45 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zong Jinliang (zjl), 
 *        Company:  SNDA
 *
 * =====================================================================================
 */

#include "LogFromLog4cplus.h"
#include "RemoteSysLogAppender.h"

NoticeLogLevelInitializer noticeLogLevelInitializer_;
RemoteSysLogInitializer remoteSysLogInitializer_;

namespace sdo{
	namespace common{
		int levels[XLOG_ALL+1]={
			log4cplus::OFF_LOG_LEVEL,
			log4cplus::FATAL_LOG_LEVEL,
			log4cplus::ERROR_LOG_LEVEL,
			log4cplus::WARN_LOG_LEVEL,
			NOTICE_LOG_LEVEL,
			log4cplus::INFO_LOG_LEVEL,
			log4cplus::DEBUG_LOG_LEVEL,
			log4cplus::TRACE_LOG_LEVEL,
			log4cplus::TRACE_LOG_LEVEL
		};
        ConfigureAndWatchThread * LogFromLog4cplus::watchdog_thread_=NULL;
		LogFromLog4cplus::LogFromLog4cplus(const char *instance):imp_(Logger::getInstance(LOG4CPLUS_TEXT(instance)))
		{
		}
		void LogFromLog4cplus::init(const char *filename,bool isAuto)
		{
		    if(isAuto==false)
            {      
			    PropertyConfigurator::doConfigure(LOG4CPLUS_TEXT(filename));
            }
            else if(watchdog_thread_==NULL)
            {
                watchdog_thread_=new ConfigureAndWatchThread(filename,5000);
            }
			else
			{
				delete watchdog_thread_;
                watchdog_thread_=NULL;
				watchdog_thread_=new ConfigureAndWatchThread(filename,5000);
			}
		}
        void LogFromLog4cplus::destroy()
        {
            if(watchdog_thread_!=NULL)
            {
                delete watchdog_thread_;
                watchdog_thread_=NULL;
            }
        }
		bool LogFromLog4cplus::is_enable_level(const XLOG_LEVEL l)
		{
			log4cplus::LogLevel level=levels[l];
			return imp_.isEnabledFor(level);
		}
		void LogFromLog4cplus::write_fmt(const XLOG_LEVEL l,const char *fmt, ...)
		{
			va_list list; 
			va_start( list, fmt ); 
			this->write_fmt(l,fmt,list);
			va_end(list);
		}
		void LogFromLog4cplus::write_fmt(const XLOG_LEVEL l,const char *fmt,va_list list)
		{
#ifdef WIN32
#if (_MSC_VER < 1500) 
#define vsnprintf _vsnprintf 
#endif 
#endif
			log4cplus::LogLevel level=levels[l];
			char buf[2048]={0};
			vsnprintf( buf, (size_t)2047,fmt, list ); 
			imp_.forcedLog(level, buf, __FILE__, __LINE__);
		}
		void LogFromLog4cplus::write_buf(const XLOG_LEVEL l,const char *szBuf)
		{
			log4cplus::LogLevel level=levels[l];
			imp_.forcedLog(level, szBuf, __FILE__, __LINE__);
		}
	}
}
