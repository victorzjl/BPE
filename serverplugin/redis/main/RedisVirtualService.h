#ifndef _RVS_REDIS_VIRTUAL_SERVICE_H
#define _RVS_REDIS_VIRTUAL_SERVICE_H

#include <detail/_time.h>

#include "IAsyncVirtualService.h"
#include "RedisMsg.h"
#include "WarnCondition.h"
#include "AvenueServiceConfigs.h"
#include <map>
#include <vector>

using std::string;
using std::vector;
using namespace redis;

#define RVS_VERSION "1.2.5"

#ifdef __cplusplus
extern "C"
{
#endif
    IAsyncVirtualService *create();
	void destroy(IAsyncVirtualService *pVirtualService);

#ifdef __cplusplus
}
#endif

class CRedisReadThreadGroup;
class CRedisWriteThreadGroup;
class CRedisReConnThread;

class CRedisVirtualService: public IAsyncVirtualService
{
	public:
		CRedisVirtualService();
		virtual ~CRedisVirtualService();
	
		virtual int Initialize(ResponseService funcResponseService, 
			ExceptionWarn funcExceptionWarn,
			AsyncLog funcAsyncLog);
		virtual int RequestService(IN void *pOwner, IN const void *pBuffer, IN int len);
		virtual void GetSelfCheckState(unsigned int &dwAliveNum, unsigned int &dwAllNum);
		virtual void GetServiceId(vector<unsigned int> &vecServiceIds);
		virtual const std::string OnGetPluginInfo() const;
	public:
		void Response(void *handler, const void *pBuffer, unsigned int dwLen);
		void Response( void *handler, unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, int nCode );
		void Warn(const string &strWarnInfo);
		void Log(const string &strGuid, unsigned int dwServiceId, unsigned int dwMsgId, const timeval_a &tStart, const string &strKey, const string &strValue, int nKeepTime, int nCode, const string &strIpAddrs, float fSpentTimeInQueue);
        void AddRedisServer(const string &strAddr);
		CRedisReConnThread *GetReConnThread() {return m_pRedisReConnThread;}
		CWarnCondition *GetWarnCondition() {return m_pWarnCondition;}
		CAvenueServiceConfigs *GetAvenueServiceConfigs(){return m_pAvenueServiceConfigs;}
	    SERVICE_CODE_TYPE_MAP & GetServiceCodeTypeMap() {return m_mapCodeTypeByService;}
		REDIS_SET_TIME_MAP & GetRedisKeepTime() {return m_mapRedisKeepTime;}
	private:
		void Dump();
	private:
		ResponseService m_pResponseCallBack;
		ExceptionWarn m_pExceptionWarn;
		AsyncLog m_pAsyncLog;
	private:
		map<ERedisType,unsigned long> m_mapReadOperateByRedisType;  //∂¡–¥∑÷¿Î”≥…‰±Ì
	private:
		vector<unsigned int> m_vecServiceIds;
		SERVICE_CODE_TYPE_MAP m_mapCodeTypeByService;
		REDIS_SET_TIME_MAP m_mapRedisKeepTime;
		string m_strLocalIp;
		bool m_bDispatchReadRequest;
	
		CRedisReadThreadGroup *m_pRedisReadThreadGroup;
		CRedisWriteThreadGroup *m_pRedisWriteThreadGroup;
		CRedisReConnThread *m_pRedisReConnThread;
		CWarnCondition *m_pWarnCondition;
        CAvenueServiceConfigs *m_pAvenueServiceConfigs;
};

#endif
