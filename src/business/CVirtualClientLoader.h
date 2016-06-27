#ifndef __ASYNC_VIRTUAL_LOADER_H__
#define __ASYNC_VIRTUAL_LOADER_H__
#include "IAsyncVirtualClient.h"
#include <map>
using std::map;
#include "AsyncVirtualService.h"
#define VIRTUAL_CLIENT_ADDRESS "127.0.127.127"    

struct VirtualClientHandle
{
	VirtualClientHandle(void*pAsyncVirtualClient ,void* handle):pPlugIn((IAsyncVirtualClient*)pAsyncVirtualClient),phandle(handle)
	{
	}
	IAsyncVirtualClient *pPlugIn;
	void* phandle;
};

class CVirtualClientLoader
{
    typedef vector<unsigned> servIds;
	typedef IAsyncVirtualClient *(*FnCreateObj)();
	typedef void  (*FnDestroyObj)(IAsyncVirtualClient*);
	
	struct SVirtualServcie
	{
		IAsyncVirtualClient *pServiceObj;
		FnCreateObj fnCreateObj;
		FnDestroyObj fnDestroyObj;
		void *pLibHandle;
        string strModule;
	};

public:
	int Load();
	int OnRequestService(void* pOwner, const void* pBuffer, int nLen);
    int OnResponseService(void* handle, const void* pBuffer, int nLen);	
	bool isInService(unsigned serviceId){return mapVirtualServcie.find(serviceId)!= mapVirtualServcie.end();}
    void GetSelfCheck(vector<SCacheSelfCheck>& vecInfo);
	const string GetPluginInfo()const;   
	void Dump();
private:
	int LoadSO(const char* pSoObj,SVirtualServcie &sService);    
private:
	map<unsigned int, SVirtualServcie> mapVirtualServcie; // key: ServiceId
	map<string, SVirtualServcie> m_mapVS;
	static const char* ASYNC_CLIENT_PATH;
};

#endif

