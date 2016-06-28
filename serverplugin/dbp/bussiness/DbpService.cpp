#include "DbpService.h"
#ifndef WIN32
#include <arpa/inet.h>
#else
#include"DbMsg.h"
	int gettimeofday(struct timeval* tv,int dummy) 
   {
      union {
         long long ns100;
         FILETIME ft;
      } now;
     
      GetSystemTimeAsFileTime (&now.ft);
      tv->tv_usec = (long) ((now.ns100 / 10LL) % 1000000LL);
      tv->tv_sec = (long) ((now.ns100 - 116444736000000000LL) / 10000000LL);
     return (0);
   }
#endif

#include "SmartThread.h"
#include <algorithm>
#include "DbThreadGroup.h"
#include "DbReconnThread.h"
#include "util.h"

#include "dbserviceconfig.h"
#include "AsyncLogThread.h"
using namespace std;

//using namespace sdo::sap;

#include "soci.h"
#include "soci-oracle.h"
#include "pthread.h"
using namespace soci;

pthread_mutex_t theMutex = PTHREAD_MUTEX_INITIALIZER; 
DbpService * theDbpService = NULL;

volatile int	bInit = CONFIG_NOT_INIT;
const char* PLUGIN_VERSION="DBPVERSION: 1.0.2.18";

IAsyncVirtualService* retest()
{
	bInit = CONFIG_NOT_INIT;
	theDbpService = NULL;
	return NULL;
}

IAsyncVirtualService* create()
{
	pthread_mutex_lock(&theMutex);

	if (NULL == theDbpService)
	{
		XLOG_REGISTER(SAP_VIRTUAL_MODULE,"dblog");
	
		XLOG_REGISTER(ASYNC_VIRTUAL_MODULE,"dbreqest");

		SV_XLOG(XLOG_DEBUG, "DbpService::%s new\n", __FUNCTION__);

		theDbpService = new DbpService;
		pthread_mutex_unlock(&theMutex);
   		return theDbpService;     

	}	
	else
	{ 
		SV_XLOG(XLOG_DEBUG, "DbpService::%s theDbpService\n", __FUNCTION__);
		pthread_mutex_unlock(&theMutex);
		return theDbpService;
	}
	

	
}
void destroy(IAsyncVirtualService* p)
{
	pthread_mutex_lock(&theMutex);
	if (theDbpService!=NULL)
	{
		CDbReconnThread::GetInstance()->Stop();
		CDbThreadGroup::GetInstance()->Stop();
		delete p;
		theDbpService = NULL;
		SV_XLOG(XLOG_DEBUG, "DbpService::%s\n", __FUNCTION__);
	}
	else
	{
		SV_XLOG(XLOG_DEBUG, "DbpService::%s theDbpService\n", __FUNCTION__);
	}
	pthread_mutex_unlock(&theMutex);
}

DbpService* DbpService::GetInstance(){return theDbpService;}

int DbpService::Initialize(ResponseService funcResponseService, 
					ExceptionWarn funcExceptionWarn,
					AsyncLog funcAsyncLog)
{	
	//XLOG_INIT("log.properties",true);
	pthread_mutex_lock(&theMutex);
	if (CONFIG_OK_INIT == bInit)
	{
		SV_XLOG(XLOG_DEBUG, "DbpService::%s theDbpService\n", __FUNCTION__);
		pthread_mutex_unlock(&theMutex);
		return 0;
	}
	else if (CONFIG_NOT_INIT == bInit)
	{
		bInit = CONFIG_IS_INITING;
		SV_XLOG(XLOG_DEBUG, "DbpService::%s \n", __FUNCTION__); 
	}
	else if (CONFIG_IS_INITING == bInit)
	{
		SV_XLOG(XLOG_DEBUG, "DbpService::%s theDbpService INITING\n", __FUNCTION__);
		while (CONFIG_IS_INITING == bInit)
		{
			sleep(1);
		}
		pthread_mutex_unlock(&theMutex);
		return 0;
	}
	
	m_fnResponseService = funcResponseService;
	m_fnExceptionWarn = funcExceptionWarn;
	m_fnAsyncLog = funcAsyncLog;
	//first init config	

#ifndef WIN32
	CDBServiceConfig::GetInstance()->LoadServiceConfig("./avenue_conf/");
	CDBServiceConfig::GetInstance()->LoadDBConnConfig("./async_virtual_service/");
#else

	CDBServiceConfig::GetInstance()->LoadServiceConfig("./bpe/avenue_conf/");
	CDBServiceConfig::GetInstance()->LoadDBConnConfig("./bpe/async_virtual_service/");
#endif	
	//second 
	CDbReconnThread::GetInstance()->Start();
	
	//third
	CDbThreadGroup::GetInstance()->OnLoadConfig();
	
	CDbThreadGroup::GetInstance()->Start(CDBServiceConfig::GetInstance()->GetDBThreadsNum());
	
	CSmartThread::GetInstance()->Start();
	

	//serviceip
	char szBuf[512]={0};
	GetLocalAddress(szBuf);
	m_strServiceAddress = szBuf;

#ifndef M_SUPPORT_PUBLIC
	SV_XLOG(XLOG_FATAL, "DbpService::%s CONFIG_OK_INIT %s Build:\n", __FUNCTION__,PLUGIN_VERSION,__TIME__,__DATE__);
#else
	SV_XLOG(XLOG_FATAL, "DbpService::%s CONFIG_OK_INIT M_SUPPORT_PUBLIC %s Build:\n", __FUNCTION__,PLUGIN_VERSION,__TIME__,__DATE__);
#endif
	bInit = CONFIG_OK_INIT;
	pthread_mutex_unlock(&theMutex);
	return 0;
}

string DbpService::GetServiceAddress()
{
	return m_strServiceAddress;
}
int DbpService::RequestService(IN SAVSParams sParams, IN const void *pBuffer, IN int len) 
{
	SV_XLOG(XLOG_DEBUG, "DbpService::%s, Buffer[%x], Len[%d]\n",__FUNCTION__, pBuffer, len);

	CDbThreadGroup::GetInstance()->OnRequest(sParams,pBuffer,len);
	
	return 0;
}

void DbpService::GetSelfCheckState(unsigned int &dwAliveNum, unsigned int &dwAllNum) 
{
	SV_XLOG(XLOG_DEBUG, "DbpService::%s\n", __FUNCTION__);
	CDbThreadGroup::GetInstance()->GetThreadState(dwAliveNum,dwAllNum);
}

void DbpService::GetServiceId(vector<unsigned int> &vecServiceIds) 
{
	SV_XLOG(XLOG_DEBUG, "DbpService::%s\n", __FUNCTION__);
	

	CDBServiceConfig::GetInstance()->GetAllServiceIds(vecServiceIds);
	
	string strServices;
	for(unsigned int index=0;index<vecServiceIds.size();index++)
	{
		char szBuf[32]={0};
		snprintf(szBuf,sizeof(szBuf),"%u",vecServiceIds[index]);
		strServices+=szBuf;
		if (index!=vecServiceIds.size()-1)
		{
			strServices+=",";
		}
	}
	SV_XLOG(XLOG_DEBUG, "DbpService::%s %s\n", __FUNCTION__,strServices.c_str());
}

const std::string DbpService::OnGetPluginInfo()const 
{
	SV_XLOG(XLOG_DEBUG, "DbpService::%s\n", __FUNCTION__);
	return string("<tr><td>CacheVirtualService.so</td><td>")
		+ PLUGIN_VERSION
		+	"</td><td>"
		+ __TIME__
		+ " " 
		+ __DATE__
		+ "</td></tr>";
}

DbpService::~DbpService(){}


void DbpService::CallResponseService(SAVSParams  para,const void *pAvenue, unsigned int length)
{
	m_fnResponseService(para,pAvenue,length);
}

void DbpService::CallExceptionWarn(const string & warnStr)
{
	m_fnExceptionWarn(warnStr);
}

void DbpService::CallAsyncLog(int nModel,XLOG_LEVEL level,int nInterval,const string &strMsg,int nCode,const string& strAddr,int serviceId, int msgId)
{
	m_fnAsyncLog(ASYNC_VIRTUAL_MODULE,XLOG_INFO,nInterval,strMsg,nCode,m_strServiceAddress.c_str(),serviceId,msgId);
}




