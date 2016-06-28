/*
 * =====================================================================================
 *
 *       Filename:  test.cpp
 *
 *    Description:  test
 *
 *        Version:  1.0
 *        Created:  07/28/2009 12:19:21 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  
 *
 * =====================================================================================
 */
#define USE_XLOG
const int TEST_MODULE=33;

#ifdef USE_XLOG
#include "LogManager.h"
#define TEST_XLOG write_test_log
#define TEST_SLOG(Level,Event) SLOG(TEST_MODULE,Level,Event)
inline void write_test_log(const XLOG_LEVEL level,const char *fmt, ...)
{
	sdo::common::ILog *pImp=sdo::common::LogManager::instance()->get_log_module(TEST_MODULE);
	if(pImp!=NULL&&pImp->is_enable_level(level))
	{
		va_list list;
		va_start( list, fmt ); 
		pImp->write_fmt(level,fmt,list);
		va_end( list );
	}
}
#else
typedef enum{
	XLOG_OFF=0,XLOG_FATAL=1, XLOG_ERROR=2,XLOG_WARNING=3,
	XLOG_INFO=4,XLOG_NOTICE=5,XLOG_DEBUG=6,XLOG_TRACE=7,
	XLOG_ALL=8}XLOG_LEVEL;
#define LOG1 write_coh_log
#define LOG2(Level,Event) 
inline void write_test_log(const XLOG_LEVEL level,const char *fmt, ...){}
#endif

int main()
{
	XLOG_INIT("log.properties");
	XLOG_REGISTER(TEST_MODULE,"coh");
	TEST_XLOG(XLOG_TRACE,"1 heelo[%s],num[%d]\n","123",34);
	TEST_SLOG(XLOG_DEBUG,"2 hello["<<123<<"],num["<<45<<"]\n");
	TEST_XLOG(XLOG_INFO,"3 heelo[%s],num[%d]\n","123",34);
	TEST_XLOG(XLOG_NOTICE,"4 heelo[%s],num[%d]\n","123",34);
	TEST_XLOG(XLOG_WARNING,"5 heelo[%s],num[%d]\n","123",34);
	TEST_XLOG(XLOG_ERROR,"6 heelo[%s],num[%d]\n","123",34);
	TEST_XLOG(XLOG_FATAL,"7 heelo[%s],num[%d]\n","123",34);


	char c=0;
	while(1)
	{
		c=getchar();
		if(c=='a')
		{
			XLOG_INIT("log.properties");
			TEST_XLOG(XLOG_TRACE,"1 heelo[%s],num[%d]\n","123",34);
			TEST_SLOG(XLOG_DEBUG,"2 hello["<<123<<"],num["<<45<<"]\n");
			TEST_XLOG(XLOG_INFO,"3 heelo[%s],num[%d]\n","123",34);
			TEST_XLOG(XLOG_NOTICE,"4 heelo[%s],num[%d]\n","123",34);
			TEST_XLOG(XLOG_WARNING,"5 heelo[%s],num[%d]\n","123",34);
			TEST_XLOG(XLOG_ERROR,"6 heelo[%s],num[%d]\n","123",34);
			TEST_XLOG(XLOG_FATAL,"7 heelo[%s],num[%d]\n","123",34);
		}
		else if(c=='q')
		{
			break;
		}
		else if(c=='o')
		{
			int i=0;
			for(i=0;i<1000;i++)
			{
				TEST_XLOG(XLOG_TRACE,"1 print i[%d]\n",i);                                                
				TEST_XLOG(XLOG_DEBUG,"2 print i[%d]\n",i);
				TEST_XLOG(XLOG_INFO,"3 print i[%d]\n",i);
				TEST_XLOG(XLOG_NOTICE,"4 print i[%d]\n",i);
				TEST_XLOG(XLOG_WARNING,"5 print i[%d]\n",i);  
				TEST_XLOG(XLOG_ERROR,"6 print i[%d]\n",i);
				TEST_XLOG(XLOG_FATAL,"7 print i[%d]\n",i);
			}
		}
		else if(c!='\n')
		{
			TEST_XLOG(XLOG_TRACE,"1 print c[%c]\n",c);
			TEST_XLOG(XLOG_DEBUG,"2 print c[%c]\n",c);
			TEST_XLOG(XLOG_INFO,"3 print c[%c]\n",c);
			TEST_XLOG(XLOG_NOTICE,"4 print c[%c]\n",c);
			TEST_XLOG(XLOG_WARNING,"5 print c[%c]\n",c);
			TEST_XLOG(XLOG_ERROR,"6 print c[%c]\n",c);
			TEST_XLOG(XLOG_FATAL,"7 print c[%c]\n",c);
		}
	}
	XLOG_UNREGISTER(TEST_MODULE);
	XLOG_DESTROY();
	return 1;
}

