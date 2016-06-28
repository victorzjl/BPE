/*
 * =====================================================================================
 *
 *       Filename:  RemoteSysLogAppender.h
 *
 *    Description:  RemoteSysLogAppender
 *
 *        Version:  1.0
 *        Created:  08/27/2009 04:14:15 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ZongJinliang (zjl), 
 *        Company:  SNDA
 *
 * =====================================================================================
 */

#ifndef _REMOTE_SYSLOG_APPENDER_H_
#define _REMOTE_SYSLOG_APPENDER_H_
#include <log4cplus/appender.h>
#include <log4cplus/spi/loggerfactory.h>
#include <log4cplus/spi/factory.h>
#include <log4cplus/helpers/pointer.h>
#include <log4cplus/helpers/property.h>
#include "NoticeLoglevel.h"


#ifdef WIN32
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#endif

namespace log4cplus {

class  RemoteSysLogAppender : public Appender {
public:
	RemoteSysLogAppender(int facility,log4cplus::tstring &host_name,log4cplus::tstring &process_name,unsigned int local_port,const log4cplus::tstring& ip,unsigned int port);
	RemoteSysLogAppender(const log4cplus::helpers::Properties properties);


	virtual ~RemoteSysLogAppender();
	virtual void close();

protected:
	virtual int getSysLogLevel(const LogLevel& ll) const;
	virtual void append(const spi::InternalLoggingEvent& event);

private:
	RemoteSysLogAppender(const RemoteSysLogAppender&);
	RemoteSysLogAppender& operator=(const RemoteSysLogAppender&);

	int socket_;
	struct sockaddr_in serverAddr_;
	int facility_;
	log4cplus::tstring host_name_;
	log4cplus::tstring process_name_;
	unsigned int local_port_;
};

namespace spi{
class RemoteSysLogAppenderFactory : public AppenderFactory {
    public:
        helpers::SharedObjectPtr<Appender> createObject(const helpers::Properties& props)
        {
            return helpers::SharedObjectPtr<Appender>(new log4cplus::RemoteSysLogAppender(props));
        }

        tstring getTypeName() { 
            return LOG4CPLUS_TEXT("log4cplus::RemoteSysLogAppender"); 
        }
    };
}

}
class RemoteSysLogInitializer
{
public:
	RemoteSysLogInitializer();
};
#endif

