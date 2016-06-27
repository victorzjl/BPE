#include "FunctionBuiltInParse.h"
#include "SapCommon.h"
#include "DirReader.h"
#include "SapLogHelper.h"
#include "ComposeService.h"

using sdo::commonsdk::CDirReader;

extern_fun_builtin_manager * extern_fun_builtin_manager::sm_pInstance=NULL;
bool extern_fun_builtin_manager::find(const string& fun_name)
{
	SS_XLOG(XLOG_DEBUG, "extern_fun_builtin_manager::%s\n",__FUNCTION__);
	if (strcasecmp(fun_name.c_str(),"ExcuteStateMachine")==0)
	{
		return true;
	}
	return false;
}

int extern_fun_builtin_manager::call(const string&fun_name,unsigned param_count,vector<inst_value>& stk,int first_param,int type)
{ 
	SS_XLOG(XLOG_DEBUG, "extern_fun_builtin_manager::%s\n",__FUNCTION__);
	if (!find(fun_name))
	{
		return SAP_CODE_NOT_FOUND;
	}
	if (strcasecmp(fun_name.c_str(),"ExcuteStateMachine")==0)
	{
		return DoStateEvent(fun_name,param_count,stk,first_param,type);
	}
	return SAP_CODE_NOT_FOUND;
}

int	extern_fun_builtin_manager::DoStateEvent(const string&fun_name,unsigned param_count,vector<inst_value>& stk,int first_param,int type)
{
	if (param_count!=3)
	{
		return SAP_CODE_FUNCALLERROR;
	}
	if(stk[first_param+0].m_type!=eiv_string||stk[first_param+1].m_type!=eiv_int||stk[first_param+2].m_type!=eiv_int)
	{
		return SAP_CODE_FUNCALLERROR;
	}
	string strMachineName = stk[first_param+0].to_string();
	int nState = stk[first_param+1].m_int_value;
	int nEvent = stk[first_param+2].m_int_value;
	if (type == sdo::commonsdk::MSG_FIELD_INT)
	{
		stk[first_param] = CStateMachine::Instance()->ExcuteStateMachine(strMachineName, nState, nEvent); 
	}
	else
	{
		return SAP_CODE_FUNCALLERROR;
	}
	return 0;
}

void extern_fun_builtin_manager::list_function()
{
}

extern_fun_builtin_manager * extern_fun_builtin_manager::GetInstance()
{
	if(sm_pInstance==NULL) 
		sm_pInstance=new extern_fun_builtin_manager();
	return sm_pInstance;
}

void extern_fun_builtin_manager::Init()
{
	SS_XLOG(XLOG_DEBUG, "extern_fun_builtin_manager::%s\n",__FUNCTION__);
	CStateMachine::Instance()->LoadFiles("./state_conf/");
}


