#include <stdlib.h>
#include "HttpResponseDecoder.h"
#include <boost/algorithm/string.hpp>
#include <string>
#include <vector>
#include <stdio.h>
#include <time.h>
#include <sys/timeb.h>
#include "pluginInterface.h"
#include "Cipher.h"
#include "UrlCode.h"
using std::vector;
using std::string;
using namespace boost::algorithm;
using sdo::common::CCipher;

extern "C"{

int GetSignature(string & key, vector<string> vec4sig, string & signature);
string getContentUrlEncode(string & key);
							
int HpsSignature(IN AHTPara & ahtPara,OUT string & httpBody,                 //默认必带的参数
				 IN string & str1,IN string & str2, IN string & str3,        //用户自定义参数最多6个
				 IN string & str4,IN string & str5, IN string & str6)
{
	SO_XLOG(XLOG_DEBUG,"HpsSignature:: Function Start!\n");
	bool isSig = false;
	bool hasBody = false;
	
	if(strcmp(str1.c_str(), "true") == 0)
	{
		isSig = true;		
	}
	string key; 
	string name;
	vector<string> vec4sig;
	//获得签名所用的key和name
	string nameKey = "merchant_name=" + ahtPara.name;
	string methodKey = "signature_method=MD5";
	long long int time_last;
	time_last = time(NULL);
	char times[64] = {0};
	sprintf(times,"%lld",time_last);
	string timestamp = "timestamp=";
	timestamp.append(times);
	
	if(isSig)
	{
		if((str2 == "") && (str3 == "" ) )
		{
			key = ahtPara.key;
			name = ahtPara.name;
		}
		else
		{
			key = str3;
			name = str2;						
		}
		boost::split(vec4sig,ahtPara.keyValue,boost::is_any_of("&"));
		vec4sig.push_back(nameKey);
		vec4sig.push_back(methodKey);
		vec4sig.push_back(timestamp);
	}
	
	/*
	* 构建http包
	*/
	string signature = "";
	string getSig;
	//获取签名
	if(isSig)//需要签名
	{
		signature.append("signature=");
		GetSignature(key, vec4sig, getSig);	
		signature.append(getSig);		
		signature.append("&"+nameKey);		
		signature.append("&"+methodKey);		
		signature.append("&"+timestamp);
	}
	//请求体构建	
	string reqBody = "";
	if( ahtPara.keyValue.size() < 1)
	{
		if( isSig )
		{
			hasBody = true;
			reqBody = signature;
		}
		
	}
	else
	{
		reqBody.append(getContentUrlEncode(ahtPara.keyValue) );
		hasBody = true;
		if( isSig )
		{
			reqBody = reqBody.append("&" + signature );
		}
	}
	
	//请求头构建
	string reqHead = "";
	
	string host = ahtPara.hostName;
	bool ishttps = false;
	if(host.find("https://") != string::npos)
	{
		host = host.substr(8,host.size()-8);	
		ishttps = true;
		reqHead.append("Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n");
		reqHead.append("User-Agent: Mozilla/5.0 (Windows NT 5.1; rv:22.0) Gecko/20100101 Firefox/22.0\r\n");
	}
	else
	{
		host = host.substr(7,host.size()-7);	
		reqHead.append("Accept: */*\r\n");
		reqHead.append("User-Agent: COH Client/1.0\r\n");
		
	}
	reqHead.append("Host: " + host + "\r\n");
	
	//请求行构建 GET和POST方法请求行不同
	string reqLine = "";
	SO_XLOG(XLOG_DEBUG,"HpsSignature::construct reqLine!\n");
	if(strcmp( ahtPara.method.c_str(), "POST")==0)//POST方法
	{
		if( ishttps)
			reqLine.append(ahtPara.method + " " + ahtPara.uri + " " + "HTTP/1.1\r\n");	
		else
			reqLine.append(ahtPara.method + " " + ahtPara.uri + " " + "HTTP/1.0\r\n");	
	}
	
	if(strcmp( ahtPara.method.c_str(), "GET")==0) //GET方法
	{
		if( hasBody )
			reqLine.append(ahtPara.method + " " + ahtPara.uri + "?" + reqBody);
		else		
			reqLine.append(ahtPara.method + " " + ahtPara.uri);
		if( ishttps )
			reqLine.append(" HTTP/1.1\r\n");
		else
			reqLine.append(" HTTP/1.0\r\n");
			
	}		
	
	
	/**int pos = ahtPara.host.find(":");	
	*默认是80端口？
	if( pos == string::npos)
		reqHead.append("Host: " + ahtPara.host + "\r\n");
	else
		reqHead.append("Host: " + ahtPara.host + ":80\r\n");
	**/		
	
	if(strcmp( ahtPara.method.c_str(), "POST")==0) //post方法
	{
		
		reqHead.append("Content-Type: application/x-www-form-urlencoded\r\n");
		char bufLength[48]={0};
		sprintf(bufLength,"Content-Length: %d\r\n",reqBody.length());
		reqHead.append(bufLength);
		/* 
        if( ishttps)
			reqHead.append("Connection: keep-alive\r\n\r\n");
		else
			reqHead.append("Connection: close\r\n\r\n"); 
        */
        reqHead.append("Connection: close\r\n\r\n");
		httpBody.append(reqLine + reqHead + reqBody);
		
	}
	
	if(strcmp( ahtPara.method.c_str(), "GET")==0) //post方法
	{
		/* 
        if( ishttps)
			reqHead.append("Connection: keep-alive\r\n\r\n");
		else
			reqHead.append("Connection: close\r\n\r\n"); 
        */
        reqHead.append("Connection: close\r\n\r\n"); 
		httpBody.append(reqLine + reqHead);		
	}	
	SO_XLOG(XLOG_DEBUG,"HpsSignature::reqBody[%s]!\n", httpBody.c_str());
	return 0;	
}

int GetSignature(string & key, vector<string> vec4sig, string & signature)
{
	sort(vec4sig.begin(), vec4sig.end());
	
	string needSig;
	vector<string>::iterator itr;
	
	for(itr = vec4sig.begin();itr != vec4sig.end(); itr++)
	{
		needSig += *itr;
	}
	needSig = needSig + key;
	SO_XLOG(XLOG_DEBUG,"HpsSignature::calculatestring[%s]!\n", needSig.c_str());
	unsigned char szSignature[16];
	char szSig[33] = {0};
	memset(szSignature, 0, 16);
	CCipher::Md5((const unsigned char *)needSig.c_str(), needSig.length(), szSignature);
	
	sprintf(szSig, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
		szSignature[0],szSignature[1],szSignature[2],szSignature[3],
		szSignature[4],szSignature[5],szSignature[6],szSignature[7],
		szSignature[8],szSignature[9],szSignature[10],szSignature[11],
		szSignature[12],szSignature[13],szSignature[14],szSignature[15]);
	
	signature = szSig;
	SO_XLOG(XLOG_DEBUG,"HpsSignature::signature[%s]!\n", signature.c_str());
	return 0;	
}


string getContentUrlEncode(string & key)//对传输参数尽心urlencode
{
	vector<string> vec4Para;
	string content;
	boost::split(vec4Para,key,boost::is_any_of("&"));
	vector<string>::iterator itr = vec4Para.begin();
	string paraName;
	string paraCon;
	bool first = true;
	for(;itr != vec4Para.end();++itr)
	{
		int pos1 = itr->find("=");
		paraName = itr->substr(0,pos1);
		paraCon  = itr->substr(pos1+1, itr->size()-pos1-1); 
		
		if( !first )
			content.append("&");
		content.append(paraName + "=" + UrlEncoder::encode(paraCon.c_str()));
		first = false;
	}
	
	return content;	
}


static string name = "HpsSignature";
static PLUGIN_FUNC_SEND funcAddr = HpsSignature;

static ssoSendConfig sconf(name, funcAddr);

//加载动态库的自动初始化函数
ssoConfig* initFunc(){
    //调用注册函数
    //CPluginRegister::GetInstance()->insertSendFunc(name,funcAddr);
	//printf("_init--HpsSig");
	SO_XLOG(XLOG_DEBUG,"HpsSignature:: initFunc!\n");
	return &sconf;
}

}









