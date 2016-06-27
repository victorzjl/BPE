#include <stdlib.h>
#include "MyHttpResponseDecoder.h"
#include <boost/algorithm/string.hpp>
#include "SoPluginLog.h"
#include <string>
#include <stdio.h>
#include "pluginInterface.h"
#include <algorithm>
#include "jsonMsg.h"
//using std::algorithm;


using std::string;
using namespace boost::algorithm;


/**
* 插件函数
*/
extern "C"{


int JsonCommonConvert(OUT string & jsonBody, IN const char *pHttpResPacket, IN int nHttpResLen)
{
	MyCHttpResponseDecoder decoder;
	if(decoder.Decode((char*)pHttpResPacket, nHttpResLen) != 0)
	{
		return -1;
	}
    jsonBody = decoder.GetBody();	
	
	string httpHead;
	string httpBody;
	string httpCode;
	
	//获得头信息
	map<string,string> mapValue = decoder.GetHeadValue();
	map<string,string>::iterator itr = mapValue.begin();
	
	CJsonEncoder jsonEn;
	int nRet = 0; 	
	for( ;itr != mapValue.end(); itr++)	
	{
		nRet = jsonEn.SetValue("",itr->first,itr->second); 	
		if (nRet !=0 )	
		{
			SO_XLOG(XLOG_DEBUG,"JsonCommonConvert::Head To Json Failed ret[%d]!\n");
		}			
	}
	string headJson;
	jsonEn.GetJsonString(headJson);
	httpHead.append("\"httphead\":" + headJson);
	
	jsonEn.Destroy();
	//获得体信息
	httpBody.append("\"httpbody\":" + decoder.GetBody());
	char hCode[32] ={0};
	sprintf(hCode,"%d",decoder.GetHttpCode());
	string temp(hCode);
	httpCode.append("\"httpcode\":" + temp );	
							
	unsigned int pos = jsonBody.find_last_of("}");
	if( pos == string::npos)
	{
		jsonBody.append("{"+httpHead + "," + httpBody + "," + httpCode + "}");
		
	}
	else
	{
		jsonBody.erase(pos);
		jsonBody.append(","+httpHead + "," + httpBody + "," + httpCode + "}");
	}
	
	return 0;
	
}

static string name = "JsonCommonConvert";
static PLUGIN_FUNC_RECV funcAddr = JsonCommonConvert;

static ssoRecvConfig sconf(name, funcAddr);

//加载动态库的自动初始化函数
ssoConfig* initFunc(void) {
    //调用注册函数
    //CPluginRegister::GetInstance()->insertRecvFunc(name,funcAddr);
	printf("_registerRecv\n");
	return &sconf;
	
}

}






