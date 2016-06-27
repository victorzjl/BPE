#include "AHTTransfer.h"
#include <stdlib.h> 
#include <time.h>
#include "HTDealerServiceLog.h"
#include "PluginRegister.h"
#include <XmlConfigParser.h>
#include "HTErrorMsg.h"
#include <boost/algorithm/string.hpp>
#include <SapTLVBody.h>
#include "jsonMsg.h"
#include "CodeConverter.h"
#include "JsonAdapter.h"
#include "HTInfo.h"
#include "SapMessage.h"
#include <set>

using namespace boost::algorithm;
using std::set;
using namespace sdo::sap;
using namespace sdo::common;


CAHTTransfer::CAHTTransfer()
{
	HT_XLOG(XLOG_DEBUG,"CAHTTransfer::%s\n", __FUNCTION__);
	Initialize("./avenue_conf");	
}

CAHTTransfer::~CAHTTransfer()
{
	HT_XLOG(XLOG_DEBUG,"CAHTTransfer::%s\n", __FUNCTION__);	
	
	if( !m_HostMap.empty())
		m_HostMap.clear();		
	if(m_pAvenueServiceConfigs)
		delete m_pAvenueServiceConfigs;
}

int CAHTTransfer::Initialize(const char *szXmlConfigFilePath)
{
	HT_XLOG(XLOG_DEBUG,"CAHTTransfer::%s\n", __FUNCTION__);
	vector<int> m_vecService;
	//加载服务描述文件中的相关项
	m_pAvenueServiceConfigs = new CAvenueServiceConfigs;
	m_pAvenueServiceConfigs->LoadAvenueServiceConfigs("./avenue_conf",m_vecService);
	//m_pAvenueServiceConfigs->LoadAvenueServiceConfigByFile();
	map<unsigned int , SServiceConfig> & mapServiceConfigById = m_pAvenueServiceConfigs->GetServiceConfigByIdMap();
	map<unsigned int , SServiceConfig>::iterator iterServiceConfig;
	
	for (iterServiceConfig = mapServiceConfigById.begin(); iterServiceConfig != mapServiceConfigById.end(); iterServiceConfig++)
	{
		m_vecServiceIds.push_back(iterServiceConfig->first);
	
		SServiceConfig &objServiceConfig = iterServiceConfig->second;
		map<int, SConfigType> mapTypeByCode = objServiceConfig.oConfig.GetTypeByCodeMap();
		map<int, SConfigType>::iterator iterConfitType;
		CODE_TYPE_MAP mapNewTypeByCode; 
		for (iterConfitType = mapTypeByCode.begin(); iterConfitType != mapTypeByCode.end(); iterConfitType++)
		{
			mapNewTypeByCode.insert(make_pair(iterConfitType->first, iterConfitType->second.eType));
		}
	
		//m_mapCodeTypeByService.insert(make_pair(iterServiceConfig->first, mapNewTypeByCode));
	}
	
	//读取config文件
	CXmlConfigParser objConfigParser;
	if(objConfigParser.ParseFile("./config.xml") != 0)
	{
		HT_XLOG(XLOG_ERROR, "CAHTTransfer::%s, Config Parser fail, file[./config.xml], error[%s]\n", __FUNCTION__, objConfigParser.GetErrorMessage().c_str());
		return HT_CONFIG_ERROR;
	}
	vector<string> vecHtSosList = objConfigParser.GetParameters("HttpSosList");
	vector<string>::const_iterator iter;
	unsigned int dwServiceId;
	//HT_XLOG(XLOG_DEBUG,"CAHTTransfer::%s --- getHost vecHostsize[%d] vecHostContent[%s]\n", __FUNCTION__,vecHtSosList.size(),vecHtSosList.at(0).c_str());
	
	//构建serviceId和Host相对应的MAP
	for( iter = vecHtSosList.begin(); iter != vecHtSosList.end(); iter++)
	{
		CXmlConfigParser tempCXMLP;
		HT_XLOG(XLOG_DEBUG,"CAHTTransfer::%s --- HostContent[%s]\n", __FUNCTION__,iter->c_str());
		if (tempCXMLP.ParseDetailBuffer(iter->c_str()) != 0)
		{
			HT_XLOG(XLOG_ERROR,"CAHTTransfer::%s config parse fail!\n", __FUNCTION__);
			return HT_CONFIG_ERROR;
			
		}
			
		vector<ahConfValue> vecAddrs;	
		vector<string> serAddr;		
		
		dwServiceId = atoi((tempCXMLP.GetParameter("serviceid").c_str()));
		serAddr = tempCXMLP.GetParameters("serveraddr");
		vector<string>::const_iterator iter_2;
		
		HT_XLOG(XLOG_DEBUG,"CAHTTransfer::%s --- Host[%d]\n", __FUNCTION__,serAddr.size());
		for(iter_2 = serAddr.begin(); iter_2 != serAddr.end(); iter_2++)
		{
			string keyNameLog;
			HT_XLOG(XLOG_DEBUG,"CAHTTransfer::%s --- ServerAddr[%s]\n", __FUNCTION__,iter_2->c_str());
			ahConfValue temp;
			size_t  npos = iter_2->find('@');
			if( npos == string::npos )
			{
				//不存在签名项
				
				temp.hostName = *iter_2;
				temp.key = "";
				temp.name = "";
				
			}
			else
			{
				//存在签名项
				 string str = *iter_2;
				 
				 int pos = str.find("/");
				 string str1 = str.substr(0,pos);
				 string str2 = str.substr(pos+1,str.size()-pos-1);
				 HT_XLOG(XLOG_DEBUG,"CAHTTransfer::%s --- getHost str1[%s] str2[%s]\n", __FUNCTION__,str1.c_str(),str2.c_str());
				
				vector<string> v1; 				 
				 boost::split(v1,str1,boost::is_any_of("@"));				 
				 if(v1.size() < 2)
					 return HT_CONFIG_ERROR;
				 
				 temp.name = v1.at(0);
				 temp.key = v1.at(1);				 
				 temp.hostName = str2;				 
			}
			keyNameLog.append(temp.name +"@" + temp.key + "^_^");
			HT_XLOG(XLOG_DEBUG,"CAHTTransfer::%s --- ServerAddr[%s] keyNameLog[%s]\n", __FUNCTION__,temp.hostName.c_str(),keyNameLog.c_str());			
			vecAddrs.push_back(temp);	
			
		}
		HT_XLOG(XLOG_DEBUG,"CAHTTransfer::%s --- getHost dwServiceId[%d] addr[%d]\n", __FUNCTION__,dwServiceId,vecAddrs.size());
		m_HostMap.insert(make_pair(dwServiceId, vecAddrs));		
	}
	
	return 0;	
}

int CAHTTransfer::AvenueToHttp(IN unsigned int dwServiceId, IN unsigned int dwMsgId, IN unsigned int dwSequenceId,IN const char *pAvenueBody, IN unsigned int nAvenueLen,
							OUT char *pHttpReqPacket, INOUT int &nHttpReqPacketLen, OUT string &serverUri, OUT string &serverAddr)
{
	HT_XLOG(XLOG_DEBUG, "CAHTTransfer::%s, dwServiceId[%d] dwMsgId[%d] dwSeq[%d]\n", __FUNCTION__, dwServiceId, dwMsgId, dwSequenceId);	

	//根据服务号获得服务配置文件
	SServiceConfig *pServiceConfig = NULL;
	int nRet = m_pAvenueServiceConfigs->GetServiceById(dwServiceId, &pServiceConfig);
	
	if( nRet != 0)
	{
		HT_XLOG(XLOG_ERROR, "CAHTTransfer::%s, dwServiceId not supported[%d]\n", __FUNCTION__, dwServiceId);
		return HT_UNSUPPORT_SERVICE;
	}
	//根据消息号获得相应的服务描述
	SConfigMessage *pConfigMessage = NULL;
	nRet = pServiceConfig->oConfig.GetMessageTypeById(dwMsgId, &pConfigMessage);
	if(nRet != 0)
	{
		HT_XLOG(XLOG_ERROR, "CAHTTransfer::%s, SerivceId[%d] ,MsgId[%d] not supported\n", __FUNCTION__, dwServiceId, dwMsgId);
		return HT_UNSUPPORT_MESSAGE;
	}	
	
	bool reqUri = false;                     //uri存在于请求参数中	
	string uri;                              //uri
	string uriParaN;                         //存放uri的参数名
	string SendFunc;                         //构建http包体所用的函数
	string method;                           //http请求的发送方法
	string host;                             //主机地址
	string key;                              //签名所用key
	string name;                        	//签名所用name
	
	
	/*
	* 服务描述头解析
	*/
	
	//判断uri是否存在于请求参数中
	unsigned int npos = (pConfigMessage->url).find("$req.");
	if( npos == string::npos )
	{
		uri = pConfigMessage->url;			
	}
	else
	{
		reqUri = true;
		int size = (pConfigMessage->url).size() - 5;
		int start = 5; // $req.
		uriParaN = (pConfigMessage->url).substr(start, size);		
	}
	//获取SendFunc
	if( pConfigMessage->SendFunc == "")
	{
		SendFunc = "HpsSignature";		
	}
	else
	{
		SendFunc = pConfigMessage->SendFunc;		
	}
	//获取http发送方法
	if( pConfigMessage->method == "")
	{
		method = "GET";		
	}
	else
	{
		method = pConfigMessage->method;		
	}
	HT_XLOG(XLOG_DEBUG, "CAHTTransfer::%s method[%s] SendFunc[%s]\n", __FUNCTION__,method.c_str(), SendFunc.c_str());
	/*
	*获取发送函数参数名
	*/
	map<string, string> paraNameMap;//参数名和值
	vector<string> paraNVec; //用于记录参数顺序
	string paraLog;
	string tempStr;
	if( strcmp(SendFunc.c_str(), "HpsSignature" ) != 0)
	{
		//用户自定义函数
		size_t  posS = SendFunc.find("(");
		size_t  posE = SendFunc.find(")");
		if((posS == string::npos)|| (posE == string::npos))
		{
			HT_XLOG(XLOG_ERROR, "CAHTTransfer::%s, SerivceId[%d]  MsgId[%d] SendFunc[%s] uncorrect\n", __FUNCTION__, dwServiceId,dwMsgId, SendFunc.c_str());
			return HT_UNCORRECT_SNEDFUNC;			
		}
		else
		{
		    //获取函数参数	
			if( posE-posS-1 > 0)
				tempStr = SendFunc.substr(posS + 1, posE-posS-1);
			else
			{
				HT_XLOG(XLOG_ERROR, "CAHTTransfer::%s, SerivceId[%d]  MsgId[%d] SendFunc[%s] uncorrect\n", __FUNCTION__, dwServiceId,dwMsgId, SendFunc.c_str());
				return HT_UNCORRECT_SNEDFUNC;		
			}
			
			//获取发送函数名
			SendFunc = SendFunc.substr(0,posS);
			
			vector<string> tempVecStr;
			boost::split(tempVecStr,tempStr,boost::is_any_of(","));
			HT_XLOG(XLOG_DEBUG, "CAHTTransfer::%s SendFunc[%s] para[%s] vecParaSize[%d]\n", __FUNCTION__,SendFunc.c_str(),tempStr.c_str(),tempVecStr.size());
			//函数参数解析
			vector<string>::iterator itVec = tempVecStr.begin();			
			{
				//获得函数参数名
				for( ;itVec != tempVecStr.end(); itVec++)
				{
					HT_XLOG(XLOG_DEBUG, "CAHTTransfer::%s SendFunc[%s] paraName[%s]\n", __FUNCTION__,SendFunc.c_str(),itVec->c_str());
					size_t posotion = itVec->find("$req.");
					if(posotion == string::npos)
					{
					    paraNameMap.insert(make_pair(*itVec,*itVec));
						paraNVec.push_back(*itVec);
						paraLog.append(*itVec + "^_^");
					}
					else
					{
						
						int start = itVec->find("."); // $req.
						int size = itVec->size() - start -1;
						string temp = itVec->substr(start+1,size);
						paraLog.append(temp + "^_^");
						paraNameMap.insert(make_pair(temp,""));
						paraNVec.push_back(temp);						
					}
				}				
			}
		}		
	}		
	//服务描述文件中SendFunc参数最多6个
	if( paraNVec.size() > 6)
	{
		HT_XLOG(XLOG_ERROR, "CAHTTransfer::%s, SerivceId[%d] MsgId[%d]parameter number[%d] uncorrect!\n", __FUNCTION__, dwServiceId,dwMsgId,paraNVec.size());
		return HT_UNCORRECT_PARA_SENDFUNC;		
	}
	HT_XLOG(XLOG_DEBUG, "CAHTTransfer::%s SendFunc[%s] para[%s]\n", __FUNCTION__,SendFunc.c_str(),paraLog.c_str());
	
	/*
	*判断发送函数是否存在
	*/
	PLUGIN_FUNC_SEND funcAddr;
	if( CPluginRegister::GetInstance()->getSendFuncAddrByName(SendFunc, funcAddr) != 0 )
	{
		HT_XLOG(XLOG_ERROR, "CAHTTransfer::%s, SerivceId[%d] ,MsgId[%d]--SendFunc[%s] not exists!\n", __FUNCTION__, dwServiceId, dwMsgId,SendFunc.c_str());
		return HT_SENDFUNC_NOT_SUPPORT;		
	}	
		
	HT_XLOG(XLOG_DEBUG, "CAHTTransfer::%s, Get HOST NAME\n", __FUNCTION__);
	//主机地址获取
	SID_HOST_MAP::iterator iter = m_HostMap.find(dwServiceId);
	if( iter == m_HostMap.end() )
	{
		HT_XLOG(XLOG_ERROR, "CAHTTransfer::%s, SerivceId[%d] host name not exists\n", __FUNCTION__, dwServiceId);
		return HT_NO_HOST;
	}
	else
	{
		//HT_XLOG(XLOG_DEBUG, "CAHTTransfer::%s, what??????\n", __FUNCTION__);
		//随机选取一个服务配置
		vector<ahConfValue> &ahConfVec = iter->second;
		HT_XLOG(XLOG_DEBUG, "CAHTTransfer::%s, alConfig[%d]\n", __FUNCTION__,ahConfVec.size());
		srand((unsigned int )time(NULL));
		int idx;
		if( ahConfVec.size() == 1)
			idx = 0;
		else
			idx = rand() % (ahConfVec.size());
		HT_XLOG(XLOG_DEBUG, "CAHTTransfer::%s, choose[%d]\n", __FUNCTION__, idx);
		ahConfValue &ahConf = ahConfVec.at(idx);
		
		host = ahConf.hostName;		
		key = ahConf.key;
		name = ahConf.name;
			
	}	
	
	serverAddr = host;
	HT_XLOG(XLOG_DEBUG, "CAHTTransfer::%s, SeriverAddr[%s]\n", __FUNCTION__, serverAddr.c_str());
	//获得avenue体
	SSapMsgHeader *pSapMsgHeader = (SSapMsgHeader *)pAvenueBody;
	char *pExtBuffer = (char *)pAvenueBody + pSapMsgHeader->byHeadLen;
	int nExtLen = nAvenueLen-pSapMsgHeader->byHeadLen;
	CSapTLVBodyDecoder msgDecoder(pExtBuffer, nExtLen);
	
	//特殊参数解析
	map<string, SConfigField>::iterator iterS;
	
	string keyValue = "";
	bool bFirst = true;
	map<string,string>::iterator iterMap;
	//入参解析并构建keyValue参数,获取SendFunc的参数
	for (iterS = pConfigMessage->oRequestType.mapFieldByName.begin(); iterS != pConfigMessage->oRequestType.mapFieldByName.end(); iterS++)
	{
		SConfigField &objConfigField = iterS->second;
		SConfigType objConfigType;
		nRet = pServiceConfig->oConfig.GetTypeByName(objConfigField.strTypeName, objConfigType);
		HT_XLOG(XLOG_DEBUG, "CAHTTransfer::%s, paraType[%s]\n", __FUNCTION__, objConfigField.strTypeName.c_str());	
		if(nRet != 0)
		{
			HT_XLOG(XLOG_ERROR, "CAHTTransfer::%s, SerivceId[%d] ,MsgId[%d] not have TypeName[%s]\n", 
										__FUNCTION__, dwServiceId, dwMsgId, objConfigField.strTypeName.c_str());
			return HT_CONFIG_TYPE_NOT_EXIST;
		}
		string strValue;
		if(objConfigField.bHasDefault)
		{
			if(objConfigType.eType == MSG_FIELD_INT)
			{
				char szTemp[16] = {0};
				snprintf(szTemp, sizeof(szTemp)-1, "%d", objConfigField.nDefaultValue);
				strValue = szTemp;
			}
			else if(objConfigType.eType == MSG_FIELD_STRING)
			{
				strValue = objConfigField.strDefaultValue;
			}
			else
			{
				HT_XLOG(XLOG_ERROR, "CAHTTransfer::%s, SerivceId[%d] ,MsgId[%d] Type[%d] is not supported\n", 
					__FUNCTION__, dwServiceId, dwMsgId, objConfigType.eType);
				return HT_DATA_TYPE_NOT_EXIST;
			}
		}
		else
		{
			
			HT_XLOG(XLOG_DEBUG, "CAHTTransfer::%s, eType[%d] paraTypeCode[%d]\n", __FUNCTION__,objConfigType.eType,objConfigType.nCode);
			if(objConfigType.eType ==  MSG_FIELD_INT)
			{	
				unsigned int dwValue = 0;
				nRet = msgDecoder.GetValue(objConfigType.nCode, &dwValue);

				char szTemp[16] = {0};
				snprintf(szTemp, sizeof(szTemp)-1, "%d", dwValue);
				strValue = szTemp;
			}
			else if(objConfigType.eType == MSG_FIELD_STRING)
			{
				nRet = msgDecoder.GetValue(objConfigType.nCode, strValue);
			}
			else
			{
				HT_XLOG(XLOG_ERROR, "CAHTTransfer::%s, SerivceId[%d] ,MsgId[%d] Type[%d] is not supported\n", 
					__FUNCTION__, dwServiceId, dwMsgId, objConfigType.eType);
				return HT_DATA_TYPE_NOT_EXIST;
			}
			
			HT_XLOG(XLOG_DEBUG, "CAHTTransfer::%s, paraValue[%s]\n", __FUNCTION__,strValue.c_str());
		}

		//Request参数不存在
		if(nRet != 0)
		{
			HT_XLOG(XLOG_DEBUG, "CAHTTransfer::%s, SerivceId[%d] ,MsgId[%d] FieldName[%s] not exist\n", 
				__FUNCTION__, dwServiceId, dwMsgId, objConfigField.strName.c_str());
			continue;
		}

		if(objConfigField.eEncodeMode == ENCODE_GBK)
		{
			Utf82Ascii objCodeTransfer;
			char szGbkCode[1024] = {0};
			
			HT_XLOG(XLOG_DEBUG, "CAHTTransfer::%s, GBK IN(%s)\n", 
										__FUNCTION__, strValue.length());
										
			objCodeTransfer.Convert((char *)strValue.c_str(), strValue.length(), szGbkCode,sizeof(szGbkCode));
			strValue = szGbkCode;
			
			HT_XLOG(XLOG_DEBUG, "CAHTTransfer::%s, GBK OUT(%s)\n", 
										__FUNCTION__, strValue.length());
										
		}
		else if(objConfigField.eEncodeMode == ENCODE_UTF8)
		{
			Ascii2Utf8 objCodeTransfer;
			char szUtf8Content[1024] = {0};
				HT_XLOG(XLOG_DEBUG, "CAHTTransfer::%s, UTF8 IN(%s)\n", 
										__FUNCTION__, strValue.length());
			objCodeTransfer.Convert((char *)strValue.c_str(), strValue.length(), szUtf8Content,sizeof(szUtf8Content));
			strValue = szUtf8Content;
				HT_XLOG(XLOG_DEBUG, "CAHTTransfer::%s, UTF8 OUT(%s)\n", 
										__FUNCTION__, strValue.length());
		}
		
		//if( !objConfigField.bUrlParameter)
		{
			HT_XLOG(XLOG_DEBUG, "CAHTTransfer::%s, paraName[%s], para[%s]\n", __FUNCTION__, objConfigField.strOriName.c_str(),strValue.c_str());			
			//string strValueEncoded = UrlEncoder::encode(strValue.c_str());
			iterMap = paraNameMap.find(objConfigField.strOriName);
			//判断是否是发送函数的参数，是则跳过不放入keyValue中
			if( iterMap != paraNameMap.end())
			{
				HT_XLOG(XLOG_DEBUG, "CAHTTransfer::%s map insert paraName[%s] para[%s]\n", __FUNCTION__, objConfigField.strOriName.c_str(),strValue.c_str());
				iterMap->second = strValue;
				//continue;				
			}
			if(strcmp(uriParaN.c_str(),objConfigField.strOriName.c_str() )==0)
			{
				uri = strValue;
				continue;
			}
			if(bFirst)
			{
				keyValue += objConfigField.strOriName + "=" + strValue;
				bFirst = false;
			}
			else
			{
				keyValue += "&" + objConfigField.strOriName + "=" + strValue; 
			}
		}
	}
	
	//判断uri是否存在
	if( uri == "" )
	{	
		HT_XLOG(XLOG_ERROR, "CAHTTransfer::%s, dwServiceId[%d] dwMsgId[%d]  uri not exists!\n", __FUNCTION__, dwServiceId, dwMsgId);
		return HT_NO_URI;
	}
	
	
	HT_XLOG(XLOG_DEBUG, "CAHTTransfer::%s construct httbody!\n", __FUNCTION__,method.c_str());
	/*
	*
	*调用构造http包体函数构造http包体
	*
	*/
	
	//构建参数体
	AHTPara tempPara;
	
	tempPara.uri = uri;
	tempPara.method = method;
	tempPara.hostName = host;
	tempPara.key = key;
	tempPara.name = name;
	tempPara.keyValue = keyValue;
	
	//按顺序获取相应的参数值
	string valueLog;
	vector<string> paraValue;
	vector<string>::iterator iterPa = paraNVec.begin();
	for(;iterPa != paraNVec.end();iterPa++)
	{
		iterMap = paraNameMap.find(*iterPa);
		if( iterMap != paraNameMap.end() )
		{
			paraValue.push_back(iterMap->second);
			valueLog.append(iterMap->second + "	");			
		}
		
	}
	int paraNum = paraValue.size();
	
	if( paraNum < 6 )
	{
		for(int i = paraNum; i < 6; i++)
		{	//参数值小于6用空补齐
			paraValue.push_back("");
		}
	}
	
	HT_XLOG(XLOG_DEBUG, "CAHTTransfer::%s valueLog[%s]\n", __FUNCTION__,valueLog.c_str());
	//发送函数调用
	string httpBody;
	HT_XLOG(XLOG_DEBUG, "CAHTTransfer::%s keyValue[%s]\n", __FUNCTION__,keyValue.c_str());
	int retVal = funcAddr( tempPara, httpBody, paraValue.at(0), paraValue.at(1),paraValue.at(2),paraValue.at(3),
								paraValue.at(4), paraValue.at(5));
								
	if( retVal != 0 )
	{
		HT_XLOG(XLOG_ERROR, "CAHTTransfer::%s, SerivceId[%d] ,MsgId[%d] httpbody build fail!\n", __FUNCTION__, dwServiceId, dwMsgId);
		return HT_BUILD_HTTPBODY_FAIL;		
	}
	
	//返回http包体 和最终的uri
	nHttpReqPacketLen = httpBody.size();
	memcpy(pHttpReqPacket,httpBody.c_str(),nHttpReqPacketLen);
	pHttpReqPacket[nHttpReqPacketLen] = 0;
	serverUri = host + uri;
	HT_XLOG(XLOG_DEBUG, "CAHTTransfer::%s httpBody[\n%s\n]\n", __FUNCTION__,httpBody.c_str());
	
	return 0;
	
}


int CAHTTransfer::HttpToAvenue(IN unsigned int dwServiceId, IN unsigned int dwMsgId, IN const char *pHttpResPacket, IN int nHttpResLen, 
							OUT char *pAvenueBody, INOUT int &nAvenueLen, OUT int &nRetCode)
{
	nRetCode = 0;
	HT_XLOG(XLOG_DEBUG, "CAHTTransfer::%s, dwServiceId[%d] dwMsgId[%d]\n", __FUNCTION__, dwServiceId, dwMsgId);	

	//根据服务号获得服务配置文件
	SServiceConfig *pServiceConfig = NULL;
	int nRet = m_pAvenueServiceConfigs->GetServiceById(dwServiceId, &pServiceConfig);
	
	if( nRet != 0)
	{
		HT_XLOG(XLOG_DEBUG, "CAHTTransfer::%s, dwServiceId not supported[%d]\n", __FUNCTION__, dwServiceId);
		nRetCode = HT_UNSUPPORT_SERVICE;
		return HT_UNSUPPORT_SERVICE;
	}
	//根据消息号获得相应的服务描述
	SConfigMessage *pConfigMessage = NULL;
	nRet = pServiceConfig->oConfig.GetMessageTypeById(dwMsgId, &pConfigMessage);
	if(nRet != 0)
	{
		HT_XLOG(XLOG_ERROR, "CAHTTransfer::%s, SerivceId[%d] ,MsgId[%d] not supported\n", __FUNCTION__, dwServiceId, dwMsgId);
		nRetCode = HT_UNSUPPORT_MESSAGE;
		return HT_UNSUPPORT_MESSAGE;
	}	
	
	string ReceiveFunc;
	//获取RecveiveFunc
	if( pConfigMessage->ReceiveFunc == "")
	{
		ReceiveFunc = "JsonCommonConvert";		
	}
	else
	{
		ReceiveFunc = pConfigMessage->ReceiveFunc;
		//去掉括号
		size_t end = ReceiveFunc.find("(");
		ReceiveFunc=ReceiveFunc.substr(0,end);
	}
	
	PLUGIN_FUNC_RECV funcAddr;
	//判断是否存在相应的函数
	if( CPluginRegister::GetInstance()->getRecvFuncAddrByName(ReceiveFunc, funcAddr) != 0 )
	{
		HT_XLOG(XLOG_ERROR, "CAHTTransfer::%s, SerivceId[%d] ,MsgId[%d]--ReceiveFunc[%s] not exists!\n", __FUNCTION__, dwServiceId, dwMsgId,ReceiveFunc.c_str());
		nRetCode = HT_UNSUPPORT_MESSAGE;
		return HT_RECVFUNC_NOT_SUPPORT;		
	}	
	
	string jsonBody;
	//http转为json
	if( funcAddr(jsonBody,pHttpResPacket,nHttpResLen) != 0)
	{
		HT_XLOG(XLOG_ERROR, "CAHTTransfer::%s, SerivceId[%d] ,MsgId[%d]--ReceiveFunc[%s] http transfer fail!\n", __FUNCTION__, dwServiceId, dwMsgId,ReceiveFunc.c_str());
		nRetCode = HT_HTTP_TO_JSON_FAIL;
		return HT_HTTP_TO_JSON_FAIL;			
	}
	else
	{
		HT_XLOG(XLOG_DEBUG, "CAHTTransfer::%s, SerivceId[%d] ,MsgId[%d]--jsonBody[%s] http to json success!\n", __FUNCTION__, dwServiceId, dwMsgId,jsonBody.c_str());
		
	}	
	
	/*
	*
	*json转换为avenue
	*
	*/
	HT_XLOG(XLOG_DEBUG, "CAHTTransfer::%s, dwServiceId[%d], dwMsgId[%d] JsonToAvenue json[%s]\n", __FUNCTION__, dwServiceId, dwMsgId,jsonBody.c_str());
	CSapTLVBodyEncoder objTlvBodyEncoder;
	CJsonDecoder objJsonder;
	
	objJsonder.Decoder(jsonBody.c_str(), jsonBody.length());
	
	map<string, SConfigField>::iterator iter;
	//json转为avenue
	for (iter = pConfigMessage->oResponseType.mapFieldByName.begin(); 
			iter != pConfigMessage->oResponseType.mapFieldByName.end(); 
			iter++)
	{
		SConfigField &objConfigField = iter->second;
		SConfigType objConfigType;
		nRet = pServiceConfig->oConfig.GetTypeByName(objConfigField.strTypeName, objConfigType);
		if(nRet != 0)
		{
			HT_XLOG(XLOG_ERROR, "CAHTTransfer::%s, SerivceId[%d] ,MsgId[%d] not have TypeName[%s]\n", 
				__FUNCTION__, dwServiceId, dwMsgId, objConfigField.strTypeName.c_str());
			nRetCode = HT_CONFIG_TYPE_NOT_EXIST;
			return HT_CONFIG_TYPE_NOT_EXIST;
		}		

		string strValue;
		nRet = CJsonAdapter::GetJsonData(objJsonder, objConfigField.strJsonPath, strValue);
		HT_XLOG(XLOG_DEBUG,"CAHTTransfer::%s, SerivceId[%d] ,MsgId[%d] strValue[%s]",__FUNCTION__, dwServiceId, dwMsgId,strValue.c_str());
		if(nRet != 0)
		{
			HT_XLOG(XLOG_TRACE, "CAHTTransfer::%s, SerivceId[%d] ,MsgId[%d] response data without name[%s]\n", 
				__FUNCTION__, dwServiceId, dwMsgId, objConfigField.strOriName.c_str());
			continue;
		}
		if(objConfigField.eEncodeMode == ENCODE_GBK)
		{
			Utf82Ascii objCodeTransfer;
			char szGbkCode[1024] = {0};
			objCodeTransfer.Convert((char *)strValue.c_str(), strValue.length(), szGbkCode,sizeof(szGbkCode));
			strValue = szGbkCode;
		}
		else if(objConfigField.eEncodeMode == ENCODE_UTF8)
		{
			Ascii2Utf8 objCodeTransfer;
			char szUtf8Content[1024] = {0};
			objCodeTransfer.Convert((char *)strValue.c_str(), strValue.length(), szUtf8Content,sizeof(szUtf8Content));
			strValue = szUtf8Content;
		}

		if(objConfigType.eType ==  MSG_FIELD_INT)
		{	
			objTlvBodyEncoder.SetValue(objConfigType.nCode, atoi(strValue.c_str()));
		}
		else if(objConfigType.eType == MSG_FIELD_STRING)
		{
			objTlvBodyEncoder.SetValue(objConfigType.nCode, strValue);
		}
		else
		{
			HT_XLOG(XLOG_ERROR, "CAHTTransfer::%s, SerivceId[%d] ,MsgId[%d] Type[%d] is not supported\n", 
				__FUNCTION__, dwServiceId, dwMsgId, objConfigType.eType);
			nRetCode = HT_DATA_TYPE_NOT_EXIST;
			return HT_DATA_TYPE_NOT_EXIST;
		}
		
		if(objConfigField.bReturnField)
		{
			nRetCode = atoi(strValue.c_str());
		}
	}

	objJsonder.Destroy();
	
	//复制avenue到结果
	int nFinalAvenueLen = objTlvBodyEncoder.GetLen();

	if(nFinalAvenueLen > nAvenueLen)
	{
		HT_XLOG(XLOG_ERROR, "CAHTTransfer::%s, Return AvenueBody len is longer than the buffer\n", __FUNCTION__);
		nRetCode = HT_AVENUE_PACKET_TOO_LONG;
		return HT_AVENUE_PACKET_TOO_LONG;
	}
	else
	{
		nAvenueLen = nFinalAvenueLen;
		memcpy(pAvenueBody,objTlvBodyEncoder.GetBuffer(),nAvenueLen);
		pAvenueBody[nAvenueLen] = 0 ;		
	}
	return 0;	
}
