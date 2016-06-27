#include "DbDataHandler.h"
#include <dlfcn.h>
#include <sched.h>

#include "DirReader.h"
#include "SapLogHelper.h"
#include "IConfigureService.h"
#include "SapStack.h"

#ifdef WIN32
	#define CST_PATH_SEP	"\\"
#else
	#define CST_PATH_SEP	"/"	
#endif

using namespace sdo::common;

const char* CDbDataHandler::SERVICE_PATH = "./db_config";

void SetConfig(const void* buffer, int len)
{
    CSapStack::Instance()->SetAllConfig(buffer, len);
}

CDbDataHandler* CDbDataHandler::pInstance_ = NULL;
CDbDataHandler* CDbDataHandler::GetInstance()
{
    if (pInstance_ == NULL)
    {
        pInstance_ = new CDbDataHandler();
    }
    return pInstance_;
}

int CDbDataHandler::LoadPlugins()
{		
	SS_XLOG(XLOG_DEBUG, "CDbDataHandler::%s...\n", __FUNCTION__);
	sdo::commonsdk::CDirReader oDirReader("*.so");
	if (!oDirReader.OpenDir(SERVICE_PATH, true))
	{
        SS_XLOG(XLOG_DEBUG, "CDbDataHandler::%s, open dir error\n",__FUNCTION__);
		return -1;
	} 

	char szFileName[MAX_PATH] = {0};	
	if (oDirReader.GetFirstFilePath(szFileName) == false) 
	{
		SS_XLOG(XLOG_ERROR,"CDbDataHandler::%s, GetFirstFilePath error\n", __FUNCTION__);
		return  -1;
	}
    
    //char szSoObj[MAX_PATH] = {0};
	do
	{ 	   
		//snprintf(szSoObj, sizeof(szSoObj),"%s%s%s", SERVICE_PATH, CST_PATH_SEP, szFileName);
		SConfServcie service;
        if(0 == LoadSO(szFileName, service))
    	{
            m_confServcies.push_back(service);
    	} 		
	}
	while(oDirReader.GetNextFilePath(szFileName));
	return 0;
}


int CDbDataHandler::LoadSO(const char* pSoObj,SConfServcie &service)
{
    SS_XLOG(XLOG_DEBUG, "CDbDataHandler::%s path[%s]\n",__FUNCTION__,pSoObj);
    void *p_lib = dlopen(pSoObj, RTLD_LAZY|RTLD_GLOBAL);
	if(p_lib == NULL)
	{
		SS_XLOG(XLOG_ERROR, "CDbDataHandler::%s, dlopen[%s] error[%s]\n", __FUNCTION__,pSoObj,dlerror());
		return -1;
	}

	service.pLibHandle = p_lib;
	service.fnCreateObj = (FnCreateObj)dlsym(p_lib, "create");
	char* dlsym_error = dlerror();
	if (dlsym_error)
	{
		dlclose(p_lib);
		SS_XLOG(XLOG_ERROR,"CDbDataHandler::%s, dlsym create[%s] error[%s]\n", __FUNCTION__, pSoObj, dlsym_error);
		return -1;					 
	}

	service.fnDestroyObj = (FnDestroyObj)dlsym(p_lib, "destroy");
	dlsym_error = dlerror();
	if (dlsym_error) 
	{
		dlclose(p_lib);
		SS_XLOG(XLOG_ERROR,"CDbDataHandler::%s,dlsym destroy[%s] error[%s]\n", __FUNCTION__, pSoObj, dlsym_error);
		return -1;
	}

	service.pServiceObj = service.fnCreateObj();
	if(service.pServiceObj  == NULL)
	{
		SS_XLOG(XLOG_ERROR, "CDbDataHandler::%s, create object failed\n", __FUNCTION__);
		return -1;
	}
    
    if (service.pServiceObj->Init(SetConfig) != 0)
    {
        SS_XLOG(XLOG_ERROR, "CDbDataHandler::%s, init failed\n", __FUNCTION__);
		return -1;
    }

	SS_XLOG(XLOG_DEBUG, "CDbDataHandler::%s create destroy[%x], obj[%x], lib[%x]\n",
		__FUNCTION__, service.fnDestroyObj, service.pServiceObj, p_lib); 
    return 0;
}


int CDbDataHandler::LoadData()const
{
    SS_XLOG(XLOG_DEBUG, "CDbDataHandler::%s...\n", __FUNCTION__);	
    int nRet = 0;
	for(plugin_citerator it=m_confServcies.begin(); it!=m_confServcies.end(); ++it)
	{
        const SConfServcie& conf = *it;
        nRet += conf.pServiceObj->Load();
	}
    return nRet;
}


int CDbDataHandler::FastLoadData()const
{
	SS_XLOG(XLOG_DEBUG, "CDbDataHandler::%s...\n", __FUNCTION__);	
    int ret = 0;
	for(plugin_citerator it=m_confServcies.begin(); it!=m_confServcies.end(); ++it)
	{
        const SConfServcie& conf = *it;
        ret += conf.pServiceObj->FastLoad();
	}
    return ret;
}



void CDbDataHandler::Dump()
{/*
	for (map<unsigned int, SVirtualServcie>::iterator iter = mapVirtualServcie.begin();
		iter != mapVirtualServcie.end(); ++iter)
	{
		SVirtualServcie &service = iter->second;
		SS_XLOG(XLOG_DEBUG, "CDbDataHandler::%s,service id[%d:%d]\n", 
			__FUNCTION__, iter->first, service.pServiceObj->GetServiceId());
	}*/
}


