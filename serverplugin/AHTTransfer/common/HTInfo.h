#ifndef _HT_INFO_H_
#define _HT_INFO_H_

#include "pluginInterface.h"
#include <map>
#include <string>
#include <vector>
using std::map;
using std::string;
using std::vector;


namespace HT {
#define GUID_CODE 9
#define		VALUE_SPLITTER		"|^_^|"
#define IN
#define OUT


/*
*传递给插件的参数类型
*主机地址
*/
typedef struct confValue
{
	string name;      //签名所用的name
	string key;       //签名所用的key
	string hostName;  //主机地址
	
}ahConfValue;


typedef map<unsigned int, unsigned int> CODE_TYPE_MAP;
typedef map<unsigned int, vector<ahConfValue> > SID_HOST_MAP;

typedef map<string, PLUGIN_FUNC_SEND> NAME_TO_ADD_SEND;               //插件发送函数名和地址对应的map
typedef map<string, PLUGIN_FUNC_RECV> NAME_TO_ADD_RECV;               //插件接受函数名和地址对应的map

//插件默认自带函数
typedef ssoConfig*(*callFunc)(void);
}

#endif
