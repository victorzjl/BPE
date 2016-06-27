#ifndef _HT_DEALER_H
#define _HT_DEALER_H

#include <detail/_time.h>

#include "IAsyncVirtualService.h"
//#include "HTMsg.h"
#include "WarnCondition.h"
#include "AvenueServiceConfigs.h"
#include "HTErrorMsg.h"
#include <set>
#include <map>
#include <vector>

using std::string;
using std::vector;
using std::set;
using std::map;
using namespace HT;

//class CMessageReceive;

#define HT_VERSION 

#ifdef __cplusplus
extern "C"
{
#endif
    IAsyncVirtualService *create();
	void destroy(IAsyncVirtualService *pVirtualService);

#ifdef __cplusplus
}
#endif

class CHTDealer: public IAsyncVirtualService
{
	public:
		CHTDealer();
		virtual ~CHTDealer();
	
		virtual int Initialize(ResponseService funcResponseService, 
			ExceptionWarn funcExceptionWarn,
			AsyncLog funcAsyncLog);
		virtual int RequestService(IN void *pOwner, IN const void *pBuffer, IN int len);
		virtual void GetSelfCheckState(unsigned int &dwAliveNum, unsigned int &dwAllNum);
		virtual void GetServiceId(vector<unsigned int> &vecServiceIds);
		
		virtual const std::string OnGetPluginInfo() const;
	public:
		void Response(void *handler, const void *pBuffer, unsigned int dwLen);
		void Response( void *handler, unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, int nCode,const void * pBuffer, unsigned int dwLen );
		//void Response(void *handler, unsigned int dwServiceId, unsigned int dwMsgId, unsigned int dwSequenceId, int nCode);
		void Warn(const string &strWarnInfo);
		
		void Log(const timeval_a &tStart,const timeval_a &tEnd, const string &strGuid, const string &serveraddr, unsigned int dwServiceId,
				unsigned int dwMsgId, const string &strIpAddrs,float fSpentHttpTime, const string &requestS,int nCode,
				const string &responseS);
		
	private:
		void Dump();
	private:
		ResponseService m_pResponseCallBack;
		ExceptionWarn m_pExceptionWarn;
		AsyncLog m_pAsyncLog;
		CWarnCondition *m_pWarnCondition;
		
		set<unsigned int> m_vecServiceIds;	
		vector<int> m_vecArrServiceIds;		
		CAvenueServiceConfigs *m_pAvenueServiceConfigs;
	private:
		string m_strLocalIp;
		
};

#endif
