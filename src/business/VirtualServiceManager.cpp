#include "VirtualServiceManager.h"
#include <dlfcn.h>
#include <sched.h>
#include "SapMessage.h"
#include "DirReader.h"
#include "SapLogHelper.h"

#ifdef WIN32
	#define CST_PATH_SEP	"\\"
#else
	#define CST_PATH_SEP	"/"	
#endif

using namespace sdo::common;

const char* CVirtualServiceManager::VIRTUAL_SERVICE_PATH = "./virtual_service";
static const unsigned LOCAL_CACHE_BEGIN = 40900;
static const unsigned LOCAL_CACHE_END   = 40999;
extern void dump_buffer(const string& strPacketInfo, const void *pBuffer);

CVirtualServiceManager::~CVirtualServiceManager()
{
	//ReleaseVirtualService();
}

int CVirtualServiceManager::LoadVirtualService()
{		
	SS_XLOG(XLOG_DEBUG, "CVirtualServiceManager::%s...\n", __FUNCTION__);
	sdo::commonsdk::CDirReader oDirReader("*.so");
	if (!oDirReader.OpenDir(VIRTUAL_SERVICE_PATH, true))
	{
        SS_XLOG(XLOG_DEBUG, "CVirtualServiceManager::%s, open dir error\n",__FUNCTION__);
		return -1;
	} 

	char szFileName[MAX_PATH] = {0};	
	if (oDirReader.GetFirstFilePath(szFileName) == false) 
	{
		SS_XLOG(XLOG_ERROR,"CVirtualServiceManager::%s, GetFirstFilePath error\n", __FUNCTION__);
		return  -1;
	}

    //char szSoObj[MAX_PATH] = {0};
	do
	{ 	   
		//sprintf(szSoObj, "%s%s%s", VIRTUAL_SERVICE_PATH, CST_PATH_SEP, szFileName);
		SVirtualServcie service;
        if(0 == LoadSO(szFileName, service))
    	{   
            const unsigned* id_array = NULL;
            int len = 0;
            service.pServiceObj->GetServiceIds(&id_array, len);                                
            for(int i=0; i<len; ++i)
            {
                SS_XLOG(XLOG_DEBUG, "CVirtualServiceManager::%s id[%u]\n",
                    __FUNCTION__,id_array[i]);
                mapVirtualServcie.insert(make_pair(id_array[i], service));		
            }
            
           //SS_XLOG(XLOG_DEBUG, "CVirtualServiceManager::%s version[%s]\n",
             //           __FUNCTION__,version.c_str());
            m_mapNameVService.insert(make_pair(szFileName,service));
        }
	}
	while(oDirReader.GetNextFilePath(szFileName));
	return 0;
}

int CVirtualServiceManager::LoadServiceByName(const string &strName)
{		
	SS_XLOG(XLOG_DEBUG, "CVirtualServiceManager::%s lib[%s]\n",__FUNCTION__,strName.c_str());
    /*boost::unordered_map<string,SVirtualServcie>::iterator itr = m_mapNameVService.find(strName);
    if(itr != m_mapNameVService.end())
    {
        SS_XLOG(XLOG_WARNING, "CVirtualServiceManager::%s lib[%s] has loaded\n",__FUNCTION__,strName.c_str());
        return -2;
    }
	char szSoObj[MAX_PATH] = {0};
    sprintf(szSoObj, "%s%s%s", VIRTUAL_SERVICE_PATH, CST_PATH_SEP, strName.c_str());
    SVirtualServcie service;
	if(0 != LoadSO(szSoObj, service))
	{
        return -1;
	}
	//mapVirtualServcie[service.pServiceObj->GetServiceId()] = service;		
    m_mapNameVService[strName] = service;*/
	return 0;
}

int CVirtualServiceManager::LoadSO(const char* pSoObj,SVirtualServcie &service)
{
    SS_XLOG(XLOG_DEBUG, "CVirtualServiceManager::%s path[%s]\n",__FUNCTION__,pSoObj);
    void *p_lib = dlopen(pSoObj, RTLD_LAZY);
	if(p_lib == NULL)
	{
		SS_XLOG(XLOG_ERROR, "CVirtualServiceManager::%s, dlopen[%s] error[%s]\n", __FUNCTION__,pSoObj,dlerror());
		return -1;
	}

	service.pLibHandle = p_lib;
	service.fnCreateObj = (FnCreateObj)dlsym(p_lib, "create");
	char* dlsym_error = dlerror();
	if (dlsym_error)
	{
		dlclose(p_lib);
		SS_XLOG(XLOG_ERROR,"CVirtualServiceManager::%s, dlsym create[%s] error[%s]\n", __FUNCTION__, pSoObj, dlsym_error);
		return -1;					 
	}

	service.fnDestroyObj = (FnDestroyObj)dlsym(p_lib, "destroy");
	dlsym_error = dlerror();
	if (dlsym_error) 
	{
		dlclose(p_lib);
		SS_XLOG(XLOG_ERROR,"CVirtualServiceManager::%s,dlsym destroy[%s] error[%s]\n", __FUNCTION__, pSoObj, dlsym_error);
		return -1;
	}

	service.pServiceObj = service.fnCreateObj();
	if(service.pServiceObj  == NULL)
	{
		SS_XLOG(XLOG_ERROR, "CVirtualServiceManager::%s, create object failed\n", __FUNCTION__);
		return -1;
	}

	SS_XLOG(XLOG_DEBUG, "CVirtualServiceManager::%s create destroy[%x], obj[%x], lib[%x]\n",
		__FUNCTION__, service.fnDestroyObj, service.pServiceObj, p_lib); 
    return 0;
}



int CVirtualServiceManager::ReleaseServiceByName(const string& strName)
{
	SS_XLOG(XLOG_DEBUG, "CVirtualServiceManager::%s...\n", __FUNCTION__);   
    /*boost::unordered_map<string,SVirtualServcie>::iterator itr = m_mapNameVService.find(strName);
    if(itr != m_mapNameVService.end())
    {
        SVirtualServcie &service = itr->second;
        //int nServiceId = service.pServiceObj->GetServiceId();
		service.fnDestroyObj(service.pServiceObj);
		int nRet = dlclose(service.pLibHandle);        
        m_mapNameVService.erase(itr);
        //mapVirtualServcie.erase(mapVirtualServcie.find(nServiceId));
        return nRet;
    }*/
    return 1;
}


int CVirtualServiceManager::ReleaseVirtualService()
{
	SS_XLOG(XLOG_DEBUG, "CVirtualServiceManager::%s...\n", __FUNCTION__);
	int nRet = -1;
	for (map<unsigned int, SVirtualServcie>::iterator iter = mapVirtualServcie.begin();
		iter != mapVirtualServcie.end(); ++iter)
	{
		SVirtualServcie &service = iter->second;
		service.fnDestroyObj(service.pServiceObj);
		nRet = dlclose(service.pLibHandle);        
	}
	mapVirtualServcie.clear();
    m_mapNameVService.clear();
    return nRet;
}

int CVirtualServiceManager::OnRequestService(unsigned int nServiceId, const void *pInPacket, unsigned int nLen,
	void **ppOutPacket, int *pLen)
{
	SS_XLOG(XLOG_DEBUG, "CVirtualServiceManager::%s,serviceid[%d]\n", __FUNCTION__, nServiceId);	
	map<unsigned int, SVirtualServcie>::iterator iter = mapVirtualServcie.find(nServiceId);
	if (iter != mapVirtualServcie.end())
	{
		SVirtualServcie &service = iter->second;		         
		return service.pServiceObj->RequestService(pInPacket, nLen, ppOutPacket, pLen);
        
	}
	SS_XLOG(XLOG_WARNING, "CVirtualServiceManager::%s,serviceid not found\n", __FUNCTION__);
	return -2;
}

bool CVirtualServiceManager::IsVirtualService(unsigned int nServiceId)
{
	SS_XLOG(XLOG_DEBUG, "CVirtualServiceManager::%s,serviceid[%d]\n", __FUNCTION__, nServiceId);
	return mapVirtualServcie.find(nServiceId) != mapVirtualServcie.end();
}

void CVirtualServiceManager::SetData(const void *pBuffer, unsigned int nLen)
{
    SS_XLOG(XLOG_DEBUG, "CVirtualServiceManager::%s buffer[%x] len[%u]\n", 
        __FUNCTION__, pBuffer, nLen);  
    dump_buffer("virturl service set data\n", pBuffer);
    CSapDecoder msgDecode(pBuffer, nLen);
    unsigned serviceId = msgDecode.GetServiceId();
    map<unsigned int, SVirtualServcie>::iterator iter = mapVirtualServcie.find(serviceId);
	if (iter == mapVirtualServcie.end())
	{
		SS_XLOG(XLOG_WARNING, "CVirtualServiceManager::%s, serviceid[%u] not found\n", __FUNCTION__,serviceId);
        return;        
	}
    SVirtualServcie &service = iter->second;		         
	service.pServiceObj->SetData(pBuffer, nLen); 
	
}

const string CVirtualServiceManager::OnGetPluginInfo()const
{
	SS_XLOG(XLOG_DEBUG, "CVirtualServiceManager::%s...\n", __FUNCTION__);
    string strVersionInfo("<table width=80% cellspacing=0 cellpadding=0 border=1 align=left>");
    strVersionInfo += "<tr><th>Name</th><th>Version</th><th>BildTime</th></tr>";
	map<unsigned int, SVirtualServcie>::const_iterator iter;
    for(iter=mapVirtualServcie.begin(); iter!=mapVirtualServcie.end(); ++iter)
    {
        const SVirtualServcie &service = iter->second;
        strVersionInfo += service.pServiceObj->OnGetPluginInfo();
    }
    strVersionInfo += "</table>";
    return strVersionInfo;	
}

void CVirtualServiceManager::Dump()
{
	for (map<unsigned int, SVirtualServcie>::iterator iter = mapVirtualServcie.begin();
		iter != mapVirtualServcie.end(); ++iter)
	{
      
		SVirtualServcie &service = iter->second;
        //const string& version = service.pServiceObj->OnGetPluginInfo();
		SS_XLOG(XLOG_DEBUG, "CVirtualServiceManager::%s,service id[%u:%x]\n", 
			__FUNCTION__, iter->first, service.pServiceObj);
	}
}


/*
IVirtualService *CVirtualServiceManager::GetService(unsigned int nServiceId)
{
	map<unsigned int, SVirtualServcie>::iterator iter = mapVirtualServcie.find(nServiceId);
	if (iter != mapVirtualServcie.end())
	{			 
		return iter->second.pServiceObj;  
	}
	return NULL;
}
*/

