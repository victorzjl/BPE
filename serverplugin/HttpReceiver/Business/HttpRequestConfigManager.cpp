#include "HttpRequestConfigManager.h"
#include "BusinessLogHelper.h"
#include "XmlConfigParser.h"
#include <boost/algorithm/string.hpp>
#include <cstdlib>
#include "tinyxml/tinyxml.h"
#include "CommonMacro.h"

using namespace sdo::common;

int CHttpRequestConfigManager::LoadConfig(const vector<string> &vecHttpRequestConf, const map<unsigned int, map<int, string> > &mapSosPriAddrsByServiceId)
{
	BS_XLOG(XLOG_DEBUG,"CHttpRequestConfigManager::%s, vecHttpRequestConf.size[%u], mapSosPriAddrsByServiceId.size[%u]\n",__FUNCTION__,vecHttpRequestConf.size(),mapSosPriAddrsByServiceId.size());
	LoadTimeoutConfig();

	vector<string>::const_iterator iter;
	for(iter = vecHttpRequestConf.begin(); iter != vecHttpRequestConf.end(); ++iter)
	{
		string strTemp = *iter;
		CXmlConfigParser oConfig;
		if(0 != oConfig.ParseDetailBuffer(strTemp.c_str()))
		{
			BS_XLOG(XLOG_ERROR, "CHttpRequestConfigManager::%s,  parse ParseDetailBuffer error:%s, buffer is\n%s\n",__FUNCTION__, oConfig.GetErrorMessage().c_str(),strTemp.c_str());
			continue;
		}
		
		string strUrlMapping = oConfig.GetParameter("UrlMapping");
		vector<string> vecItems;		
		boost::algorithm::split(vecItems,strUrlMapping,boost::algorithm::is_any_of(","),boost::algorithm::token_compress_on);
		if(vecItems.size() != 3)
		{
			BS_XLOG(XLOG_WARNING,"CHttpRequestConfigManager::%s, invalid UrlMapping[%s]\n",__FUNCTION__,strUrlMapping.c_str());
			continue;
		}

		string strUrl = vecItems[0];
		transform(strUrl.begin(), strUrl.end(), strUrl.begin(), ::tolower);
		unsigned int dwServiceId = atoi(vecItems[1].c_str());
		unsigned int dwMsgId = atoi(vecItems[2].c_str());
		SServiceMsgId sSrvMsgId(dwServiceId, dwMsgId);

		SHttpRequestConfig sHttpReqConfig;
		sHttpReqConfig.strUrl = strUrl;
		sHttpReqConfig.sServiceMsgId = sSrvMsgId;		

		SRouteConfig sRouteConfig;
		sHttpReqConfig.strSignature = oConfig.GetParameter("signatureKey", "");
		string strRouteBalance = oConfig.GetParameter("RouteBalance", "");		
		if(0 == LoadRouteBalanceConfig(strRouteBalance,mapSosPriAddrsByServiceId,dwServiceId,dwMsgId, sRouteConfig))
		{
			sHttpReqConfig.sRouteConfig = sRouteConfig;
		}		

		SHttpResponseFormat sHttpResponseFormat;
		string strFields = oConfig.GetParameter("HttpResponseFormat/Fields", "");
		string strFormat = oConfig.GetParameter("HttpResponseFormat/Format", "");
		string strCharSet = oConfig.GetParameter("HttpResponseFormat/CharSet", "utf-8");
		string strContentType = oConfig.GetParameter("HttpResponseFormat/ContentType", "text/html");
		if(0 == LoadHttpResponseFormat(strFields,strFormat, strCharSet, strContentType, sHttpResponseFormat))
		{
			sHttpReqConfig.sHttpResponseFormat = sHttpResponseFormat;
		}
		
		m_mapHttpReqConfByServiceMsgId.insert(make_pair(sSrvMsgId, sHttpReqConfig));
		m_mapHttpReqConfByUrl.insert(make_pair(strUrl, sHttpReqConfig));
		
	}

	return 0;
}

int CHttpRequestConfigManager::LoadTimeoutConfig()
{
	BS_XLOG(XLOG_DEBUG,"COsapUrlManager::%s...\n",__FUNCTION__);
	TiXmlDocument xmlDoc;
	TiXmlElement* xmlRoot;
	TiXmlElement* xmlElement;
	if (!xmlDoc.LoadFile("./HttpRequestConfig.xml"))
	{
		return -1;
	}

	//////////////////////////////////////////////////////////////////////////
	if ((xmlRoot = xmlDoc.RootElement()) == NULL)
	{
		return -2;
	}

	if ((xmlElement = xmlRoot->FirstChildElement("timeoutConfig")) != NULL)
	{
		for (TiXmlElement *xmlUrl = xmlElement->FirstChildElement("url");
			xmlUrl != NULL; xmlUrl = xmlUrl->NextSiblingElement("url"))
		{
			STimeoutConfig conf;
			char buff[1024] = {0};
			if (xmlUrl->QueryValueAttribute("requestName", &buff) != TIXML_SUCCESS)
			{
				continue;
			}
			conf.strParam = buff;
			if (xmlUrl->QueryValueAttribute("default", &buff) != TIXML_SUCCESS)
			{
				continue;
			}
			conf.nDefTimeout = atoi(buff);

			const char* pValue = xmlUrl->GetText();
			if (NULL != pValue)
			{
				string strUrl = pValue;
				transform(strUrl.begin(), strUrl.end(), strUrl.begin(), (int(*)(int))tolower);
				m_mapTimeoutConfig.insert(make_pair(strUrl, conf));
			}
		}
	}

	return 0;
}

int CHttpRequestConfigManager::LoadHttpResponseFormat(const string &strFields, const string &strFormat, const string &strCharSet, string &strContentType, SHttpResponseFormat &sHttpResponseFormat)
{
	BS_XLOG(XLOG_DEBUG,"CHttpRequestConfigManager::%s, strFields[%s], SHttpResponseFormat.[%s]\n",__FUNCTION__,strFields.c_str(),strFormat.c_str());
	vector<string> vecStrFields; 
	if (strFields.size()>0)
	{	
		boost::algorithm::split(vecStrFields,strFields,boost::algorithm::is_any_of(","),boost::algorithm::token_compress_on);
	}
	vector<string>::iterator itr;
	for(itr=vecStrFields.begin(); itr!=vecStrFields.end(); ++itr)
	{
		int nType = atoi((*itr).c_str());
		switch(nType)
		{
		case 1:
			sHttpResponseFormat.vecFields.push_back(E_ErrorCode_Neg);
			break;
		case 2:
			sHttpResponseFormat.vecFields.push_back(E_ErrorCode_Pos);
			break;
		case 3:
			sHttpResponseFormat.vecFields.push_back(E_ErrorMsg_String);
			break;
		default:
			BS_XLOG(XLOG_ERROR,"CHttpRequestConfigManager::%s, unrecognize field type[%d], must 1,2or3\n",__FUNCTION__,nType);
			break;
		}
	}

	sHttpResponseFormat.strFormat = strFormat;
	
	string::size_type loc = sHttpResponseFormat.strFormat.find("&apos;");
	while(loc != string::npos)	
	{	
		sHttpResponseFormat.strFormat = sHttpResponseFormat.strFormat.replace(loc,6,1,'\'');	
		loc = sHttpResponseFormat.strFormat.find("&apos;");	
	}

	loc = sHttpResponseFormat.strFormat.find("&quot;");
	while(loc != string::npos)	
	{	
		sHttpResponseFormat.strFormat = sHttpResponseFormat.strFormat.replace(loc,6,1,'\"');	
		loc = sHttpResponseFormat.strFormat.find("&quot;");	
	}

	loc = sHttpResponseFormat.strFormat.find("&lt;");
	while(loc != string::npos)	
	{	
		sHttpResponseFormat.strFormat = sHttpResponseFormat.strFormat.replace(loc,4,1,'<');	
		loc = sHttpResponseFormat.strFormat.find("&lt;");	
	}
	
	loc = sHttpResponseFormat.strFormat.find("&gt;");
	while(loc != string::npos)	
	{	
		sHttpResponseFormat.strFormat = sHttpResponseFormat.strFormat.replace(loc,4,1,'>');	
		loc = sHttpResponseFormat.strFormat.find("&gt;");	
	}

	
	sHttpResponseFormat.strCharSet = strCharSet;
	sHttpResponseFormat.strContentType = strContentType;
	return 0;
}

int CHttpRequestConfigManager::LoadRouteBalanceConfig(const string &strRouteBalance, const map<unsigned int, map<int, string> > &mapSosPriAddrsByServiceId, 
		unsigned int dwServiceId, unsigned int dwMsgId,SRouteConfig &sRouteConfig)
{	
	
	map<unsigned int, map<int, string > >::const_iterator itrT = mapSosPriAddrsByServiceId.find(dwServiceId);
	if(itrT == mapSosPriAddrsByServiceId.end())
	{
		BS_XLOG(XLOG_WARNING,"CHttpRequestConfigManager::%s, invalid serviceId[%u], not server available. \n",__FUNCTION__,dwServiceId);
		return -1;
	}
	
	sRouteConfig.mapSosPriorityAddr = itrT->second;
	if(sRouteConfig.mapSosPriorityAddr.begin() != sRouteConfig.mapSosPriorityAddr.end())
	{
		sRouteConfig.nCurrentSosPriority = sRouteConfig.mapSosPriorityAddr.begin()->first;
	}
	else
	{
		sRouteConfig.nCurrentSosPriority = 0;
	}

	CXmlConfigParser oConfig;
	if(0 != oConfig.ParseDetailBuffer(strRouteBalance.c_str()))
	{
		BS_XLOG(XLOG_ERROR, "CHttpRequestConfigManager::%s,	parse ParseDetailBuffer error:%s, buffer is\n%s\n", __FUNCTION__, oConfig.GetErrorMessage().c_str(),strRouteBalance.c_str());
		return -2;
	}

	vector<string> vecPolicy = oConfig.GetParameters("RoutePolicy");
	vector<string>::const_iterator itr2;
	for(itr2 = vecPolicy.begin(); itr2 != vecPolicy.end(); ++itr2)
	{
		string strTmp2 = *itr2;
		CXmlConfigParser oConfigPl;
		if(0 != oConfigPl.ParseDetailBuffer(strTmp2.c_str()))
		{
			BS_XLOG(XLOG_ERROR, "CHttpRequestConfigManager::%s,	parse ParseDetailBuffer error:%s, buffer is\n%s\n", __FUNCTION__,oConfigPl.GetErrorMessage().c_str(),strTmp2.c_str());
			continue;
		}
		string strErrorCodes = oConfigPl.GetParameter("ErrorCode");
		vector<string> vecPriTrh = oConfigPl.GetParameters("SosPriorityReqThresHold");
		vector<string> vecErrCodes; 	
		boost::algorithm::split(vecErrCodes,strErrorCodes,boost::algorithm::is_any_of(","),boost::algorithm::token_compress_on);
		vector<string>::iterator itrCode;
		for(itrCode = vecErrCodes.begin(); itrCode != vecErrCodes.end(); ++itrCode)
		{
			int nErrCode = atoi((*itrCode).c_str());
	
			SRoutePolicy sPloicy;
			sPloicy.nErrorCode = nErrCode;
			
			vector<string>::iterator itrPri;
			for(itrPri=vecPriTrh.begin(); itrPri!= vecPriTrh.end(); ++itrPri)
			{
				string strPriTrh = *itrPri;
				BS_XLOG(XLOG_DEBUG,"CHttpRequestConfigManager::%s,	nErrCode[%d], strPriTrh[%s]\n",__FUNCTION__,nErrCode,strPriTrh.c_str());
				vector<string> vecPris; 	
				boost::algorithm::split(vecPris,strPriTrh,boost::algorithm::is_any_of(","),boost::algorithm::token_compress_on);
				if(vecPris.size() != 2)
				{
					BS_XLOG(XLOG_WARNING,"CHttpRequestConfigManager::%s, invalid sosPriorityReqThreshold[%s], serviceId[%u], messagId[%u]\n",__FUNCTION__,strPriTrh.c_str(),dwServiceId,dwMsgId);
					continue;
				}
				int nSosPri = atoi(vecPris[0].c_str());
				int nReqThreshold = atoi(vecPris[1].c_str());
				SPriorityReqThreshold sPriTreshold(nSosPri,nReqThreshold);

				map<int, string>::iterator itrMPri = sRouteConfig.mapSosPriorityAddr.find(nSosPri);
				if(itrMPri == sRouteConfig.mapSosPriorityAddr.end())
				{
					BS_XLOG(XLOG_WARNING,"CHttpRequestConfigManager::%s, nPri[%d]  don't exist in mapSosPriority(config.xml). serviceId[%u], messagId[%u]\n",__FUNCTION__,nSosPri,dwServiceId,dwMsgId);
				}
	
				sPloicy.mapReqThresholdByPri.insert(make_pair(nSosPri, sPriTreshold));
			}
	
			sRouteConfig.mapRoutePolicyByErrCode.insert(make_pair(nErrCode, sPloicy));
		}
	}

	return 0;
}

int CHttpRequestConfigManager::GetCurrentSosPri(unsigned int dwServiceId, unsigned int dwMsgId)
{	
	BS_XLOG(XLOG_DEBUG,"CHttpRequestConfigManager::%s, dwServiceId[%u], dwMsgId[%u]\n",__FUNCTION__,dwServiceId,dwMsgId);
	SServiceMsgId sSrvMsgId(dwServiceId, dwMsgId);
	map<SServiceMsgId, SHttpRequestConfig>::iterator itr = m_mapHttpReqConfByServiceMsgId.find(sSrvMsgId);
	if(itr != m_mapHttpReqConfByServiceMsgId.end())
	{
		SHttpRequestConfig &sHttpReqConfig = itr->second;
		return sHttpReqConfig.sRouteConfig.nCurrentSosPriority;
	}
	return 0;
}

/**
if ErrCode is zero(avenue response success), then set the nFailCount to zero. else
if ErrCode is equal with the last errorCode, then increse the nFailCount. then 
judge if the nFailCount is greater than the threshold of the sos group(priority) and errorcode. 
   if so, change the current pri to the next priority.  and set the nFailCount to zero
   else do nothing
*/
int CHttpRequestConfigManager::UpdateRouteConfig(unsigned int dwServiceId, unsigned int dwMsgId, int nErrCode )
{
	BS_XLOG(XLOG_DEBUG,"CHttpRequestConfigManager::%s, dwServiceId[%u], dwMsgId[%u], nErrCode[%d]\n",__FUNCTION__,dwServiceId,dwMsgId,nErrCode);
	SServiceMsgId sSrvMsgId(dwServiceId, dwMsgId);
	map<SServiceMsgId, SHttpRequestConfig>::iterator itr = m_mapHttpReqConfByServiceMsgId.find(sSrvMsgId);
	if(itr != m_mapHttpReqConfByServiceMsgId.end())
	{
		SRouteConfig &sConfig = itr->second.sRouteConfig;
		if(nErrCode == 0)
		{
			ResetFailMsg(sConfig);
			return 0;
		}

		if(nErrCode == sConfig.nLastErrorCode)
		{
			sConfig.nFailCount++;
		}
		else
		{
			sConfig.nFailCount = 1;
		}
		sConfig.nLastErrorCode = nErrCode;
		
		map<int, SRoutePolicy>::iterator itrPolicy = sConfig.mapRoutePolicyByErrCode.find(nErrCode);
		if(itrPolicy != sConfig.mapRoutePolicyByErrCode.end())
		{
			SRoutePolicy &sPolicy = itrPolicy->second;
			map<int, SPriorityReqThreshold>::iterator itrTrh = sPolicy.mapReqThresholdByPri.find(sConfig.nCurrentSosPriority);
			if(itrTrh != sPolicy.mapReqThresholdByPri.end())
			{
				SPriorityReqThreshold &sTrd = itrTrh->second;
				
				if(sConfig.nFailCount >= sTrd.nReqThreshold)
				{
					map<int, string>::iterator itrPri = sConfig.mapSosPriorityAddr.find(sConfig.nCurrentSosPriority);
					if(itrPri != sConfig.mapSosPriorityAddr.end() && ++itrPri != sConfig.mapSosPriorityAddr.end())
					{
						sConfig.nCurrentSosPriority = itrPri->first;
					}
					else
					{						
						sConfig.nCurrentSosPriority = sConfig.mapSosPriorityAddr.begin()->first;
					}

					ResetFailMsg(sConfig);
				}
			}
		}
	}
	return 0;
}

int CHttpRequestConfigManager::ResetFailMsg(SRouteConfig &sConfig)
{
	sConfig.nLastErrorCode = 0;
	sConfig.nFailCount = 0;

	return 0;
}

string CHttpRequestConfigManager::GetResponseString(unsigned int dwServiceId, unsigned int dwMsgId, int nErrorCode, const string& strIp, const string &strUrl)
{
	BS_XLOG(XLOG_DEBUG,"CHttpRequestConfigManager::%s, dwServiceId[%u], dwMsgId[%u], nErrCode[%d]\n",__FUNCTION__,dwServiceId,dwMsgId,nErrorCode);

	char szErrorCodeNeg[16] = {0};
	snprintf(szErrorCodeNeg, 15, "%d", nErrorCode);
	
	char szErrorCodePos[16] = {0};
	snprintf(szErrorCodePos, 15, "%d", abs(nErrorCode));

	string strErrorMsg = CErrorCode::GetInstance()->GetErrorMessage(nErrorCode);
	if (nErrorCode == ERROR_CHECK_AUTHEN_IPLIST)
	{
		strErrorMsg += ", remote ip is ";
		strErrorMsg += strIp;
	}

	string strResponse = "";
	//SServiceMsgId sSrvMsgId(dwServiceId, dwMsgId);
	//map<SServiceMsgId, SHttpRequestConfig>::iterator itr = m_mapHttpReqConfByServiceMsgId.find(sSrvMsgId);
	//if(itr != m_mapHttpReqConfByServiceMsgId.end())
	map<string, SHttpRequestConfig>::iterator itr = m_mapHttpReqConfByUrl.find(strUrl);
	if(itr != m_mapHttpReqConfByUrl.end())
	{
		SHttpResponseFormat &sResFormat = itr->second.sHttpResponseFormat;
		strResponse = sResFormat.strFormat;

		vector<EFieldType>::iterator itrV;
		string::size_type loc = 0;
		for(itrV=sResFormat.vecFields.begin();itrV!=sResFormat.vecFields.end(); ++itrV)
		{			
			switch(*itrV)
			{
			case E_ErrorCode_Neg:	
				loc = strResponse.find("%d");
				if(loc != string::npos)
				{
					strResponse = strResponse.replace(loc,2,szErrorCodeNeg);					
				}
				break;
				
			case E_ErrorCode_Pos:
				loc = strResponse.find("%u");
				if(loc != string::npos)
				{
					strResponse = strResponse.replace(loc,2,szErrorCodePos);					
				}
				break;
				
			case E_ErrorMsg_String:				
				loc = strResponse.find("%s");
				if(loc != string::npos)
				{
					strResponse = strResponse.replace(loc,2,strErrorMsg);
				}
				break;
				
			default:
				break;				
			}
		}

		if(0 == strncmp(sResFormat.strCharSet.c_str(), "gbk", 3) ||
			0 == strncmp(sResFormat.strCharSet.c_str(), "gb2312", 6))
		{
			strResponse = m_oCodeConvert.Utf82Gbk(strResponse);
		}
	}
	else
	{
		strResponse = string("{\"return_code\":") + szErrorCodeNeg + ",\"return_message\":\"" + strErrorMsg + "\",\"data\":{}}";
	}

	return strResponse;
	
}

void CHttpRequestConfigManager::GetResponseOption(unsigned int dwServiceId, unsigned int dwMsgId, const string &strUrl,
												  string &strCharSet, string &strContentType)
{	
	BS_XLOG(XLOG_DEBUG,"CHttpRequestConfigManager::%s, dwServiceId[%u], dwMsgId[%u] url[%s]\n",__FUNCTION__,dwServiceId,dwMsgId,strUrl.c_str());

	//SServiceMsgId sSrvMsgId(dwServiceId, dwMsgId);
	//map<SServiceMsgId, SHttpRequestConfig>::iterator itr = m_mapHttpReqConfByServiceMsgId.find(sSrvMsgId);
	//if(itr != m_mapHttpReqConfByServiceMsgId.end())
	map<string, SHttpRequestConfig>::iterator itr = m_mapHttpReqConfByUrl.find(strUrl);
	if(itr != m_mapHttpReqConfByUrl.end())
	{
		SHttpResponseFormat &sResFormat = itr->second.sHttpResponseFormat;
		strCharSet = sResFormat.strCharSet;	
		strContentType = sResFormat.strContentType;
	}
	if (strCharSet.empty())
		strCharSet = "utf-8";
	if (strContentType.empty())
		strContentType = "text/html";
}



string CHttpRequestConfigManager::GetSignatureKey(unsigned int dwServiceId, unsigned int dwMsgId, const string &strUrl)
{	
	BS_XLOG(XLOG_DEBUG,"CHttpRequestConfigManager::%s, dwServiceId[%u], dwMsgId[%u]\n",__FUNCTION__,dwServiceId,dwMsgId);
	//SServiceMsgId sSrvMsgId(dwServiceId, dwMsgId);
	//map<SServiceMsgId, SHttpRequestConfig>::iterator itr = m_mapHttpReqConfByServiceMsgId.find(sSrvMsgId);
	//if(itr != m_mapHttpReqConfByServiceMsgId.end())
	map<string, SHttpRequestConfig>::iterator itr = m_mapHttpReqConfByUrl.find(strUrl);
	if(itr != m_mapHttpReqConfByUrl.end())
	{
		return itr->second.strSignature;
	}
	else
	{
		return "";
	}
}

bool CHttpRequestConfigManager::FindTimeoutConfig(const string& strUrl, string& strParam, unsigned int nTimeout)
{
	map<string, STimeoutConfig>::iterator iter = m_mapTimeoutConfig.find(strUrl);
	if (iter != m_mapTimeoutConfig.end())
	{
		strParam = iter->second.strParam;
		nTimeout = iter->second.nDefTimeout;
		return true;
	}
	return false;
}

void CHttpRequestConfigManager::Dump()
{
	BS_XLOG(XLOG_NOTICE,"CHttpRequestConfigManager, =============DUMP================\n");
	BS_XLOG(XLOG_NOTICE," +m_mapHttpReqConfByServiceMsgId.size[%u]\n",m_mapHttpReqConfByServiceMsgId.size());
	map<SServiceMsgId, SHttpRequestConfig>::iterator itr;
	for(itr=m_mapHttpReqConfByServiceMsgId.begin(); itr!=m_mapHttpReqConfByServiceMsgId.end();++itr)
	{
		SHttpRequestConfig &sHttpReqConf = itr->second;
		SRouteConfig &sConfig = sHttpReqConf.sRouteConfig;
		SHttpResponseFormat &sHttpResponseFormat = sHttpReqConf.sHttpResponseFormat;
		BS_XLOG(XLOG_NOTICE,"   +strUrl[%s], serviceId[%u], msgId[%u]\n",
			sHttpReqConf.strUrl.c_str(), sHttpReqConf.sServiceMsgId.dwServiceID, sHttpReqConf.sServiceMsgId.dwMsgID);

		vector<EFieldType>::iterator itrV;
		string strFields;
		for(itrV=sHttpResponseFormat.vecFields.begin();itrV!=sHttpResponseFormat.vecFields.end(); ++itrV)
		{
			char szFields[16] = {0};
			snprintf(szFields,15,"%u,",*itrV);
			strFields += szFields;
		}
		BS_XLOG(XLOG_NOTICE,"	-HttpResonseFields[%s], HttpResponseFormat[%s]\n", 
			strFields.c_str(), sHttpResponseFormat.strFormat.c_str());

		BS_XLOG(XLOG_NOTICE,"   +RouteConfig:  nCurrentSosPriority[%d], nLastErrorCode[%d], nFailCount[%d]\n",
			sConfig.nCurrentSosPriority,sConfig.nLastErrorCode, sConfig.nFailCount);

		BS_XLOG(XLOG_NOTICE,"	       +mapSosPriorityAddr.size[%u]\n",sConfig.mapSosPriorityAddr.size());
		map<int, string>::iterator itrPri;
		for(itrPri=sConfig.mapSosPriorityAddr.begin(); itrPri!=sConfig.mapSosPriorityAddr.end(); ++itrPri)
		{
			BS_XLOG(XLOG_NOTICE,"	          -pri[%d], addr[%s]\n",itrPri->first, (itrPri->second).c_str());
		}

		BS_XLOG(XLOG_NOTICE,"	       +mapRoutePolicyByErrCode.size[%u]\n",sConfig.mapRoutePolicyByErrCode.size());
		map<int, SRoutePolicy>::iterator itrPolicy;
		for(itrPolicy=sConfig.mapRoutePolicyByErrCode.begin(); itrPolicy!=sConfig.mapRoutePolicyByErrCode.end();++itrPolicy)
		{
			SRoutePolicy &sPolicy = itrPolicy->second;				
			BS_XLOG(XLOG_NOTICE,"	          +errorCode[%d],sPolicy.mapReqThresholdByPri.size[%u]\n",itrPolicy->first,sPolicy.mapReqThresholdByPri.size());
					
			map<int, SPriorityReqThreshold>::iterator itrThr;
			for(itrThr=sPolicy.mapReqThresholdByPri.begin(); itrThr!=sPolicy.mapReqThresholdByPri.end(); ++itrThr)
			{
				SPriorityReqThreshold sThr = itrThr->second;
				BS_XLOG(XLOG_NOTICE,"	             -nSosPriority[%d],nReqThreshold[%d]\n", sThr.nSosPriority, sThr.nReqThreshold);
			}
		}
	}
	BS_XLOG(XLOG_NOTICE,"CHttpRequestConfigManager, =============DUMP====end=========\n\n");
}


