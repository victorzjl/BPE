#include "PluginManager.h"
#include "DirReader.h"
#include "HTDealerServiceLog.h"
#include <dlfcn.h>


CPluginManager* CPluginManager::m_pInstance = new CPluginManager;
CPluginManager::CGarbo CPluginManager::Garbo;

CPluginManager::CPluginManager()
{
	HT_XLOG(XLOG_DEBUG, "CPluginManager::%s\n",__FUNCTION__);
}

CPluginManager::~CPluginManager()
{
	HT_XLOG(XLOG_DEBUG, "CPluginManager::%s\n",__FUNCTION__);
}

int CPluginManager::InitPlugin(const string & strPath)
{	
	HT_XLOG(XLOG_DEBUG, "CPluginManager::%s, strPath[%s]\n", __FUNCTION__, strPath.c_str());
	//printf("CPluginManager::InitPlugin\n");
	if(0 != LoadAllPlugins(strPath))
	{
		HT_XLOG(XLOG_ERROR, "CPluginManager::%s, strPath[%d] load fail\n", __FUNCTION__, strPath.c_str());
		return -1;
	}
	
	return 0;	
}

int CPluginManager::LoadAllPlugins( const string &strPath )
{
	//查找该目录下所有的so文件
	HT_XLOG(XLOG_DEBUG, "CPluginManager::LoadAllPlugins [%s]\n", strPath.c_str());
	//printf("CPluginManager::LoadAllPlugins\n");
	CDirReader oDirReader("*.so");
	if(!oDirReader.OpenDir(strPath.c_str()))
	{
		HT_XLOG(XLOG_ERROR, "CPluginManager::LoadAllPlugins,OpenDir Failed[%s]\n", strPath.c_str());
		return -1;
	}
	
	char szFileName[MAX_PATH] = {0};
	char szServiceConfigFile[MAX_PATH] = {0};
	if (oDirReader.GetFirstFilePath(szFileName)) 
	{
		sprintf(szServiceConfigFile, "%s%s%s", strPath.c_str(), CST_PATH_SEP, szFileName);
		//printf("CPluginManager::LoadAllPlugins--file name%s\n",szServiceConfigFile);
		LoadPlugins(szServiceConfigFile);
		
		while(oDirReader.GetNextFilePath(szFileName))
		{
			memset(szServiceConfigFile, 0, MAX_PATH);			
			sprintf(szServiceConfigFile, "%s%s%s", strPath.c_str(), CST_PATH_SEP, szFileName);
			//printf("CPluginManager::LoadAllPlugins--file name while body%s\n",szServiceConfigFile);			
			LoadPlugins(szServiceConfigFile);
		}
		return 0;
	}
	else 
	{
		HT_XLOG(XLOG_DEBUG, "CPluginManager::LoadAllPlugins,the dir is empty or no file is unfiltered, dir=[%s]\n", strPath.c_str());
		return -1;
	}
}

int CPluginManager::LoadPlugins(const char *szSoName)
{
	HT_XLOG(XLOG_DEBUG, "CPluginManager::%s, szSoName[%s]\n", __FUNCTION__, szSoName);
	//printf("CPluginManager::LoadPlugins\n");
	void *pHandle = NULL;
	if (NULL == (pHandle = dlopen(szSoName, RTLD_NOW)))
	{
		HT_XLOG(XLOG_ERROR,"CPluginManager::%s,dlopen[%s] error[%s]\n", __FUNCTION__, szSoName,dlerror());
		//printf("ERROR[%s]---CPluginManager::LoadPlugins--%s\n",dlerror(),szSoName);
		return -1;
	}  
	
	callFunc call = (callFunc)dlsym(pHandle,"initFunc");
	ssoConfig* sConfig =(ssoConfig*) call();
	
	/*
	*将插件函数进行注册
	*/
	
	if( sConfig->type == 0)
	{
		//0表示发送
		ssoSendConfig* sendConfig =  (ssoSendConfig*)sConfig; 
		( CPluginRegister::GetInstance())->insertSendFunc(sendConfig->name, sendConfig->funcAddr);
	}
	else
	{
		ssoRecvConfig* recvConfig =  (ssoRecvConfig*)sConfig; 
		( CPluginRegister::GetInstance())->insertRecvFunc(recvConfig->name, recvConfig->funcAddr);
		
	}	
	
	char* dlsym_error = dlerror();
    if (dlsym_error)
    {
        HT_XLOG(XLOG_ERROR,"CSoAHTransferAdapter::%s,dlsym create[%s] error[%s]\n",__FUNCTION__,szSoName,dlsym_error);
        //printf("%s\n",dlsym_error);
		return -1;                    
    }
	
	
	
	return 0;
}
