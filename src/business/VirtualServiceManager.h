#ifndef VIRTUAL_SERVICE_MANAGER_H
#define VIRTUAL_SERVICE_MANAGER_H
#include "IVirtualService.h"
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <string>
#include <map>
#include <boost/unordered_map.hpp>

using namespace std;

typedef struct stVirtualReload
{
	boost::mutex &mut;
	boost::condition_variable &cond;
	int nRet;
	bool isTimeout;
	stVirtualReload(boost::mutex &mut_,boost::condition_variable &cond_)
		:mut(mut_),cond(cond_),nRet(0),isTimeout(false)
		{}
}SVSReload;

typedef struct stPlugInfo
{
	boost::mutex &mut;
	boost::condition_variable &cond;
	string strInfo;
	bool isTimeout;
	stPlugInfo(boost::mutex &mut_,boost::condition_variable &cond_)
		:mut(mut_),cond(cond_),isTimeout(false)
		{}
}SPlugInfo;


class CVirtualServiceManager
{
	typedef IVirtualService *(*FnCreateObj)();
	typedef void  (*FnDestroyObj)(IVirtualService*);
	
	struct SVirtualServcie
	{
		IVirtualService *pServiceObj;
		FnCreateObj fnCreateObj;
		FnDestroyObj fnDestroyObj;
		void *pLibHandle;
	};

public:
	CVirtualServiceManager(){}
	
	~CVirtualServiceManager();
	
	int LoadVirtualService();
	int ReleaseVirtualService();
	int LoadServiceByName(const string &strName);
	int ReleaseServiceByName(const string& strName);
	bool IsVirtualService(unsigned int nServiceId);
	int OnRequestService(unsigned int nServiceId, const void *pInPacket, unsigned int nLen,
		void **ppOutPacket, int *pLen);
	void SetData(const void *pBuffer, unsigned int nLen);
	const string OnGetPluginInfo()const;
	
	//IVirtualService *GetService(unsigned int nServiceId);
	void Dump();
private:
	int LoadSO(const char* pSoObj,SVirtualServcie &sService);
private:
	map<unsigned int, SVirtualServcie> mapVirtualServcie; // key: ServiceId
	boost::unordered_map<string,SVirtualServcie> m_mapNameVService;
	static const char* VIRTUAL_SERVICE_PATH;
};

#endif

