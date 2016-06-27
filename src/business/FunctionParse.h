#ifndef __FUNCTION_PARSE_H__
#define __FUNCTION_PARSE_H__
#include <boost/shared_ptr.hpp>
#include <string>
#include <vector>
#include <map>
#include <boost/algorithm/string.hpp>
#include <boost/thread/tss.hpp>
using std::string;
using std::vector;
using std::map;

enum e_inst_value_t
{
	eiv_invalid,
	eiv_int,
	eiv_string
};
class inst_value
{
public:
	inst_value(int value=0):m_type(eiv_int),m_int_value(value){}
	//inst_value(double value):m_type(eiv_float),m_int_value(0),m_float_value(value){}
	inst_value(const string& value):m_type(eiv_string),m_int_value(0),m_string_value(value){}
	int to_int();
	//double to_float();
	string to_string();

	e_inst_value_t m_type;
	int m_int_value;
	//double m_float_value;
	string m_string_value;
};

class func_extern
{
public:
	func_extern(const string& fun_name,e_inst_value_t return_t=eiv_invalid)
		:m_fun_name(fun_name),m_return_t(return_t){}

	virtual void operator()(int param_count,vector<inst_value>& stk,int first_param)=0;
	const string m_fun_name;
	e_inst_value_t m_return_t;
	vector<e_inst_value_t> m_params_t;
	virtual ~func_extern(){}
};

class weak_param{};
template<typename T> T invert_param_to(vector<inst_value>& stk,int index=0);

template<typename  T>struct type_to_eiv
{
	static const e_inst_value_t eiv=eiv_invalid;
};
template<> struct type_to_eiv<int>
{
	static const e_inst_value_t eiv=eiv_int;
};
//template<> struct type_to_eiv<double>
//{
//	static const e_inst_value_t eiv=eiv_float;
//};
template<> struct type_to_eiv<string>
{
	static const e_inst_value_t eiv=eiv_string;
};


template<typename ResultT,
typename P1=weak_param,typename P2=weak_param,
typename P3=weak_param,typename P4=weak_param,
typename P5=weak_param,typename P6=weak_param,
typename P7=weak_param,typename P8=weak_param>
class func_extern_impl :public func_extern
{
public:
	typedef ResultT (*fun0_t)() ;
	typedef ResultT (*fun1_t)(P1) ;
	typedef ResultT (*fun2_t)(P1,P2) ;
	typedef ResultT (*fun3_t)(P1,P2,P3) ;
	typedef ResultT (*fun4_t)(P1,P2,P3,P4) ;
	typedef ResultT (*fun5_t)(P1,P2,P3,P4,P5) ;
	typedef ResultT (*fun6_t)(P1,P2,P3,P4,P5,P6) ;
	typedef ResultT (*fun7_t)(P1,P2,P3,P4,P5,P6,P7) ;
	typedef ResultT (*fun8_t)(P1,P2,P3,P4,P5,P6,P7,P8) ;
private:
	int m_fun_index;
	union{
		fun0_t _0;
		fun1_t _1;
		fun2_t _2;
		fun3_t _3;
		fun4_t _4;
		fun5_t _5;
		fun6_t _6;
		fun7_t _7;
		fun8_t _8;
	}m_fun;
public:
	void init_params()
	{
		m_return_t=type_to_eiv<ResultT>::eiv;
		e_inst_value_t tmp_inst_value;
		if((tmp_inst_value=type_to_eiv<P1>::eiv)!=eiv_invalid) m_params_t.push_back(tmp_inst_value);
		if((tmp_inst_value=type_to_eiv<P2>::eiv)!=eiv_invalid) m_params_t.push_back(tmp_inst_value);
		if((tmp_inst_value=type_to_eiv<P3>::eiv)!=eiv_invalid) m_params_t.push_back(tmp_inst_value);
		if((tmp_inst_value=type_to_eiv<P4>::eiv)!=eiv_invalid) m_params_t.push_back(tmp_inst_value);
		if((tmp_inst_value=type_to_eiv<P5>::eiv)!=eiv_invalid) m_params_t.push_back(tmp_inst_value);
		if((tmp_inst_value=type_to_eiv<P6>::eiv)!=eiv_invalid) m_params_t.push_back(tmp_inst_value);
		if((tmp_inst_value=type_to_eiv<P7>::eiv)!=eiv_invalid) m_params_t.push_back(tmp_inst_value);
		if((tmp_inst_value=type_to_eiv<P8>::eiv)!=eiv_invalid) m_params_t.push_back(tmp_inst_value);
	}

#define FUNC_EXTERN_IMPL_CONSTRUCTOR(num) \
	func_extern_impl(const string& fun_name,fun##num##_t pf) \
	:func_extern(fun_name) \
	{ \
	m_fun._##num=pf;\
	m_fun_index=num;\
	init_params();\
	}
	FUNC_EXTERN_IMPL_CONSTRUCTOR(0);
	FUNC_EXTERN_IMPL_CONSTRUCTOR(1);
	FUNC_EXTERN_IMPL_CONSTRUCTOR(2);
	FUNC_EXTERN_IMPL_CONSTRUCTOR(3);
	FUNC_EXTERN_IMPL_CONSTRUCTOR(4);
	FUNC_EXTERN_IMPL_CONSTRUCTOR(5);
	FUNC_EXTERN_IMPL_CONSTRUCTOR(6);
	FUNC_EXTERN_IMPL_CONSTRUCTOR(7);
	FUNC_EXTERN_IMPL_CONSTRUCTOR(8);


	template<typename ResultTT,typename P1T,typename P2T,typename P3T,typename P4T,typename P5T,typename P6T,typename P7T,typename P8T>
	ResultTT call_wrap(P1T p1,P2T p2,P3T p3,P4T p4,P5T p5,P6T p6,P7T p7,P8T p8)
	{
		if(m_fun_index==0) return m_fun._0();
		if(m_fun_index==1) return m_fun._1(p1);
		if(m_fun_index==2) return m_fun._2(p1,p2);
		if(m_fun_index==3) return m_fun._3(p1,p2,p3);
		if(m_fun_index==4) return m_fun._4(p1,p2,p3,p4);
		if(m_fun_index==5) return m_fun._5(p1,p2,p3,p4,p5);
		if(m_fun_index==6) return m_fun._6(p1,p2,p3,p4,p5,p6);
		if(m_fun_index==7) return m_fun._7(p1,p2,p3,p4,p5,p6,p7);
		if(m_fun_index==8) return m_fun._8(p1,p2,p3,p4,p5,p6,p7,p8);
		return ResultTT();
	}

	virtual void operator()(int param_count,vector<inst_value>& stk,int first_param)
	{
		stk[first_param] =call_wrap<ResultT>(
			invert_param_to<P1>(stk,first_param),
			invert_param_to<P2>(stk,first_param+1),
			invert_param_to<P3>(stk,first_param+2),
			invert_param_to<P4>(stk,first_param+3),
			invert_param_to<P5>(stk,first_param+4),
			invert_param_to<P6>(stk,first_param+5),
			invert_param_to<P7>(stk,first_param+6),
			invert_param_to<P8>(stk,first_param+7)
			);
	}
};
class extern_fun_manager
{
	typedef map<string,vector<boost::shared_ptr<func_extern> > > MMapFunc;
public:
	static extern_fun_manager * GetInstance();
	bool register_fun(boost::shared_ptr<func_extern> sp_fun);
	bool find(const string& fun_name);
	int call(const string&fun_name,unsigned param_count,vector<inst_value>& stk,int first_param);
	void list_function();

	void Init();
	//extern_fun_manager* get();
private:
	static extern_fun_manager * sm_pInstance;
	boost::thread_specific_ptr<MMapFunc> m_funs;
	//map<string,vector<boost::shared_ptr<func_extern> > > m_funs;
	//boost::thread_specific_ptr<extern_fun_manager> m_fun_manager;
};

//static extern_fun_manager* m_extern_fun_manager;
//extern_fun_manager* funs_manager();

/*register function*/
template<typename ResultT>
void register_function(const string&fun_name,ResultT (*pf)())
{
	extern_fun_manager::GetInstance()->register_fun(boost::shared_ptr<func_extern_impl<ResultT> >(new func_extern_impl<ResultT>(fun_name,pf)));
}
template<typename ResultT,typename P1>
void register_function(const string&fun_name, ResultT (*pf)(P1))
{
	extern_fun_manager::GetInstance()->register_fun(boost::shared_ptr<func_extern_impl<ResultT,P1> >(new func_extern_impl<ResultT,P1>(fun_name,pf)));
}
template<typename ResultT,typename P1,typename P2>
void register_function(const string&fun_name, ResultT (*pf)(P1,P2))
{
	extern_fun_manager::GetInstance()->register_fun(boost::shared_ptr<func_extern_impl<ResultT,P1,P2> >(new func_extern_impl<ResultT,P1,P2>(fun_name,pf)));
}
template<typename ResultT,typename P1,typename P2,typename P3>
void register_function(const string&fun_name, ResultT (*pf)(P1,P2,P3))
{
	extern_fun_manager::GetInstance()->register_fun(boost::shared_ptr<func_extern_impl<ResultT,P1,P2,P3> >(new func_extern_impl<ResultT,P1,P2,P3>(fun_name,pf)));
}
template<typename ResultT,typename P1,typename P2,typename P3,typename P4>
void register_function(const string&fun_name, ResultT (*pf)(P1,P2,P3,P4))
{
	extern_fun_manager::GetInstance()->register_fun(boost::shared_ptr<func_extern_impl<ResultT,P1,P2,P3,P4> >(new func_extern_impl<ResultT,P1,P2,P3,P4>(fun_name,pf)));
}
template<typename ResultT,typename P1,typename P2,typename P3,typename P4,typename P5>
void register_function(const string&fun_name, ResultT (*pf)(P1,P2,P3,P4,P5))
{
	extern_fun_manager::GetInstance()->register_fun(boost::shared_ptr<func_extern_impl<ResultT,P1,P2,P3,P4,P5> >(new func_extern_impl<ResultT,P1,P2,P3,P4,P5>(fun_name,pf)));
}
template<typename ResultT,typename P1,typename P2,typename P3,typename P4,typename P5,typename P6>
void register_function(const string&fun_name, ResultT (*pf)(P1,P2,P3,P4,P5,P6))
{
	extern_fun_manager::GetInstance()->register_fun(boost::shared_ptr<func_extern_impl<ResultT,P1,P2,P3,P4,P5,P6> >(new func_extern_impl<ResultT,P1,P2,P3,P4,P5,P6>(fun_name,pf)));
}
template<typename ResultT,typename P1,typename P2,typename P3,typename P4,typename P5,typename P6,typename P7>
void register_function(const string&fun_name, ResultT (*pf)(P1,P2,P3,P4,P5,P6,P7))
{
	extern_fun_manager::GetInstance()->register_fun(boost::shared_ptr<func_extern_impl<ResultT,P1,P2,P3,P4,P5,P6,P7> >(new func_extern_impl<ResultT,P1,P2,P3,P4,P5,P6,P7>(fun_name,pf)));
}
template<typename ResultT,typename P1,typename P2,typename P3,typename P4,typename P5,typename P6,typename P7,typename P8>
void register_function(const string&fun_name, ResultT (*pf)(P1,P2,P3,P4,P5,P6,P7,P8))
{
	extern_fun_manager::GetInstance()->register_fun(boost::shared_ptr<func_extern_impl<ResultT,P1,P2,P3,P4,P5,P6,P7,P8> >(new func_extern_impl<ResultT,P1,P2,P3,P4,P5,P6,P7,P8>(fun_name,pf)));
}

/*call function*/
template<typename ResultT>
int call_function(const string&fun_name, ResultT &t)
{
	vector<inst_value> stk;
	stk.push_back(inst_value());
	int bResult=extern_fun_manager::GetInstance()->call(fun_name,0,stk,0);
	if(bResult==0) t=invert_param_to<ResultT>(stk,0);
	return bResult;
}
template<typename ResultT,typename P1>
int call_function(const string&fun_name, ResultT &t,P1 p1)
{
	vector<inst_value> stk;
	stk.push_back(inst_value(p1));
	int bResult=extern_fun_manager::GetInstance()->call(fun_name,1,stk,0);
	if(bResult==0) t=invert_param_to<ResultT>(stk,0);
	return bResult;
}
template<typename ResultT,typename P1,typename P2>
int call_function(const string&fun_name, ResultT &t,P1 p1, P2 p2)
{
	vector<inst_value> stk;
	stk.push_back(inst_value(p1));
	stk.push_back(inst_value(p2));
	int bResult=extern_fun_manager::GetInstance()->call(fun_name,2,stk,0);
	if(bResult==0) t=invert_param_to<ResultT>(stk,0);
	return bResult;
}
template<typename ResultT,typename P1,typename P2,typename P3>
int call_function(const string&fun_name, ResultT &t,P1 p1,P2 p2, P3 p3)
{
	vector<inst_value> stk;
	stk.push_back(inst_value(p1));
	stk.push_back(inst_value(p2));
	stk.push_back(inst_value(p3));
	int bResult=extern_fun_manager::GetInstance()->call(fun_name,3,stk,0);
	if(bResult==0) t=invert_param_to<ResultT>(stk,0);
	return bResult;
}
template<typename ResultT,typename P1,typename P2,typename P3,typename P4>
int call_function(const string&fun_name, ResultT &t,P1 p1, P2 p2, P3 p3,P4 p4)
{
	vector<inst_value> stk;
	stk.push_back(inst_value(p1));
	int bResult=extern_fun_manager::GetInstance()->call(fun_name,4,stk,0);
	if(bResult==0) t=invert_param_to<ResultT>(stk,0);
	return bResult;
}
template<typename ResultT,typename P1,typename P2,typename P3,typename P4,typename P5>
int call_function(const string&fun_name, ResultT &t,P1 p1, P2 p2, P3 p3,P4 p4, P5 p5)
{
	vector<inst_value> stk;
	stk.push_back(inst_value(p1));
	stk.push_back(inst_value(p2));
	stk.push_back(inst_value(p3));
	stk.push_back(inst_value(p4));
	stk.push_back(inst_value(p5));
	int bResult=extern_fun_manager::GetInstance()->call(fun_name,5,stk,0);
	if(bResult==0) t=invert_param_to<ResultT>(stk,0);
	return bResult;
}
template<typename ResultT,typename P1,typename P2,typename P3,typename P4,typename P5,typename P6>
int call_function(const string&fun_name, ResultT &t,P1 p1, P2 p2, P3 p3,P4 p4, P5 p5, P6 p6)
{
	vector<inst_value> stk;
	stk.push_back(inst_value(p1));
	stk.push_back(inst_value(p2));
	stk.push_back(inst_value(p3));
	stk.push_back(inst_value(p4));
	stk.push_back(inst_value(p5));
	stk.push_back(inst_value(p6));
	int bResult=extern_fun_manager::GetInstance()->call(fun_name,6,stk,0);
	if(bResult==0) t=invert_param_to<ResultT>(stk,0);
	return bResult;
}
template<typename ResultT,typename P1,typename P2,typename P3,typename P4,typename P5,typename P6,typename P7>
int call_function(const string&fun_name, ResultT &t,P1 p1, P2 p2, P3 p3,P4 p4, P5 p5, P6 p6, P7 p7)
{
	vector<inst_value> stk;
	stk.push_back(inst_value(p1));
	stk.push_back(inst_value(p2));
	stk.push_back(inst_value(p3));
	stk.push_back(inst_value(p4));
	stk.push_back(inst_value(p5));
	stk.push_back(inst_value(p6));
	stk.push_back(inst_value(p7));
	int bResult=extern_fun_manager::GetInstance()->call(fun_name,7,stk,0);
	if(bResult==0) t=invert_param_to<ResultT>(stk,0);
	return bResult;
}
template<typename ResultT,typename P1,typename P2,typename P3,typename P4,typename P5,typename P6,typename P7,typename P8>
int call_function(const string&fun_name, ResultT &t,P1 p1, P2 p2, P3 p3,P4 p4, P5 p5, P6 p6, P7 p7, P8 p8)
{
	vector<inst_value> stk;
	stk.push_back(inst_value(p1));
	stk.push_back(inst_value(p2));
	stk.push_back(inst_value(p3));
	stk.push_back(inst_value(p4));
	stk.push_back(inst_value(p5));
	stk.push_back(inst_value(p6));
	stk.push_back(inst_value(p7));
	stk.push_back(inst_value(p8));
	int bResult=extern_fun_manager::GetInstance()->call(fun_name,8,stk,0);
	if(bResult==0) t=invert_param_to<ResultT>(stk,0);
	return bResult;
}

//string fstr_append(string str1,string str2);
//int f_sum(int int1,int int2,int int3);
//string f_ftoa(float float1,float float2,float float3);

#endif

