#ifndef __FUNCTION_LUA_PARSE_H__
#define __FUNCTION_LUA_PARSE_H__
#include "FunctionParse.h"
extern "C" {  
	#include "lua.h"  
	#include "lualib.h"  
	#include "lauxlib.h"  
}  

class extern_fun_lua_manager
{
public:
	static extern_fun_lua_manager * GetInstance();
	bool find(const string& fun_name);
	int call(const string&fun_name,unsigned param_count,vector<inst_value>& stk,int first_param, int type);
	void list_function();
	void Init();
private:
	static extern_fun_lua_manager * sm_pInstance;
	boost::thread_specific_ptr<lua_State> specL;
};

#endif

