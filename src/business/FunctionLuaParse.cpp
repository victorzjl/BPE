#include "FunctionLuaParse.h"
#include "SapCommon.h"
#include "DirReader.h"
#include "SapLogHelper.h"
#include "ComposeService.h"


using sdo::commonsdk::CDirReader;

extern_fun_lua_manager * extern_fun_lua_manager::sm_pInstance=NULL;

bool extern_fun_lua_manager::find(const string& fun_name)
{
//	lua_State * L=specL.get();
	return false;
}

int extern_fun_lua_manager::call(const string&fun_name,unsigned param_count,vector<inst_value>& stk,int first_param,int type)
{ 
	SS_XLOG(XLOG_DEBUG, "extern_fun_lua_manager::%s\n",__FUNCTION__);
	lua_State * L=specL.get();
	lua_getglobal(L,fun_name.c_str()); 

	unsigned count_i=0;
	for(;count_i<param_count;count_i++)
	{
		if(stk[first_param+count_i].m_type==eiv_string) 
		{
			lua_pushstring(L,stk[first_param+count_i].to_string().c_str());
		}
		else if(stk[first_param+count_i].m_type==eiv_int) 
		{
			lua_pushinteger(L,stk[first_param+count_i].m_int_value);
		}
	}
	int ret = lua_pcall(L,param_count,1,0) ;
	if ( ret != 0 )
	{
	  SS_XLOG(XLOG_DEBUG, "extern_fun_lua_manager::%s, %s\n",__FUNCTION__,lua_tostring(L,-1));
	  if (strcmp(lua_tostring(L,-1),"attempt to call a nil value")==0)
	  {
		 return SAP_CODE_NOT_FOUND;
	  }
	  return SAP_CODE_FUNCALLERROR;  
	}
	if (stk.size() == 0 && param_count==0)
	{
		inst_value tmp;
		stk.push_back(tmp);
	}
	if (type == sdo::commonsdk::MSG_FIELD_STRING)
	{
		stk[first_param] = string(lua_tostring(L,-1));  
	}
	else
	{
		stk[first_param] = lua_tointeger(L,-1); 
	}
    lua_pop(L,1);  
	return 0;
}

void extern_fun_lua_manager::list_function()
{
}
extern_fun_lua_manager * extern_fun_lua_manager::GetInstance()
{
	if(sm_pInstance==NULL) 
		sm_pInstance=new extern_fun_lua_manager();
	return sm_pInstance;
}

void extern_fun_lua_manager::Init()
{
	SS_XLOG(XLOG_DEBUG, "extern_fun_lua_manager::%s\n",__FUNCTION__);
	lua_State* L= luaL_newstate();
	luaL_openlibs(L);
	CDirReader oDirReader("*.lua");
    char szFileName[MAX_PATH] = {0};
	if (oDirReader.OpenDir("./lua_function", true) &&
        oDirReader.GetFirstFilePath(szFileName)) 
	{
        do
        {   
			if (0!=luaL_dofile(L, szFileName))
			{
				SS_XLOG(XLOG_DEBUG, "extern_fun_lua_manager::%s luafilename: %s error: %s\n",__FUNCTION__,szFileName, lua_tostring(L,-1));
			}
			else
			{
				SS_XLOG(XLOG_DEBUG, "extern_fun_lua_manager::%s luafilename: %s OK\n",__FUNCTION__,szFileName);
			}
        }
        while(oDirReader.GetNextFilePath(szFileName));    
	}
	specL.reset(L);
}


