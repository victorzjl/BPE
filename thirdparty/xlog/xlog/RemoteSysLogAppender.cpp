/*
 * =====================================================================================
 *
 *       Filename:  RemoteSysLogAppender.cpp
 *
 *    Description:  RemoteSysLogAppender
 *
 *        Version:  1.0
 *        Created:  08/27/2009 04:18:11 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ZongJinliang (zjl), 
 *        Company:  SNDA
 *
 * =====================================================================================
 */
#include "RemoteSysLogAppender.h"
#include <log4cplus/streams.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/helpers/threads.h>
#include <log4cplus/spi/loggingevent.h>
#include <time.h>
#ifndef WIN32
#include <unistd.h>
#endif

 using namespace std;
 using namespace log4cplus;
 using namespace log4cplus::helpers;
 using namespace log4cplus::spi;

RemoteSysLogAppender::RemoteSysLogAppender(int facility,log4cplus::tstring &host_name,log4cplus::tstring &process_name,
    unsigned int local_port,const log4cplus::tstring& ip,unsigned int port)
    :facility_(facility),host_name_(host_name),process_name_(process_name),local_port_(local_port)
 {
#ifdef WIN32
    WSADATA wsa;
    WSAStartup (MAKEWORD (2, 2), &wsa);
#endif
     socket_ = socket(AF_INET, SOCK_DGRAM, 0);
     memset(&serverAddr_,0,sizeof(serverAddr_)); 
     serverAddr_.sin_family = AF_INET;
     serverAddr_.sin_addr.s_addr =inet_addr(ip.c_str());
     serverAddr_.sin_port = htons(port);
 }
 
 
RemoteSysLogAppender::RemoteSysLogAppender(const Properties properties)
 : Appender(properties)
 {
 #ifdef WIN32
    WSADATA wsa;
    WSAStartup (MAKEWORD (2, 2), &wsa);
#endif
     tstring ip= properties.getProperty( LOG4CPLUS_TEXT("ip") );
     int port=514;
     facility_=1;
    if(properties.exists( LOG4CPLUS_TEXT("port") )) {
        tstring tmp = properties.getProperty( LOG4CPLUS_TEXT("port") );
        port = atoi(LOG4CPLUS_TSTRING_TO_STRING(tmp).c_str());
    }
    if(properties.exists( LOG4CPLUS_TEXT("Facility") )) {
        tstring tmp = properties.getProperty( LOG4CPLUS_TEXT("Facility") );
        facility_ = atoi(LOG4CPLUS_TSTRING_TO_STRING(tmp).c_str());
    }
    if(properties.exists( LOG4CPLUS_TEXT("HostName") )) {
        host_name_ = properties.getProperty( LOG4CPLUS_TEXT("HostName") );
    }
    else
    {
        char szHost[128]={0};
        gethostname(szHost,128);
        host_name_=szHost;
    }
    if(properties.exists( LOG4CPLUS_TEXT("ProcessName") )) {
        process_name_= properties.getProperty( LOG4CPLUS_TEXT("ProcessName") );
    }
    else
    {
        process_name_="NoName";
     }
    socket_ = socket(AF_INET, SOCK_DGRAM, 0);
     memset(&serverAddr_,0,sizeof(serverAddr_)); 
     serverAddr_.sin_family = AF_INET;
     serverAddr_.sin_addr.s_addr = inet_addr(ip.c_str());
     serverAddr_.sin_port = htons(port);

     if(properties.exists( LOG4CPLUS_TEXT("LocalPort") )) {
        tstring tmp = properties.getProperty( LOG4CPLUS_TEXT("LocalPort") );
        int localPort=atoi(LOG4CPLUS_TSTRING_TO_STRING(tmp).c_str());
        
        struct sockaddr_in localAddr;
        memset(&localAddr,0,sizeof(localAddr)); 
        localAddr.sin_family = AF_INET;
        localAddr.sin_addr.s_addr = htons(INADDR_ANY);
        localAddr.sin_port = htons(localPort);
        
        
#ifdef WIN32
        char val=1;
		int len=sizeof(val);
#else
        long val=1;
		socklen_t len=sizeof(val);
#endif
		setsockopt(socket_,SOL_SOCKET,SO_REUSEADDR,&val,len);
        bind(socket_,(struct sockaddr *)&localAddr,sizeof(localAddr));
    }
 }
 
 
RemoteSysLogAppender::~RemoteSysLogAppender()
 {
     destructorImpl();
 }

 void 
 RemoteSysLogAppender::close()
 {
     getLogLog().debug(LOG4CPLUS_TEXT("Entering RemoteSysLogAppender::close()..."));
 #ifdef WIN32
    ::closesocket(socket_);
    WSACleanup();
#else
    ::close(socket_);
#endif
     closed = true;
 }

 int
 RemoteSysLogAppender::getSysLogLevel(const LogLevel& ll) const
 {
     if(ll < DEBUG_LOG_LEVEL) {
         return -1;
     }
     else if(ll < INFO_LOG_LEVEL) {
         return 7;
     }
     else if(ll < NOTICE_LOG_LEVEL) {
         return 6;
     }
     else if(ll < WARN_LOG_LEVEL) {
         return 5;
     }
     else if(ll < ERROR_LOG_LEVEL) {
         return 4;
     }
     else if(ll < FATAL_LOG_LEVEL) {
         return 3;
     }
     else if(ll == FATAL_LOG_LEVEL) {
         return 2;
     }
 
     return 1;  
 }
 
 void
 RemoteSysLogAppender::append(const spi::InternalLoggingEvent& event)
 {
     int level = getSysLogLevel(event.getLogLevel());
     if(level != -1) {
        int nPRI=(facility_<<3)+level;

        char szTime[192]={0};
        time_t t;
        t = time(NULL);
        strftime(szTime, sizeof(szTime), "%b %d %H:%M:%S ", localtime(&t));
        
         log4cplus::tostringstream buf;
         buf<<"<"<<nPRI<<">"<<szTime<<host_name_<<" " <<process_name_
            <<"["<<log4cplus::thread::getCurrentThreadName()<<"]: ";
         layout->formatAndAppend(buf, event);
         ::sendto(socket_,LOG4CPLUS_TSTRING_TO_STRING(buf.str()).c_str(),LOG4CPLUS_TSTRING_TO_STRING(buf.str()).length(),0,(struct sockaddr *)(&serverAddr_),sizeof(serverAddr_));  
     }
 }
 RemoteSysLogInitializer::RemoteSysLogInitializer()
{
    AppenderFactoryRegistry& reg = getAppenderFactoryRegistry();
    reg.put (std::auto_ptr<AppenderFactory> (new RemoteSysLogAppenderFactory));
}


