/*
 * =====================================================================================
 *
 *       Filename:  LogManager.cpp
 *
 *    Description:  log manager imp.
 *
 *        Version:  1.0
 *        Created:  07/29/2009 12:52:02 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zong Jinliang(zjl), 
 *        Company:  SNDA
 *
 * =====================================================================================
 */
#include "LogManager.h"
#include <stdarg.h> 
#include "LogFromLog4cplus.h"

namespace sdo{
	namespace common{
		LogManager * LogManager::instance_=NULL;
		LogManager * LogManager::instance()
		{
			if(instance_==NULL)
			{
				instance_=new LogManager();
			}
			return instance_;
		}
		LogManager::LogManager()
		{
			memset(logModules_,0,sizeof(logModules_));	
		}
		void LogManager::init(const char *filename,bool isAuto)
		{
			LogFromLog4cplus::init(filename,isAuto);
		}
		void LogManager::destroy()
		{
			int i=0;
			ILog *pImp=NULL;
			for(i=0;i<MAX_LOG_MODULES;i++)
			{
				if(logModules_[i]!=NULL)
				{
					pImp=logModules_[i];
					delete pImp;
					logModules_[i]=NULL;
				}
			}
            LogFromLog4cplus::destroy();
			delete instance_;
		}
		void LogManager::register_log_module(int loc, const char *name)
		{
			if(loc>=0&&loc<MAX_LOG_MODULES)
			{
				logModules_[loc]=new LogFromLog4cplus(name);
			}
		}
		void LogManager::unregister_log_module(int loc)
		{
			if(loc>=0&&loc<MAX_LOG_MODULES&&logModules_[loc]!=NULL)
			{
				ILog *pImp=logModules_[loc];
				delete pImp;
				logModules_[loc]=NULL;
			}
		}
		ILog * LogManager::get_log_module(int loc)
		{
			if(loc>=0&&loc<MAX_LOG_MODULES)
			{
				return logModules_[loc];
			}
			else
			{
				return NULL;
			}
		}
		void LogManager::write_fmt(int loc,const XLOG_LEVEL level,const char *fmt, ...)
		{
			ILog *pImp=get_log_module(loc);
			if(pImp!=NULL&&pImp->is_enable_level(level))
			{
				va_list list;
				va_start( list, fmt ); 
				pImp->write_fmt(level,fmt,list);
				va_end( list );
			}
		}
		void LogManager::write_buf(int loc,const XLOG_LEVEL level,const char *buf)
		{
			ILog *pImp=get_log_module(loc);
			if(pImp!=NULL&&pImp->is_enable_level(level))
			{
				pImp->write_buf(level,buf);
			}
		}
	}
}

