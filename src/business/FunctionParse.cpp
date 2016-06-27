#include "FunctionParse.h"
#include "SapCommon.h"
#include "SapLogHelper.h"
int inst_value::to_int()
{
	if(m_type==eiv_int) return m_int_value;
	//else if(m_type==eiv_float) return (int)m_float_value;
	else if(m_type==eiv_string)
	{
		return atoi(m_string_value.c_str());
	}
	else return 0;
}

//double inst_value::to_float()
//{
//	if(m_type==eiv_int) return m_int_value;
//	else if(m_type==eiv_float) return m_float_value;
//	else if(m_type==eiv_string)
//	{
//		return atof(m_string_value.c_str());
//	}
//	else return 0;
//}

string inst_value::to_string()
{
	if(m_type==eiv_string) return m_string_value;
	else if(m_type==eiv_int)
	{
		char buf[32];
		sprintf(buf,"%d",m_int_value);
		return buf;
	}
	/*else if(m_type==eiv_float)
	{
		char buf[32];
		sprintf(buf,"%10.14f",m_float_value);
		return buf;
	}*/
	return "";
}


template<> weak_param invert_param_to<weak_param>(vector<inst_value>& stk,int index)
{
	return weak_param();
}
template<> int invert_param_to<int>(vector<inst_value>& stk,int index)
{
	return stk[index].to_int();
}
//template<> double  invert_param_to<double>(vector<inst_value>& stk,int index)
//{
//	return stk[index].to_float();
//}
template<> string invert_param_to<string>(vector<inst_value>& stk,int index)
{
	return stk[index].to_string();
}

extern_fun_manager * extern_fun_manager::sm_pInstance=NULL;
bool extern_fun_manager::register_fun(boost::shared_ptr<func_extern> sp_fun)
{
	MMapFunc * pMapThisThread=m_funs.get();
	(*pMapThisThread)[sp_fun->m_fun_name].push_back(sp_fun);
	return true;
}

bool extern_fun_manager::find(const string& fun_name)
{
	MMapFunc * pMapThisThread=m_funs.get();
	map<string,vector<boost::shared_ptr<func_extern> > >::iterator it=pMapThisThread->find(fun_name);
	if(it!=pMapThisThread->end()) return true;
	else return false;
}

int extern_fun_manager::call(const string&fun_name,unsigned param_count,vector<inst_value>& stk,int first_param)
{ 
	MMapFunc * pMapThisThread=m_funs.get();
	map<string,vector<boost::shared_ptr<func_extern> > >::iterator it=pMapThisThread->find(fun_name);
	if(it==pMapThisThread->end()) return SAP_CODE_NOT_FOUND;
	vector<boost::shared_ptr<func_extern> > funs=it->second;
	for(vector<boost::shared_ptr<func_extern> >::iterator it=funs.begin();it!=funs.end();)
	{
		if((*it)->m_params_t.size()!=param_count) it=funs.erase(it);
		else it++;
	}
	for(vector<boost::shared_ptr<func_extern> >::iterator it=funs.begin();it!=funs.end();it++)
	{
		SS_XLOG(XLOG_DEBUG,"extern_fun_manager::%s, fun_name: %s paramcounts: %d\n",__FUNCTION__,fun_name.c_str(),param_count);
		unsigned count_i=0;
		for(;count_i<param_count;count_i++)
		{
			SS_XLOG(XLOG_DEBUG,"extern_fun_manager::%s, fun_name: %s inputtype %d functype %d\n",__FUNCTION__,fun_name.c_str(),stk[first_param+count_i].m_type, (*it)->m_params_t[count_i]);
			
			if(stk[first_param+count_i].m_type!=(*it)->m_params_t[count_i]) break;
		}
		if(count_i==param_count) 
		{
			if (stk.size() == 0 && param_count==0)
			{
				inst_value tmp;
				stk.push_back(tmp);
			}
			(**it)(param_count,stk,first_param);
			return 0;
		}
	}

	return SAP_CODE_FUNCALLERROR;
}

void extern_fun_manager::list_function()
{
	MMapFunc * pMapThisThread=m_funs.get();
	for(map<string,vector<boost::shared_ptr<func_extern> > >::iterator it=pMapThisThread->begin();it!=pMapThisThread->end();it++)
	{
		printf("exten function#[%s]\n",it->first.c_str());
	}
}
extern_fun_manager * extern_fun_manager::GetInstance()
{
	if(sm_pInstance==NULL) 
		sm_pInstance=new extern_fun_manager();
	return sm_pInstance;
}
//extern_fun_manager* funs_manager()
//{
//	return m_extern_fun_manager->get();
//}

void extern_fun_manager::Init()
{
	m_funs.reset(new MMapFunc);
}
//extern_fun_manager* extern_fun_manager::get()
//{
//	return m_fun_manager.get();
//}
