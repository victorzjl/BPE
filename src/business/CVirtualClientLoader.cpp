#include "CVirtualClientLoader.h"
#include <dlfcn.h>
#include <sched.h>
#include <boost/thread/locks.hpp> 

#include "SessionManager.h"
#include "AsyncLogThread.h"
#include "DirReader.h"
#include "SapLogHelper.h"
#include "SapStack.h"
#ifdef WIN32
	#define CST_PATH_SEP	"\\"
#else
	#define CST_PATH_SEP	"/"	
#endif

void AsyncVritualClient_OnSendResponse2BPE(void* pOwner, const void* buffer, int len)
{
    ((CSessionManager*)pOwner)->OnRecvVirtualClient(buffer, len);
}

void AsyncVritualClient_OnSendResponse(IAsyncVirtualClient* pPlugIn, void* handle, const void* buffer, int len)
{
	VirtualClientHandle* pVCH = new VirtualClientHandle(pPlugIn,handle);
	CSapStack::Instance()->GetThread()->OnRecvVirtualClientRequest(pVCH, buffer, len);
}

void AsyncVritualClient_ReportError(const string & strInfo)
{
    CAsyncLogThread::GetInstance()->ReportAsyncVSError(strInfo);
}

void AsyncVritualClient_OnAsyncLog(int nModel, XLOG_LEVEL level, int nInterval, const string & strMsg, 
    int nCode, const string & strAddr, int serviceId, int msgId)
{
    SS_XLOG(XLOG_DEBUG, "%s...\n", __FUNCTION__);
    CAsyncLogThread::GetInstance()->OnAsyncLog(nModel, level, 0, nInterval, 
        strMsg, nCode, strAddr, serviceId, msgId, false);
}

const char* CVirtualClientLoader::ASYNC_CLIENT_PATH = "./async_virtual_client";

int CVirtualClientLoader::Load()
{		
	SS_XLOG(XLOG_DEBUG, "CVirtualClientLoader::%s...\n", __FUNCTION__);
	sdo::commonsdk::CDirReader oDirReader("*.so");
	if (!oDirReader.OpenDir(ASYNC_CLIENT_PATH, true))
	{
        SS_XLOG(XLOG_DEBUG, "CVirtualClientLoader::%s, open dir error\n",__FUNCTION__);
		return -1;
	} 

	char szFileName[MAX_PATH] = {0};	
	if (oDirReader.GetFirstFilePath(szFileName) == false) 
	{
		SS_XLOG(XLOG_ERROR,"CVirtualClientLoader::%s, GetFirstFilePath error\n", __FUNCTION__);
		return  -1;
	}
    servIds vecServIds;
    //char szSoObj[MAX_PATH] = {0};
	do
	{ 	   
		//sprintf(szSoObj, "%s%s%s", ASYNC_CLIENT_PATH, CST_PATH_SEP, szFileName);
		SVirtualServcie service;
        if(0 != LoadSO(szFileName, service))
    	{
            continue;
    	} 
        service.pServiceObj->GetServiceId(vecServIds);
        for(servIds::iterator it=vecServIds.begin(); it!=vecServIds.end(); ++it)
        {
            mapVirtualServcie.insert(make_pair(*it, service));
            service.strModule = szFileName;            
        }
        m_mapVS.insert(make_pair(szFileName, service));
	}
	while(oDirReader.GetNextFilePath(szFileName));
	return 0;
}


int CVirtualClientLoader::LoadSO(const char* pSoObj,SVirtualServcie &service)
{
    SS_XLOG(XLOG_DEBUG, "CVirtualClientLoader::%s path[%s]\n",__FUNCTION__,pSoObj);
    void *p_lib = dlopen(pSoObj, RTLD_LAZY|RTLD_GLOBAL);
	if(p_lib == NULL)
	{
		SS_XLOG(XLOG_ERROR, "CVirtualClientLoader::%s, dlopen[%s] error[%s]\n", __FUNCTION__,pSoObj,dlerror());
		return -1;
	}

	service.pLibHandle = p_lib;
	service.fnCreateObj = (FnCreateObj)dlsym(p_lib, "create");
	char* dlsym_error = dlerror();
	if (dlsym_error)
	{
		dlclose(p_lib);
		SS_XLOG(XLOG_ERROR,"CVirtualClientLoader::%s, dlsym create[%s] error[%s]\n", __FUNCTION__, pSoObj, dlsym_error);
		return -1;					 
	}

	service.fnDestroyObj = (FnDestroyObj)dlsym(p_lib, "destroy");
	dlsym_error = dlerror();
	if (dlsym_error) 
	{
		dlclose(p_lib);
		SS_XLOG(XLOG_ERROR,"CVirtualClientLoader::%s,dlsym destroy[%s] error[%s]\n", __FUNCTION__, pSoObj, dlsym_error);
		return -1;
	}

	service.pServiceObj = service.fnCreateObj();
	if(service.pServiceObj  == NULL)
	{
		SS_XLOG(XLOG_ERROR, "CVirtualClientLoader::%s, create object failed\n", __FUNCTION__);
		return -1;
	}
    if (service.pServiceObj->Initialize(AsyncVritualClient_OnSendResponse, 
			AsyncVritualClient_OnSendResponse2BPE,
			AsyncVritualClient_ReportError, 
			AsyncVritualClient_OnAsyncLog) != 0)
    {
        SS_XLOG(XLOG_ERROR, "CVirtualClientLoader::%s, init failed\n", __FUNCTION__);
		return -1;
    }

	SS_XLOG(XLOG_DEBUG, "CVirtualClientLoader::%s create destroy[%x], obj[%x], lib[%x]\n",
		__FUNCTION__, service.fnDestroyObj, service.pServiceObj, p_lib); 
    return 0;
}

int CVirtualClientLoader::OnRequestService(void* pOwner, const void* pBuffer, int nLen)
{
	SS_XLOG(XLOG_DEBUG, "CVirtualClientLoader::%s session manager[%x]\n", __FUNCTION__, pOwner);
    SSapMsgHeader *pHead = (SSapMsgHeader *)pBuffer;
    unsigned serviceId = ntohl(pHead->dwServiceId);
	map<unsigned int, SVirtualServcie>::iterator iter = mapVirtualServcie.find(serviceId);
	if (iter == mapVirtualServcie.end())
	{
		SS_XLOG(XLOG_WARNING, "CVirtualClientLoader::%s,serviceid[%u] not found\n", __FUNCTION__, serviceId);
	    return SAP_CODE_VIRTUAL_SERVICE_NOT_FOUND;
	}
    
    SVirtualServcie& service = iter->second;  
    return service.pServiceObj->RequestService(pOwner, pBuffer, nLen);
}

int CVirtualClientLoader::OnResponseService(void* handle, const void* pBuffer, int nLen)
{
	SS_XLOG(XLOG_DEBUG, "CVirtualClientLoader::%s \n", __FUNCTION__);
	VirtualClientHandle* pVCH = (VirtualClientHandle*) handle;
	void * pHandle = pVCH->phandle; 
	IAsyncVirtualClient* pPlugIn = pVCH->pPlugIn; 
	delete pVCH;
    return pPlugIn->ResponseService(pHandle, pBuffer, nLen);
}


void CVirtualClientLoader::GetSelfCheck(vector<SCacheSelfCheck>& vecInfo)
{
	SS_XLOG(XLOG_DEBUG, "CVirtualClientLoader::%s...\n", __FUNCTION__);  
    unsigned dwAll=0, dwAlive=0;
	map<string, SVirtualServcie>::iterator it;
    for(it=m_mapVS.begin(); it!=m_mapVS.end(); ++it)
    {
        SVirtualServcie &service = it->second;
        service.pServiceObj->GetSelfCheckState(dwAlive, dwAll); 
        vecInfo.push_back(SCacheSelfCheck(service.strModule, dwAlive, dwAll-dwAlive));
    }
}



const string CVirtualClientLoader::GetPluginInfo()const
{
	SS_XLOG(XLOG_DEBUG, "CVirtualClientLoader::%s...\n", __FUNCTION__);
    string strVersionInfo("<table width=80% cellspacing=0 cellpadding=0 border=1 align=left>");
    strVersionInfo += "<tr><th>Name</th><th>Version</th><th>BildTime</th></tr>";
	map<string, SVirtualServcie>::const_iterator it;
    for(it=m_mapVS.begin(); it!=m_mapVS.end(); ++it)
    {
        const SVirtualServcie &service = it->second;
        strVersionInfo += service.pServiceObj->OnGetPluginInfo();
    }
    strVersionInfo += "</table>";
    return strVersionInfo;	
}



void CVirtualClientLoader::Dump()
{
	for (map<unsigned int, SVirtualServcie>::iterator iter = mapVirtualServcie.begin();
		iter != mapVirtualServcie.end(); ++iter)
	{
		//SVirtualServcie &service = iter->second;
		SS_XLOG(XLOG_DEBUG, "CVirtualClientLoader::%s,service id[%d]\n", 
			__FUNCTION__, iter->first);
	}
}


