#include "CPPFunctionManager.h"
#include <dlfcn.h>
#include <sched.h>
#include "DirReader.h"
#include "SapLogHelper.h"
#include "StateMachine.h"
#ifdef WIN32
	#define CST_PATH_SEP	"\\"
#else
	#define CST_PATH_SEP	"/"	
#endif

using namespace sdo::common;

const char* CCPPFunctionManager::CPP_FUNCTION_PATH = "./cpp_function";

CCPPFunctionManager::CCPPFunctionManager()
{
	//m_extern_fun_manager=new extern_fun_manager();
}
CCPPFunctionManager::~CCPPFunctionManager()
{
	//delete m_extern_fun_manager;
}
int CCPPFunctionManager::LoadExecFunction()
{		
	SS_XLOG(XLOG_DEBUG, "CCPPFunctionManager::%s...\n", __FUNCTION__);
	
	extern_fun_manager::GetInstance()->Init();
	extern_fun_lua_manager::GetInstance()->Init();
	CStateMachine::Instance()->LoadFiles("./state_conf/");
	
	sdo::commonsdk::CDirReader oDirReader("*.so");
	if (!oDirReader.OpenDir(CPP_FUNCTION_PATH, true))
	{
        SS_XLOG(XLOG_DEBUG, "CCPPFunctionManager::%s, open dir error\n",__FUNCTION__);
		return -1;
	} 

	char szFileName[MAX_PATH] = {0};	
	if (oDirReader.GetFirstFilePath(szFileName) == false) 
	{
		SS_XLOG(XLOG_ERROR,"CCPPFunctionManager::%s, GetFirstFilePath error\n", __FUNCTION__);
		return  -1;
	}
	
	do
	{
		LoadSO(szFileName);
	}
	while(oDirReader.GetNextFilePath(szFileName));
	return 0;
}

int CCPPFunctionManager::LoadSO(const char* pSoObj)
{
    SS_XLOG(XLOG_DEBUG, "CCPPFunctionManager::%s path[%s]\n",__FUNCTION__,pSoObj);
    void *p_lib = dlopen(pSoObj, RTLD_LAZY);
	if(p_lib == NULL)
	{
		SS_XLOG(XLOG_ERROR, "CCPPFunctionManager::%s, dlopen[%s] error[%s]\n", __FUNCTION__,pSoObj,dlerror());
		return -1;
	}
	FnCreateObj fnCreateObj = (FnCreateObj)dlsym(p_lib, "create");
	char* dlsym_error = dlerror();
	if (dlsym_error)
	{
		dlclose(p_lib);
		SS_XLOG(XLOG_ERROR,"CCPPFunctionManager::%s, dlsym create[%s] error[%s]\n", __FUNCTION__, pSoObj, dlsym_error);
		return -1;					 
	}
	fnCreateObj();
	SS_XLOG(XLOG_DEBUG, "CCPPFunctionManager::%s lib[%x]\n",__FUNCTION__, p_lib); 
    return 0;
}
