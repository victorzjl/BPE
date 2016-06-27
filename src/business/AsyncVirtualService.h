#ifndef __ASYNC_VIRTUAL_SERVICE_H__
#define __ASYNC_VIRTUAL_SERVICE_H__
#include "IAsyncVirtualService.h"
//#include <boost/thread/shared_mutex.hpp>
#include <map>

using std::map;

struct SCacheSelfCheck
{    
    string strModule;
    unsigned nAlive;
    unsigned nDead;    
    SCacheSelfCheck(string strModule_="", int nAlive_=0,  int nDead_=0)
        :strModule(strModule_),nAlive(nAlive_),nDead(nDead_){}
    SCacheSelfCheck& operator +=(const SCacheSelfCheck& rhds)
    {
        nAlive += rhds.nAlive;
        nDead  += rhds.nDead;
        return *this;
    }
};
    
class CAsyncVirtualService
{
    enum{CACHE_BEG = 49900, CACHE_END = 50000};
    typedef vector<unsigned> servIds;
	typedef IAsyncVirtualService *(*FnCreateObj)();
	typedef void  (*FnDestroyObj)(IAsyncVirtualService*);
	
	struct SVirtualServcie
	{
		IAsyncVirtualService *pServiceObj;
		FnCreateObj fnCreateObj;
		FnDestroyObj fnDestroyObj;
		void *pLibHandle;
        string strModule;
	};

public:
//		static CAsyncVirtualService* Instance();	
//      int Release();
	int Load();
    int OnRequestService(void* pOwner, const void* pBuffer, int nLen);	
	bool isInService(unsigned serviceId){return mapVirtualServcie.find(serviceId)!= mapVirtualServcie.end();}
    void GetSelfCheck(vector<SCacheSelfCheck>& vecInfo);
	const string GetPluginInfo()const;   
	void Dump();
private:
//	    CAsyncVirtualService(){}
	int LoadSO(const char* pSoObj,SVirtualServcie &sService);    
private:
	map<unsigned int, SVirtualServcie> mapVirtualServcie; // key: ServiceId
	map<string, SVirtualServcie> m_mapVS;
	static const char* ASYNC_VSERVICE_PATH;
//	    boost::shared_mutex m_mut;
//	    static CAsyncVirtualService* _instace;
};

#endif

