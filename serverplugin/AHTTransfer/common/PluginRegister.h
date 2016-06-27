#ifndef _PLUGIN_REGISTER_H_
#define _PLUGIN_REGISTER_H_

#include<map>
#include <boost/thread/mutex.hpp> 
#include <string>
#include <stdio.h>
#include "HTInfo.h"


#define IN  
#define OUT  

using std::map;
using std::string;
using namespace HT;

class CPluginRegister
{
public:
	static CPluginRegister* GetInstance( ){return m_pInstance;};
	
	//发送函数
	int getSendFuncAddrByName( const string & name, PLUGIN_FUNC_SEND & funcAddr);
	int insertSendFunc(string name, PLUGIN_FUNC_SEND funcAddr);
	
	//接受函数
	int getRecvFuncAddrByName( const string &name, PLUGIN_FUNC_RECV & funcAddr);
	int insertRecvFunc(string name, PLUGIN_FUNC_RECV funcAddr);
	NAME_TO_ADD_SEND pluginMapSend;
	NAME_TO_ADD_RECV pluginMapRecv;
private:
	CPluginRegister(); //{printf("CPluginRegister\n"); }
	 ~CPluginRegister();
	 
private:
	static CPluginRegister* m_pInstance;	
	boost::mutex m_mut;
	boost::mutex m_mut2;
	
	class CGarbo
    {
    public:
        ~CGarbo()
        {  
			//printf("~CPluginRegister\n");
            if (CPluginRegister::m_pInstance)
                delete CPluginRegister::m_pInstance;
        }
    };
    static CGarbo Garbo;
	
};

#endif
