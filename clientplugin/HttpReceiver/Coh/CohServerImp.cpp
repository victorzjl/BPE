#include "CohServerImp.h"
#include "CohResponseMsg.h"
#include "CohLogHelper.h"
#include "IAsyncLogModule.h"
#include "HpsStack.h"
#include "XmlConfigParser.h"
#include "OsapManager.h"
#include "UpdateManager.h"
#include <boost/algorithm/string.hpp>
#include "CJson.h"
#include "HpsRequestMsg.h"
#include "DirReader.h"
#include "tinyxml/tinyxml.h"
#include "IpLimitManager.h"

using namespace sdo::hps;


namespace sdo{
namespace coh{

string strHttpHead = "<html xmlns=\"http://www.w3.org/1999/xhtml\"><head>"
		"<style type='text/css'>"
		"BODY { color: #000000; background-color: white; font-family: Verdana, ËÎÌו; margin-left: 0px; margin-top: 0px; }"
		"#content { margin-left: 30px; font-size: .70em; padding-bottom: 2em; }"
		".heading1 { color: #ffffff; font-family: Tahoma, ËÎÌו; font-size: 26px; font-weight: normal; background-color: #003366; margin-top: 0px; margin-bottom: 0px; margin-left: -30px; padding-top: 10px; padding-bottom: 3px; padding-left: 15px; width: 105%; }"
		"h3 { font-size: 1.1em; color: #000000; margin-left: -15px; margin-top: 10px; margin-bottom: 10px; }"
		".intro { margin-left: -15px; }"
		"td { color: #000000; font-family: Verdana, ËÎÌו; font-size: .7em; }"
		"</style>"
		"</head>";
	
CohServerImp * CohServerImp::sm_pInstance = NULL;

CohServerImp * CohServerImp::GetInstance()
{
	if(sm_pInstance == NULL)
	{
		sm_pInstance = new CohServerImp;
	}

	return sm_pInstance;
}

int CohServerImp::Start(unsigned int dwPort)
{
	m_dwPort = dwPort;
	return ICohServer::Start(dwPort);
}

CohServerImp::CohServerImp()
{
	m_mapCommand.insert(make_pair("Help.do","Get Help Command"));
	m_mapCommand.insert(make_pair("ReloadSosList.do","Reload Sos list in config.xml"));
	//m_mapCommand.insert(make_pair("UnloadSharedLibs.do","Unload so, Using: SoName=name1.so&SoName=name2.so"));
	//m_mapCommand.insert(make_pair("ReloadSharedLibs.do","Reload so, Using: SoName=name1.so&SoName=name2.so, must unload first"));
	m_mapCommand.insert(make_pair("QuerySelfCheck.do","Get self check message"));
	m_mapCommand.insert(make_pair("GetPluginSoInfo.do","Get so version message"));
	m_mapCommand.insert(make_pair("ReLoadRouteConfig.do","Reload HttpRequest route config"));
	m_mapCommand.insert(make_pair("GetCurrentNoHttpResponseNum.do","Get the current number of Avenue2Http requests which are not responsed"));
	m_mapCommand.insert(make_pair("GetUrlMappingList.do","show the url map list"));

	m_mapCommand.insert(make_pair("NotifyChanged.do","Clear Osap Catch"));
	m_mapCommand.insert(make_pair("QueryAllServices.do","get all service id"));
	m_mapCommand.insert(make_pair("QueryAllURLs.do","get all url"));
	m_mapCommand.insert(make_pair("QueryAllServiceMsg.do","get service and message id"));
	m_mapCommand.insert(make_pair("QueryAllServers.do","get all server info"));
	m_mapCommand.insert(make_pair("NotifyBusinessConfigChanged.do","down load config from osapserver"));
	m_mapCommand.insert(make_pair("ShowParam.do","show parameters"));
}

void CohServerImp::OnReceiveRequest(void * identifier,const string &request,const string &remote_ip,unsigned int remote_port)
{
	CS_XLOG(XLOG_DEBUG,"CohServerImp::%s,remote_ip[%s],remote_port[%d],\n",__FUNCTION__,remote_ip.c_str(),remote_port);
			
	CCohRequestMsg cohRequest;
	if(cohRequest.Decode(request)==0)
	{
		string strCommand = cohRequest.GetCommand();

		if(strCommand=="Help.do")
		{
			OnHelp(identifier);
		}/*
		else if(strCommand=="UnloadSharedLibs.do")
		{
			vector<string> vecShareSos = cohRequest.GetAttributes("SoName");
			OnUnloadSharedLibs(identifier, vecShareSos);
		}
		else if(strCommand=="ReloadSharedLibs.do")
		{
			vector<string> vecShareSos = cohRequest.GetAttributes("SoName");
			OnReloadSharedLibs(identifier,vecShareSos);
		}*/		      
		else if(strCommand=="QuerySelfCheck.do")
		{
			OnQuerySelfCheck(identifier);
		}
		else if(strCommand=="ReloadSosList.do")
		{
			OnReloadSosList(identifier);
		}
		else if(strCommand=="GetPluginSoInfo.do")
		{
			OnGetPluginSoInfo(identifier);
		}
		else if(strCommand=="NotifyChanged.do")
		{			
			OnClearCache(identifier, request);
		}
		else if(strCommand=="GetCurrentNoHttpResponseNum.do")
		{
			OnGetCurrentNoHttpResponseNum(identifier);
		}
		else if(strCommand=="ReLoadRouteConfig.do")
		{
			OnReLoadRouteConfig(identifier);
		}
		else if(strCommand=="CalcMd5.do")
		{
			OnCalcMd5(identifier, request);
		}
		else if(strCommand=="GetUrlMappingList.do")
		{
			OnGetUrlMapping(identifier);
		}
		////////////////////////////////////////////
		else if(strCommand=="QueryAllServices.do")
		{
			OnGetAllService(identifier);
		}
		else if(strCommand=="QueryAllURLs.do")
		{
			OnGetAllURLs(identifier);
		}
		else if(strCommand=="QueryAllServiceMsg.do")
		{
			OnGetAllServiceMsg(identifier);
		}
		else if(strCommand=="QueryAllServers.do")
		{
			OnGetAllServers(identifier);
		}
		else if(strCommand=="NotifyBusinessConfigChanged.do")
		{
			OnUpdateConfigFromOsap(identifier);
		}
		else if(strCommand=="ShowParam.do")
		{
			OnShowParam(identifier, cohRequest.GetAttribute("url"));
		}
		else
		{       	
			CS_XLOG(XLOG_WARNING,"CohServerImp::%s Command not found!\n",__FUNCTION__);
			CCohResponseMsg msg;
			msg.SetStatus(-1,"Not Found Command");
			DoSendResponse(identifier,msg.Encode());
		}
	}
	else
	{
		CCohResponseMsg msg;
		msg.SetStatus(-1,"Can't Parse Request");
		DoSendResponse(identifier,msg.Encode());
	}
}

void CohServerImp::OnHelp(void * identifier)
{
	CS_XLOG(XLOG_DEBUG,"CohServerImp::%s\n",__FUNCTION__);
	string strResponse = "HTTP/1.0 200 OK\r\n\r\n<html><head><title>Help</title></head><body><table border='1' align='center'>";
	strResponse += string("<tr><td>command</td><td>usage</td></tr>");
	map<string,string>::iterator iter;
	for(iter = m_mapCommand.begin(); iter != m_mapCommand.end(); ++iter)
	{
		strResponse += string("<tr>")
			+ "<td>" + iter->first + "</td><td>" + iter->second + "</td>"
			+ "</tr>";
	}
	strResponse += string("</table></body></html>");
	DoSendResponse(identifier,strResponse);	
}

void CohServerImp::OnReloadSosList(void * identifier)
{
	CS_XLOG(XLOG_DEBUG,"CohServerImp::%s\n",__FUNCTION__);
	CCohResponseMsg ResMsg;
			
	CXmlConfigParser oConfig;
	if(oConfig.ParseFile("./config.xml")!=0)
	{
		char szErr[256] = {0};
		sprintf(szErr, "parse config.xml error:%s\n", oConfig.GetErrorMessage().c_str());
		ResMsg.SetStatus(-1,szErr);
	}	 
	else
	{
		vector<string> vecSosListConfig = oConfig.GetParameters("SosList");
		CXmlConfigParser oServer;
		if(oServer.ParseFile("./server.xml")==0)
		{
			vector<string> vecSosListServer = oServer.GetParameters("SosList");
			vecSosListConfig.insert(vecSosListConfig.begin(), vecSosListServer.begin(), vecSosListServer.end());
		}	 

		CHpsStack::Instance()->OnLoadSosList(vecSosListConfig);
	}
	
	DoSendResponse(identifier,ResMsg.Encode());
}

void CohServerImp::OnUnloadSharedLibs(void * identifier,const vector<string> &vecSo)
{
	CS_XLOG(XLOG_DEBUG,"CohServerImp::%s\n",__FUNCTION__);
/*
	CCohResponseMsg ResMsg;		
	CXmlConfigParser oConfig;
	if(oConfig.ParseFile(+"./config.xml")!=0)
	{
		char szErr[256] = {0};
		sprintf(szErr, "parse config error:%s\n", oConfig.GetErrorMessage().c_str());
		ResMsg.SetStatus(-1,szErr);
	}	
	else
	{	
		string strPluginDir = oConfig.GetParameter("TransferLibs", "./plugin");
		CHpsStack::Instance()->OnUnLoadSharedLibs(strPluginDir,vecSo);	
		string strReloadErorMsg;
		int nRet = CAvenueHttpTransfer::WaitForReload(1, strReloadErorMsg);		
		ResMsg.SetStatus(nRet,strReloadErorMsg);
	}

	DoSendResponse(identifier,ResMsg.Encode());
	*/
	
}

void CohServerImp::OnReloadSharedLibs(void * identifier,const vector<string> &vecSo)
{
	CS_XLOG(XLOG_DEBUG,"CohServerImp::%s\n",__FUNCTION__);/*
	CCohResponseMsg ResMsg;
	CXmlConfigParser oConfig;
	if(oConfig.ParseFile(+"./config.xml")!=0)
	{
		char szErr[256] = {0};
		sprintf(szErr, "parse config error:%s\n", oConfig.GetErrorMessage().c_str());
		ResMsg.SetStatus(-1,szErr);
	}	
	else
	{		
		string strPlugin = oConfig.GetParameter("TransferLibs", "./plugin");
		string strReloadErorMsg;
		int nRet = CAvenueHttpTransfer::SetReloadFlag(1,strReloadErorMsg);
		if(nRet != 0)
		{
			ResMsg.SetStatus(nRet,strReloadErorMsg);	
		}
		else
		{
			CHpsStack::Instance()->OnReLoadTransferObjs(strPlugin,vecSo);
			nRet = CAvenueHttpTransfer::WaitForReload(1,strReloadErorMsg);		
			ResMsg.SetStatus(nRet,strReloadErorMsg);
		}
	}
	
	DoSendResponse(identifier,ResMsg.Encode());*/
}

void CohServerImp::OnGetPluginSoInfo(void *identifier)
{
	CS_XLOG(XLOG_DEBUG,"CohServerImp::%s\n",__FUNCTION__);
	string strResponse = "<html><head><title>PluginSoInfo</title></head><body><div align='center'>";
	strResponse += CHpsStack::Instance()->GetPluginSoInfo();
	strResponse += string("</div></body></html>");
	DoSendResponse(identifier,strResponse);	
}

		
void CohServerImp::OnQuerySelfCheck(void * identifier)
{
    CS_XLOG(XLOG_DEBUG,"CohServerImp::%s\n",__FUNCTION__);
    string strCheck = GetAsyncLogModule()->GetSelfCheckData();
            
    char bufLength[48]={0};
	sprintf(bufLength,"Content-Length: %d\r\n\r\n",strCheck.length());
            
    string strResponse=string("HTTP/1.0 200 OK\r\n" \
          "Content-Type: text/html;charset=utf-8\r\n" \
          "Server: COH Server/1.0\r\n") +bufLength+strCheck.c_str();
            
	DoSendResponse(identifier,strResponse);
}

void CohServerImp::OnClearCache(void * identifier, const string& strRequest)
{
    CS_XLOG(XLOG_DEBUG,"CohServerImp::%s\n",__FUNCTION__);

	string strMerchant = sdo::hps::CHpsRequestMsg::GetValueByKey(strRequest, "merchant_name");

	CHpsStack::Instance()->ClearOsapCatch(strMerchant);

	map < string,string > mapIpsByUrl;
	CIpLimitLoader::GetInstance()->ReLoadConfig(mapIpsByUrl);
	CHpsStack::Instance()->OnUpdateIpLimit(mapIpsByUrl);

	
	string strResponse = string("HTTP/1.0 200 OK\r\n\r\nOk");
	DoSendResponse(identifier,strResponse);
}


void CohServerImp::OnGetCurrentNoHttpResponseNum(void * identifier)
{
    CS_XLOG(XLOG_DEBUG,"CohServerImp::%s\n",__FUNCTION__);
    unsigned int dwNum = GetAsyncLogModule()->GetNoHttpResponseNum();
	char szBody[128] = {0};
	snprintf(szBody, 127, "{\"num\":%u}", dwNum);
            
    char bufLength[48]={0};
	sprintf(bufLength,"Content-Length: %d\r\n\r\n",strlen(szBody));
            
    string strResponse=string("HTTP/1.0 200 OK\r\n" \
          "Content-Type: text/html;charset=utf-8\r\n" \
          "Server: COH Server/1.0\r\n") +bufLength+szBody;
            
	DoSendResponse(identifier,strResponse);
}


void CohServerImp::OnReLoadRouteConfig(void * identifier)
{
    CS_XLOG(XLOG_DEBUG,"CohServerImp::%s\n",__FUNCTION__);
	CCohResponseMsg ResMsg;
	CXmlConfigParser oConfig;
    if(oConfig.ParseFile("./HttpRequestConfig.xml")!=0)
    {
		char szErr[256] = {0};
		sprintf(szErr, "parse config error:%s\n", oConfig.GetErrorMessage().c_str());
		ResMsg.SetStatus(-1,szErr);
    }  
	else
	{
		CHpsStack::Instance()->OnLoadRouteConfig(oConfig.GetParameters("HttpRequest"));
		ResMsg.SetStatus(0,"OK");
	}
	
	DoSendResponse(identifier,ResMsg.Encode());
}

void CohServerImp::OnCalcMd5(void * identifier, const string& strRequest)
{
	char szCommand[128]={0};
	char szAttribute[2048]={0};
	sscanf(strRequest.c_str(),"%[^?]?%s",szCommand,szAttribute);

	string strAttri = szAttribute;
	string strKey;
	size_t posFind;
	if ((posFind = strAttri.find("signature_key=")) != string::npos)
	{
		strKey = strAttri.substr(posFind+strlen("signature_key="));
		strAttri = strAttri.substr(0, posFind);
	}

	string strResponse=string("HTTP/1.0 200 OK\r\n\r\n");
	strResponse += COsapManager::OnCalcMd5(strAttri, strKey);

	DoSendResponse(identifier, strResponse);
}

void CohServerImp::OnGetUrlMapping(void *identifier)
{
	CS_XLOG(XLOG_DEBUG,"CohServerImp::%s\n",__FUNCTION__);
	vector<string> vecUrlMapping;
	CHpsStack::Instance()->GetUrlMapping(vecUrlMapping);
	
	string strResponse = "<html><head><title>UrlMappingList</title></head><body><div align='center'>";
	strResponse += "UrlMappingList<p><table border='2' cellpadding='5' cellspacing='0'>";
	for(vector<string>::iterator itr=vecUrlMapping.begin(); itr!=vecUrlMapping.end(); ++itr)
	{
		char szCount[32] = {0};
		snprintf(szCount, 31, "%d", itr-vecUrlMapping.begin()+1);
		strResponse += "<tr><td>" + string(szCount) + "</td><td>" + *itr + "</td></tr>";
	}
	
	strResponse += string("</table></p></div></body></html>");
	DoSendResponse(identifier,strResponse);	
}

void CohServerImp::OnGetAllService(void * identifier)
{
	vector<string> vecUrlMapping;
	vector<int> vecRes;
	CHpsStack::Instance()->GetUrlMapping(vecUrlMapping);
	for(vector<string>::iterator it=vecUrlMapping.begin();
		it!=vecUrlMapping.end();it++)
	{
		vector<string> vecSplit;
		boost::algorithm::split( vecSplit, (*it), boost::algorithm::is_any_of(","),boost::algorithm::token_compress_on);  
		if( vecSplit.size() == 3 )
		{
			if (find(vecRes.begin(), vecRes.end(), atoi(vecSplit[1].c_str())) == vecRes.end())
			{
				vecRes.push_back(atoi(vecSplit[1].c_str()));
			}
		}
	}
	CJsonEncoder json;
	json.SetValue("", "return_code", vecRes.size() ==0 ? -1 : 0);
	json.SetValue("", "data", vecRes);
	string strResponse = string("HTTP/1.0 200 OK\r\n\r\n") +
		string((char*)json.GetJsonString(), json.GetJsonStringLen());
	json.DestroyJsonEncoder();
	DoSendResponse(identifier, strResponse);
}

void CohServerImp::OnGetAllURLs(void * identifier)
{
	vector<string> vecUrlMapping;
	vector<string> vecRes;
	CHpsStack::Instance()->GetUrlMapping(vecUrlMapping);
	for(vector<string>::iterator it=vecUrlMapping.begin();
		it!=vecUrlMapping.end();it++)
	{
		vector<string> vecSplit;
		boost::algorithm::split( vecSplit, (*it), boost::algorithm::is_any_of(","),boost::algorithm::token_compress_on);  
		if( vecSplit.size() == 3 )
		{
			string strUrl = vecSplit[0];
			if (find(vecRes.begin(), vecRes.end(), strUrl) == vecRes.end())
			{
				vecRes.push_back(strUrl);
			}
		}
	}
	CJsonEncoder json;
	json.SetValue("", "return_code", vecRes.size() ==0 ? -1 : 0);
	json.SetValue("", "data", vecRes);
	string strResponse = string("HTTP/1.0 200 OK\r\n\r\n") +
		string((char*)json.GetJsonString(), json.GetJsonStringLen());
	json.DestroyJsonEncoder();
	DoSendResponse(identifier, strResponse);
}

void CohServerImp::OnGetAllServiceMsg(void *identifier)
{
	vector<string> vecUrlMapping;
	vector<SServiceMsgId> vecRes;
	CHpsStack::Instance()->GetUrlMapping(vecUrlMapping);
	for(vector<string>::iterator it=vecUrlMapping.begin();
		it!=vecUrlMapping.end();it++)
	{
		vector<string> vecSplit;
		boost::algorithm::split( vecSplit, (*it), boost::algorithm::is_any_of(","),boost::algorithm::token_compress_on);  
		if( vecSplit.size() == 3 )
		{
			unsigned dwServiceID = atoi(vecSplit[1].c_str());
			unsigned dwMsgID = atoi(vecSplit[2].c_str());
			vecRes.push_back(SServiceMsgId(dwServiceID, dwMsgID));
		}
	}
	CJsonEncoder json;
	json.SetValue("", "return_code", vecRes.size() ==0 ? -1 : 0);
	vector<CJsonEncoder> dataJson;
	for (vector<SServiceMsgId>::iterator iter = vecRes.begin();
		iter != vecRes.end(); ++iter)
	{
		CJsonEncoder data;
		data.SetValue("", "serviceId", iter->dwServiceID);
		data.SetValue("", "messageId", iter->dwMsgID);
		dataJson.push_back(data);
	}
	json.SetValue("", "data", dataJson);
	string strResponse = string("HTTP/1.0 200 OK\r\n\r\n") + 
		string((char*)json.GetJsonString(), json.GetJsonStringLen());
	json.DestroyJsonEncoder();
	DoSendResponse(identifier, strResponse);
}

void CohServerImp::OnGetAllServers(void * identifier)
{
	string strResponse=string("HTTP/1.0 200 OK\r\n\r\n");
	string strJson;
	CUpdateManager::GetAllServerInfo(strJson);
	strResponse += strJson;
	DoSendResponse(identifier, strResponse);
}

void CohServerImp::OnUpdateConfigFromOsap(void *identifier)
{
	CS_XLOG(XLOG_DEBUG,"CohServerImp::%s\n",__FUNCTION__);

	int nRet = CHpsStack::Instance()->OnUpdateConfigFromOsap();
	CJsonEncoder json;
	json.SetValue("", "return_code", nRet);
	json.SetValue("", "return_message", CUpdateManager::GetErrorString(nRet));
	string strResponse = string("HTTP/1.0 200 OK\r\n\r\n") + 
		string((char*)json.GetJsonString(), json.GetJsonStringLen());
	json.DestroyJsonEncoder();
	DoSendResponse(identifier,strResponse);
}

string GetTypeDec(TiXmlElement* xml, const string& typeName)
{
	for (TiXmlElement *xmlType = xml->FirstChildElement("type");
		xmlType != NULL; xmlType = xmlType->NextSiblingElement("type"))
	{
		char buff[1024] = {0};
		if (xmlType->QueryValueAttribute("name", &buff) == TIXML_SUCCESS &&
			buff == typeName)
		{
			if (xmlType->QueryValueAttribute("description", &buff) == TIXML_SUCCESS)
			{
				return buff;
			}
			return "";
		}
	}
	return "";
}

void CohServerImp::OnShowParam(void *identifier, const string& strUrl)
{
	SUrlInfo soInfo = CHpsStack::Instance()->GetSoNameByUrl(strUrl);
	string strResponse = string("HTTP/1.0 200 OK\r\n\r\n");
	strResponse += strHttpHead + "<body><div id='content'><p class='heading1'>HPS</p><br>";
	string strRequest = "?";
	string strContent;
	string::size_type pos = soInfo.strSoName.rfind("/");
	if (pos == string::npos)
	{
		DoSendResponse(identifier,strResponse+"unknown url 0!");
		return;
	}
	string strPath = soInfo.strSoName.substr(0, pos);
	nsHpsTransfer::CDirReader oDirReader("*.xml");
	if (!oDirReader.OpenDir((strPath + "/Services").c_str()))
	{
		if (!oDirReader.OpenDir((strPath + "/services").c_str()))
		{
			DoSendResponse(identifier,strResponse+"unknown url 1!");
			return;
		}
	}
	char szFileName[MAX_PATH] = {0};
	if (!oDirReader.GetFirstFilePath(szFileName))
	{
		DoSendResponse(identifier,strResponse+"unknown url 2!");
		return;
	}
	do
	{
		TiXmlDocument xmlDoc;
		TiXmlElement* xmlElement, *xmlReq, *xmlRsp;
		char buff[1024] = {0};
		if (!xmlDoc.LoadFile(szFileName) || (xmlElement = xmlDoc.RootElement()) == NULL)
		{
			continue;
		}
		if (xmlElement->QueryValueAttribute("id", &buff) != TIXML_SUCCESS ||
			atoi(buff) != soInfo.dwServiceId)
		{
			continue;
		}
		for (TiXmlElement *xmlMsg = xmlElement->FirstChildElement("message");
			xmlMsg != NULL; xmlMsg = xmlMsg->NextSiblingElement("message"))
		{
			if (xmlMsg->QueryValueAttribute("id", &buff) != TIXML_SUCCESS ||
				atoi(buff) != soInfo.dwMessageId)
			{
				continue;
			}
			if (xmlMsg->QueryValueAttribute("description", &buff) == TIXML_SUCCESS)
			{
				strResponse += string("<h3>description:</h3>") + buff;
			}
			strResponse += "<h3>request url: </h3>";
			strResponse += strUrl;
			strContent += "<h3>request parameters: </h3><table border='2' cellpadding='5' cellspacing='0' width='60%'>";
			if ((xmlReq = xmlMsg->FirstChildElement("requestParameter")) != NULL)
			{
				int index = 0;
				strContent += "<tr><td width='30%'><b>name</td><td><b>description</td></tr>";
				for (TiXmlElement *xmlFld = xmlReq->FirstChildElement("field");
					xmlFld != NULL; xmlFld = xmlFld->NextSiblingElement("field"))
				{
					char szName[MAX_PATH] = {0};
					char szDesc[1024] = {0};
					char szType[MAX_PATH] = {0};
					xmlFld->QueryValueAttribute("name", &szName);
					xmlFld->QueryValueAttribute("type", &szType);
					xmlFld->QueryValueAttribute("description", &szDesc);
					string strDesc = GetTypeDec(xmlElement, szType);
					if (!strDesc.empty() && strlen(szDesc) > 0)
						strDesc += ", ";
					strDesc += szDesc;
					if (index != 0)
						strRequest += "&";
					index++;
					strRequest += szName;
					strRequest += "=XXX";
					strContent += string("<tr><td>") + szName + "&nbsp;</td><td>" + strDesc + "&nbsp;</td></tr>";
				}
			}
			strContent += "</table><h3>response parameters: </h3><table border='2' cellpadding='5' cellspacing='0' width='60%'>";
			if ((xmlRsp = xmlMsg->FirstChildElement("responseParameter")) != NULL)
			{
				strContent += "<tr><td width='30%'><b>name</td><td><b>description</td></tr>";
				for (TiXmlElement *xmlFld = xmlRsp->FirstChildElement("field");
					xmlFld != NULL; xmlFld = xmlFld->NextSiblingElement("field"))
				{
					char szName[MAX_PATH] = {0};
					char szDesc[1024] = {0};
					char szType[MAX_PATH] = {0};
					xmlFld->QueryValueAttribute("name", &szName);
					xmlFld->QueryValueAttribute("type", &szType);
					xmlFld->QueryValueAttribute("description", &szDesc);
					string strDesc = GetTypeDec(xmlElement, szType);
					if (!strDesc.empty() && strlen(szDesc) > 0)
						strDesc += ", ";
					strDesc += szDesc;
					strContent += string("<tr><td>") + szName + "&nbsp;</td><td>" + strDesc + "&nbsp;</td></tr>";
				}
			}
			strContent += "</table><h3>signature test: </h3>";
			char port[32];
			sprintf(port, "%u", m_dwPort);
			strContent += string("http://inner.hps.sdo.com:") + port + "/CalcMd5.do" + strRequest
				+ "&signature_method=MD5&amp;timestamp=XXX&merchant_name=XXX&signature_key=XXX";
			strContent += "<p>All values should be url encoded</p>";
			strContent += "</div></body></html>";
			strRequest += "&signature_method=MD5&amp;timestamp=XXX&merchant_name=XXX&signature=XXX";
			strResponse += strRequest + strContent;
			DoSendResponse(identifier,strResponse);
			return;
		}
	}while(oDirReader.GetNextFilePath(szFileName));

	DoSendResponse(identifier,strResponse+"unknown url 3!");
}

}
}

