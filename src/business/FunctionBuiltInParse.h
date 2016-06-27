#ifndef __FUNCTION_BUILTIN_PARSE_H__
#define __FUNCTION_BUILTIN_PARSE_H__
#include "FunctionParse.h"
#include "StateMachine.h"

class extern_fun_builtin_manager
{
public:
	static extern_fun_builtin_manager * GetInstance();
	bool find(const string& fun_name);
	int  call(const string&fun_name,unsigned param_count,vector<inst_value>& stk,int first_param, int type);
	void list_function();
	void Init();
private:
	static extern_fun_builtin_manager * sm_pInstance;
private:
	int DoStateEvent(const string&fun_name,unsigned param_count,vector<inst_value>& stk,int first_param,int type);
};

#endif

