#include "PluginRegister.h"
#include "HTDealerServiceLog.h"
#include<map>

using std::map;

CPluginRegister* CPluginRegister::m_pInstance = new CPluginRegister;
CPluginRegister::CGarbo CPluginRegister::Garbo;

CPluginRegister::CPluginRegister()
{
	HT_XLOG(XLOG_DEBUG, "CPluginRegister::%s\n", __FUNCTION__);	
}

CPluginRegister::~CPluginRegister()
{
	//printf("~CPluginRegister\n");
	HT_XLOG(XLOG_DEBUG, "CPluginRegister::%s clear func map!\n", __FUNCTION__);
	pluginMapSend.clear();
	pluginMapRecv.clear();
}

int CPluginRegister::getSendFuncAddrByName( const string &name, PLUGIN_FUNC_SEND & funcAddr)
{
	HT_XLOG(XLOG_DEBUG, "CPluginRegister::%s, GET FuncNameName[%s]\n", __FUNCTION__, name.c_str());
	NAME_TO_ADD_SEND::iterator iter = pluginMapSend.find(name);
	if( iter == pluginMapSend.end())
		return -1;
	
	funcAddr = iter->second;
	return 0;
	
}

int CPluginRegister::insertSendFunc(string name, PLUGIN_FUNC_SEND funcAddr)
{
	//printf("CPluginRegister::insertSendFunc");
	//判断是否存在相同的函数名
	HT_XLOG(XLOG_DEBUG, "CPluginRegister::%s, insert FuncNameName[%s]\n", __FUNCTION__, name.c_str());
	NAME_TO_ADD_SEND::iterator iter = pluginMapSend.find(name);
	if( iter != pluginMapSend.end())
		return -1;
	
	HT_XLOG(XLOG_DEBUG, "CPluginRegister::%s, insert FuncNameName[%s] sucess\n", __FUNCTION__, name.c_str());
	boost::unique_lock<boost::mutex> lock(m_mut);
	pluginMapSend.insert(make_pair(name, funcAddr));
	return 0;	
}


int CPluginRegister::getRecvFuncAddrByName( const string &name, PLUGIN_FUNC_RECV & funcAddr)
{
	//printf("CPluginRegister::getRecvFuncAddrByName");
	HT_XLOG(XLOG_DEBUG, "CPluginRegister::%s, GET FuncNameName[%s]\n", __FUNCTION__, name.c_str());
	NAME_TO_ADD_RECV::iterator iter = pluginMapRecv.find(name);
	if( iter == pluginMapRecv.end())
		return -1;
	
	funcAddr = iter->second;
	return 0;
	
}

int CPluginRegister::insertRecvFunc(string name, PLUGIN_FUNC_RECV funcAddr)
{
	//判断是否存在相同的函数名
	HT_XLOG(XLOG_DEBUG, "CPluginRegister::%s,insert FuncNameName[%s]\n", __FUNCTION__, name.c_str());
	NAME_TO_ADD_RECV::iterator iter = pluginMapRecv.find(name);
	if( iter != pluginMapRecv.end())
		return -1;
	HT_XLOG(XLOG_DEBUG, "CPluginRegister::%s, insert FuncNameName[%s] sucess\n", __FUNCTION__, name.c_str());
	boost::unique_lock<boost::mutex> lock(m_mut2);
	pluginMapRecv.insert(make_pair(name, funcAddr));
	return 0;
	
}


