#ifndef _DBP_SERVICE_H_
#define _DBP_SERVICE_H_


#include "IAsyncVirtualService.h"
#include "AsyncVirtualServiceLog.h"

#include <map>
#include <vector>
//#include "SapMessage.h"



#ifdef WIN32
	#ifdef DBP2_EXPORTS 
		#define DBPEXPORT _declspec(dllexport) 
	#else
		#define DBPEXPORT
	#endif
#endif
#ifdef __cplusplus
extern "C"
{
#endif
#ifdef WIN32
	DBPEXPORT IAsyncVirtualService* create();
	DBPEXPORT void destroy(IAsyncVirtualService* p);
	DBPEXPORT IAsyncVirtualService* retest();
#else
	IAsyncVirtualService* create();
	void destroy(IAsyncVirtualService* p);
	IAsyncVirtualService* retest();
#endif


#ifdef __cplusplus
}
#endif

//using sdo::sap::CSapDecoder;
//using sdo::sap::CSapEncoder;

typedef void* SAVSParams;

using std::map;
enum {CONFIG_NOT_INIT=0, CONFIG_IS_INITING=1, CONFIG_OK_INIT=2};
class DbpService : public IAsyncVirtualService
{
public:
	virtual int Initialize(ResponseService funcResponseService, 
                            ExceptionWarn funcExceptionWarn,
                            AsyncLog funcAsyncLog);
	virtual int RequestService(IN SAVSParams sParams, IN const void *pBuffer, IN int len) ;
	virtual void GetSelfCheckState(unsigned int &dwAliveNum, unsigned int &dwAllNum) ;
	virtual void GetServiceId(vector<unsigned int> &vecServiceIds) ;
	virtual const std::string OnGetPluginInfo()const ;
	string  GetServiceAddress();
	virtual ~DbpService();
public:
	static DbpService* GetInstance();
	
	void CallResponseService(SAVSParams  para, const void *, unsigned int);
	void CallExceptionWarn(const string &);
	void CallAsyncLog(int nModel,XLOG_LEVEL level,int nInterval,const string &strMsg,
		int nCode,const string& strAddr,int serviceId, int msgId);
		
private:
	ResponseService m_fnResponseService;
	ExceptionWarn   m_fnExceptionWarn;
	AsyncLog	    m_fnAsyncLog;
	string 			m_strServiceAddress;
	

	
};

#endif
