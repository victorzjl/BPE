#ifndef _PLUGIN_INTERFACE_H
#define _PLUGIN_INTERFACE_H

#include <vector>
#include <stdio.h>
#include "SoPluginLog.h"

using std::string;
#define OUT  
#define IN 


	
//插件参数类型
typedef struct paraToSO
{
	//构造http包体头所需参数
	
	string key;           //签名所用的key
	string name;          //签名所用的name	
	string method;      //发送方法	
	string uri;         //uri
	string hostName;    //主机地址
	
	string keyValue;    //http包体keyValue
	
	paraToSO(){}
	~paraToSO(){}	
}AHTPara;

/*
*插件函数
*/

//插件发送函数
typedef int (*PLUGIN_FUNC_SEND)(IN AHTPara & ahtPara,OUT string & httpBody,                                //默认必带的参数
								IN string & str1,IN string & str2, IN string & str3,                       //用户自定义参数最多6个
								IN string & str4,IN string & str5, IN string & str6); 

//插件接受函数指针定义
typedef int (*PLUGIN_FUNC_RECV)(OUT string & jsonBody, IN const char *pHttpResPacket,
								IN int nHttpResLen); 

/*
*插件注册函数返回类型
*/
typedef struct soConfig{
	int type; //类型发送0，接受1
    string name; //函数名
	soConfig(){}
    soConfig(int t, string name):type(t),name(name){}	
}ssoConfig;


//发送函数返回类型
typedef struct soSendConfig : public soConfig{
	PLUGIN_FUNC_SEND funcAddr; //函数地址
	soSendConfig(string name,PLUGIN_FUNC_SEND func):soConfig::soConfig(0,name), funcAddr(func){ }
}ssoSendConfig;

//接受函数返回类型
typedef struct soRecvConfig : public soConfig{
	PLUGIN_FUNC_RECV funcAddr; //函数地址
	soRecvConfig(){}
	soRecvConfig(string name,PLUGIN_FUNC_RECV func):soConfig::soConfig(1,name), funcAddr(func){ }
}ssoRecvConfig;


//加载动态库的自动初始化函数
extern "C" ssoConfig* initFunc();

#endif