#include "OsapManager.h"
#include "HpsCommon.h"
#include "UrlCode.h"
#include "HpsCommonFunc.h"
#include <boost/algorithm/string.hpp>
#include "HpsStack.h"
#include <Cipher.h>
#include "BusinessLogHelper.h"
#include "HpsRequestMsg.h"

#define SMS_MSG_GETALL		8
#define SMS_ATTRI_MD5_PWD	1



namespace
{

enum
{
	SMS_Attri_Name			= 1,//(string)
	SMS_Attri_Ip			= 2, //(string)
	SMS_Attri_Port			= 3,// (INT)

	SMS_Attri_ConfValue		= 4, //(key+value)           //OLD PROTOCOL
	SMS_Attri_Privilege		= 5, //(string)
	SMS_Attri_PasswdType	= 6, //(struct)
	SMS_Attri_SpId          = 7, //  £¨INT£©
	SMS_Attri_AppId         = 8, //£¨INT£©
	SMS_Attri_AreaId        = 9, //£¨INT£©
	SMS_Attri_GroupId       = 10, //£¨INT£©
	SMS_Attri_HostId        = 11,// £¨INT£©
	SMS_Attri_ConfigIp      = 13,//(string)
	SMS_Attri_ConfigPort    = 14, //£¨INT£©
	SMS_Attri_LimitIp       = 15,//£¨string£©

};

bool is_sys_error(int code)
{
	return (code<=-10250000 && code>=-10251000 ||
		code<=-10242100 && code>=-10242110);
}

void get_key_value(const string& attri, string& key, string& value)
{
	string::size_type pos = attri.find('=');
	if(pos != string::npos)
	{
		key = attri.substr(0, pos);
		if (pos != attri.length()-1)
		{
			value = attri.substr(pos+1, attri.length()-pos).c_str();
		}
	}
}

bool cmpstr(const string& s1, const string& s2)
{
	string key1, key2, value1, value2;
	get_key_value(s1, key1, value1);
	get_key_value(s2, key2, value2);
	if (key1 == key2)
		return value1 < value2;
	else
		return key1 < key2;
}

bool cmpstr_apipool(const string& s1, const string& s2)
{
	string str1 = s1;
	string str2 = s2;
	transform(str1.begin(), str1.end(), str1.begin(), (int(*)(int))tolower);
	transform(str2.begin(), str2.end(), str2.begin(), (int(*)(int))tolower);

	return cmpstr(str1, str2);
}

void encode_hex_str(unsigned char * pszIn, int nInLen, unsigned char* pszOut)
{
	for (int i=0; i<nInLen; i++)
	{
		sprintf((char*)(pszOut+i*2), "%02x", pszIn[i]);
	}
}

time_t get_time(char * szTime) 
{ 
	tm tm1 = {0}; 
	sscanf(szTime, "%4d-%2d-%2d %2d:%2d:%2d",      
		&tm1.tm_year, 
		&tm1.tm_mon, 
		&tm1.tm_mday, 
		&tm1.tm_hour, 
		&tm1.tm_min, 
		&tm1.tm_sec);
	if (tm1.tm_year <= 1971)
		return 0;
	if (tm1.tm_year >= 2038)
		return 2147483647;
	tm1.tm_year -= 1900; 
	tm1.tm_mon --; 
	tm1.tm_isdst=-1; 
	return mktime(&tm1); 
} 

} // namespace

COsapManager::COsapManager()
: m_nSequence(0)
{
}

COsapManager::~COsapManager()
{
	ClearCatch();
}

void COsapManager::SetSessionManager(CHpsSessionManager* pSessionManager)
{
	BS_XLOG(XLOG_DEBUG,"COsapManager::%s...\n",__FUNCTION__);
	m_pSessionManager = pSessionManager;
}

int COsapManager::LoadConfig()
{
	BS_XLOG(XLOG_DEBUG,"COsapManager::%s...\n",__FUNCTION__);
	return m_config.Load();
}

bool COsapManager::IsUrlInNoSaagList(const string& strUrl)
{
	BS_XLOG(XLOG_DEBUG,"COsapManager::%s...\n",__FUNCTION__);
	return m_config.IsUrlInNoSaagList(strUrl);
}

int COsapManager::GetUserInfo(SRequestInfo &sReq, OsapUserInfo** ppUserInfo)
{
	BS_XLOG(XLOG_DEBUG,"COsapManager::%s...\n",__FUNCTION__);
	if (CHpsStack::nOsapOption == 0)
	{
		BS_XLOG(XLOG_DEBUG,"COsapManager::%s CHpsStack::nOsapOption == 0\n",__FUNCTION__);
		return 0; // do not need check
	}
    
	if (m_config.IsUrlInWhiteList(sReq.strUrlIdentify))
	{
		BS_XLOG(XLOG_DEBUG,"COsapManager::%s URL[%s] InWhiteList\n",__FUNCTION__,sReq.strUrlIdentify.c_str());
		sReq.inWhiteList = true;
		//return 0; // do not need check
	}
    sReq.inTmpWhiteList=m_config.IsUrlInTmpWhiteList(sReq.strUrlIdentify);
    BS_XLOG(XLOG_DEBUG,"COsapManager::%s inWhiteList[%d] inTmpWhiteList[%d]\n",__FUNCTION__,sReq.inWhiteList,sReq.inTmpWhiteList);
	if (sReq.strUserName.empty())
	{
		sReq.strUserName = m_config.GetDefaultUserName(sReq.strUrlIdentify);
		if (sReq.strUserName.empty())
		{
			BS_XLOG(XLOG_DEBUG,"COsapManager::%s URL[%s] no user name\n",__FUNCTION__,sReq.strUrlIdentify.c_str());
			sReq.nSignatureStatus = OSAP_NO_USER_PARAM;
			return (sReq.inWhiteList || sReq.inTmpWhiteList)?  0 : ERROR_SERVER_REJECT;
		}
		sReq.bFromApipool = true;
	}

	sReq.bOsapCheck = true;
	boost::unordered_map<string, OsapUserInfo*>::iterator iter = 
		m_mapUserInfoCatch.find(sReq.strUserName);
	if (iter == m_mapUserInfoCatch.end())
	{
		BS_XLOG(XLOG_DEBUG,"COsapManager::%s need osap request\n",__FUNCTION__);
		return DoSendOsapRequest(sReq);
	}

	OsapUserInfo *pUserInfo = iter->second;

	timeval_a now,interval,endtime;
	gettimeofday_a(&now,NULL);
	interval.tv_sec = 30*60;
	interval.tv_usec = 0;
	timeradd(&(pUserInfo->tmUpdate),&interval,&endtime);
	if (timercmp(&now,&endtime,>=))
	{
		BS_XLOG(XLOG_DEBUG,"COsapManager::%s, timeout! need osap request\n",__FUNCTION__);
		delete pUserInfo;
		m_mapUserInfoCatch.erase(iter);
		return DoSendOsapRequest(sReq);
	}
	pUserInfo->tmUpdate = now;
	//CHpsStack::Instance()->UpdataOsapUserTime(m_pSessionManager, sReq.strUserName, now);
	if (ppUserInfo != NULL)
	{
		*ppUserInfo = pUserInfo;
	}
	return 0;
}

int COsapManager::GetUserInfo(SAvenueRequestInfo& sReq,
				const void* pBuffer, unsigned nLen, OsapUserInfo** ppUserInfo)
{
	BS_XLOG(XLOG_DEBUG,"COsapManager::%s...\n",__FUNCTION__);

	boost::unordered_map<string, OsapUserInfo*>::iterator iter = 
		m_mapUserInfoCatch.find(sReq.strUsername);
	if (iter != m_mapUserInfoCatch.end())
	{
		OsapUserInfo *pUserInfo = iter->second;

		timeval_a now,interval,endtime;
		gettimeofday_a(&now,NULL);
		interval.tv_sec = 30*60;
		interval.tv_usec = 0;
		timeradd(&(pUserInfo->tmUpdate),&interval,&endtime);
		if (timercmp(&now,&endtime,>=))
		{
			BS_XLOG(XLOG_DEBUG,"COsapManager::%s, timeout! need osap request\n",__FUNCTION__);
			delete pUserInfo;
			m_mapUserInfoCatch.erase(iter);
		}
		else
		{
			pUserInfo->tmUpdate = now;
			//CHpsStack::Instance()->UpdataOsapUserTime(m_pSessionManager, sReq.strUsername, now);
			if (ppUserInfo != NULL)
			{
				*ppUserInfo = pUserInfo;
			}
			return 0;
		}
	}

	// need send request
	if (m_mapAvenueRequest.find(sReq.strUsername) != m_mapAvenueRequest.end())
	{
		m_mapAvenueRequest[sReq.strUsername].push_back(new AvenueRequest(sReq, pBuffer, nLen));
		return 1; 
	}

	CSapEncoder osapRequest;
	FillOsapRequest(sReq.strUsername, "", 0, osapRequest);
	unsigned int nSequenceId = htonl(((SSapMsgHeader*)osapRequest.GetBuffer())->dwSequence);
	int nCode = m_pSessionManager->SendOsapRequest(osapRequest.GetBuffer(), osapRequest.GetLen());
	if (nCode == 0)
	{
		timeval_a itv, timeout;
		itv.tv_sec = OSAP_REQUEST_TIME_INTERVAL;
		itv.tv_usec = 0;
		timeradd(&(sReq.tStart), &itv, &timeout);
		m_mapAvenueRequest[sReq.strUsername].push_back(new AvenueRequest(sReq, pBuffer, nLen));
		m_mapRegistRequest[nSequenceId] = UserInfo(sReq.strUsername, timeout);
		m_mapRegistRequestTimer.insert(make_pair(timeout, nSequenceId));
		return 1;
	}
	else
	{
		if (CHpsStack::nOsapOption == 1)
		{
			return 0;
		}
	}
	return ERROR_OSAP_REQUEST_FAIL;
}

int COsapManager::CheckAuthenAndSignature(SRequestInfo& sReq, OsapUserInfo* pUserInfo)
{
	BS_XLOG(XLOG_DEBUG,"COsapManager::%s URL[%s] userName[%s]\n",__FUNCTION__,
		sReq.strUriCommand.c_str(),sReq.strUserName.c_str());
	if (!sReq.bOsapCheck)
	{
		BS_XLOG(XLOG_DEBUG,"COsapManager::%s !bOsapCheck\n",__FUNCTION__);
		return 0; // do not need check
	}

	vector<string> vecAttri;
	boost::algorithm::split(vecAttri, sReq.strUriAttribute,
		boost::algorithm::is_any_of("&"), boost::algorithm::token_compress_on);

	for (vector<string>::iterator iter = vecAttri.begin();
		iter != vecAttri.end(); ++iter)
	{
		*iter = URLDecoder::decode(iter->c_str());
	}

	if (sReq.bFromApipool)
		sort(vecAttri.begin(), vecAttri.end(), cmpstr_apipool);
	else
		sort(vecAttri.begin(), vecAttri.end(), cmpstr);

	string strSignatureSrc;
	string strSignature;
	for (vector<string>::iterator iter = vecAttri.begin();
		iter != vecAttri.end(); ++iter)
	{
		size_t nKeyLen = strlen("signature=");
		if (strncmp(iter->c_str(), "signature=", nKeyLen) == 0)
		{
			strSignature = iter->substr(nKeyLen, iter->length()-nKeyLen);
		}
		else
		{
			strSignatureSrc += (*iter);
		}
	}

	if (pUserInfo == NULL)
	{
		BS_XLOG(XLOG_DEBUG,"COsapManager::%s osap request failed\n",__FUNCTION__);
		sReq.nSignatureStatus = OSAP_SYS_ERR;
		return sReq.inWhiteList ?  0 : ERROR_OSAP_REQUEST_FAIL;
	}

	sReq.nAppid =  pUserInfo->nAppid;
	sReq.nAreaId = pUserInfo->nAreaId;
	sReq.nGroupId = pUserInfo->nGroupId;
	sReq.nHostid = pUserInfo->nHostid;
	sReq.nSpid = pUserInfo->nSpid;

	if (pUserInfo->nResult != 0)
	{
		BS_XLOG(XLOG_DEBUG,"COsapManager::%s pUserInfo->nResult = [%d]\n",__FUNCTION__,pUserInfo->nResult);
		sReq.nSignatureStatus = OSAP_NO_USER_COFG;
		return sReq.inWhiteList ?  0 : ERROR_CHECK_AUTHEN;
	}

	int nRet = CheckAuthen(pUserInfo, sReq.strRemoteIp, sReq.nServiceId, sReq.nMsgId);
	if (0 != nRet)
	{
		BS_XLOG(XLOG_DEBUG,"COsapManager::%s, check authen failed\n",__FUNCTION__);
		sReq.nSignatureStatus = nRet;
		if (sReq.inWhiteList)
		{
			return 0;
		}
		return nRet == OSAP_IP_FORBIDEN ? ERROR_CHECK_AUTHEN_IPLIST : ERROR_CHECK_AUTHEN;
	}

	nRet = CheckSignature(pUserInfo, strSignatureSrc, strSignature);
	if (0 != nRet)
	{
		BS_XLOG(XLOG_DEBUG,"COsapManager::%s, check signature failed\n",__FUNCTION__);
		sReq.nSignatureStatus = nRet;
		return sReq.inWhiteList ?  0 : ERROR_CHECK_SIGNATURE;
	}
	sReq.nSignatureStatus = OSAP_OK;
	return 0; // OK!
}

int COsapManager::DoSendOsapRequest(SRequestInfo &sReq)
{
	if (m_mapHttpRequest.find(sReq.strUserName) != m_mapHttpRequest.end())
	{
		m_mapHttpRequest[sReq.strUserName].push_back(sReq);
		return 1; 
	}

	CSapEncoder osapRequest;
	FillOsapRequest(sReq.strUserName, sReq.strRemoteIp, sReq.dwRemotePort, osapRequest);
	unsigned int nSequenceId = htonl(((SSapMsgHeader*)osapRequest.GetBuffer())->dwSequence);
	int nCode = m_pSessionManager->SendOsapRequest(osapRequest.GetBuffer(), osapRequest.GetLen());
	if (nCode == 0)
	{
		timeval_a itv, timeout;
		itv.tv_sec = OSAP_REQUEST_TIME_INTERVAL;
		itv.tv_usec = 0;
		timeradd(&(sReq.tStart), &itv, &timeout);
		m_mapHttpRequest[sReq.strUserName].push_back(sReq);
		m_mapRegistRequest[nSequenceId] = UserInfo(sReq.strUserName, timeout);
		m_mapRegistRequestTimer.insert(make_pair(timeout, nSequenceId));
		return 1;
	}
	else
	{
		if (CHpsStack::nOsapOption == 1)
		{
			sReq.bOsapCheck = false;
			return 0;
		}
	}
	return ERROR_OSAP_REQUEST_FAIL;
}

int COsapManager::FillOsapRequest(const string& strUserName, const string& strClientIp,
					unsigned int nPort, CSapEncoder& osapRequest)
{
	BS_XLOG(XLOG_DEBUG,"COsapManager::%s userName[%s]\n",__FUNCTION__,strUserName.c_str());
	osapRequest.SetService(SAP_PACKET_REQUEST, SAP_SMS_SERVICE_ID, SMS_MSG_GETALL);
	osapRequest.SetSequence(m_nSequence++); 
	osapRequest.SetValue(SMS_Attri_Name, strUserName);
	//osapRequest.SetValue(SMS_Attri_Ip, strClientIp);
	//osapRequest.SetValue(SMS_Attri_Port, nPort);
	return 0;
}

int COsapManager::DecodeOsapResponse(const string& strUserName, 
					const void* pBuffer, unsigned int nLen, OsapUserInfo** ppUserInfo)
{
	CSapDecoder response(pBuffer, nLen);
	BS_XLOG(XLOG_DEBUG,"COsapManager::%s userName[%s] ocde[%d]\n",__FUNCTION__,strUserName.c_str(),response.GetCode());
	if (is_sys_error((int)response.GetCode()))
	{
		return response.GetCode();
	}

	OsapUserInfo* pUserInfo = new OsapUserInfo;
	pUserInfo->nResult = response.GetCode();

	if (pUserInfo->nResult == 0) // else: error_code, such as SMS_NOUSER/SMS_IPNOTMATCH
	{
		response.DecodeBodyAsTLV();

		vector<SSapValueNode> vecKeyValue;
		response.GetValues(SMS_Attri_ConfValue, vecKeyValue);
		ParseKeyValue(vecKeyValue, pUserInfo);

		vector<SSapValueNode> vecPassword;
		response.GetValues(SMS_Attri_PasswdType, vecPassword);
		ParsePassword(vecPassword, pUserInfo);

		string strPrivilege;
		response.GetValue(SMS_Attri_Privilege, strPrivilege);
		ParsePrivilege(strPrivilege, pUserInfo);

		response.GetValue(SMS_Attri_AppId, (unsigned int*)&pUserInfo->nAppid);
		response.GetValue(SMS_Attri_AreaId , (unsigned int*)&pUserInfo->nAreaId);
		response.GetValue(SMS_Attri_GroupId , (unsigned int*)&pUserInfo->nGroupId);
		response.GetValue(SMS_Attri_HostId , (unsigned int*)&pUserInfo->nHostid);
		response.GetValue(SMS_Attri_SpId , (unsigned int*)&pUserInfo->nSpid);

		response.GetValues(SMS_Attri_LimitIp , pUserInfo->vecIpList);
		sort(pUserInfo->vecIpList.begin(), pUserInfo->vecIpList.end());
	}

	timeval_a now;
	gettimeofday_a(&now,NULL);
	pUserInfo->tmUpdate = now;
	m_mapUserInfoCatch.insert(make_pair(strUserName, pUserInfo));
	CHpsStack::Instance()->UpdataUserInfo(m_pSessionManager, strUserName, pUserInfo);
	if (ppUserInfo != NULL)
	{
		*ppUserInfo = pUserInfo;
	}
	return 0;
}

int COsapManager::OnOsapResponse(const void* pBuffer, unsigned int nLen)
{
	BS_XLOG(XLOG_DEBUG,"COsapManager::%s...\n",__FUNCTION__);
	unsigned int nSequenceId = htonl(((SSapMsgHeader*)pBuffer)->dwSequence);
	map<unsigned int, UserInfo>::iterator iterReg = m_mapRegistRequest.find(nSequenceId);
	if (iterReg != m_mapRegistRequest.end())
	{
		OsapUserInfo* pUserInfo = NULL;
		int nOsapCode = DecodeOsapResponse(iterReg->second.strName, pBuffer, nLen, &pUserInfo);

		SendRequestFromMap(iterReg->second.strName, nOsapCode, pUserInfo);

		std::pair<multimap<timeval_a, unsigned int>::iterator, multimap<timeval_a, unsigned int>::iterator>
			itr_pair = m_mapRegistRequestTimer.equal_range(iterReg->second.oTimeout);
		for(multimap<timeval_a, unsigned int>::iterator itr=itr_pair.first;
			itr!=itr_pair.second; ++itr)
		{
			if(itr->second == nSequenceId)
			{
				m_mapRegistRequestTimer.erase(itr);
				break;
			}
		}
		m_mapRegistRequest.erase(iterReg);
		return 0;
	}
	return -1;
}

void COsapManager::SendRequestFromMap(const string& strUsername, int nOsapCode, OsapUserInfo* pUserInfo)
{
	BS_XLOG(XLOG_DEBUG,"COsapManager::%s user[%s]\n",__FUNCTION__,strUsername.c_str());
	map<string, vector<SRequestInfo> >::iterator iteHttpReqests
		= m_mapHttpRequest.find(strUsername);
	if (iteHttpReqests != m_mapHttpRequest.end())
	{
		vector<SRequestInfo>& requests = iteHttpReqests->second;
		for (vector<SRequestInfo>::iterator iter = requests.begin();
			iter != requests.end(); ++iter)
		{
			SRequestInfo &sReq = *iter;
			int nCode = nOsapCode;
			if (nCode != 0 && CHpsStack::nOsapOption == 1)
			{
				sReq.bOsapCheck = false;
				nCode = 0;
			}
			m_pSessionManager->DoSendAvenueRequest(sReq, nCode, pUserInfo);
		}
		m_mapHttpRequest.erase(iteHttpReqests);
	}

	map<string, vector<AvenueRequest*> >::iterator iteAVReqests
		= m_mapAvenueRequest.find(strUsername);
	if (iteAVReqests != m_mapAvenueRequest.end())
	{
		vector<AvenueRequest*>& requests = iteAVReqests->second;
		for (vector<AvenueRequest*>::iterator iter = requests.begin();
			iter != requests.end(); ++iter)
		{
			AvenueRequest* pRequest = *iter;
			m_pSessionManager->DoSendHttpRequest(pRequest->avenueRequest,
				pRequest->pBuffer, pRequest->nLen, pUserInfo);
			delete pRequest;
		}
		m_mapAvenueRequest.erase(iteAVReqests);
	}
}

void COsapManager::DetectOsapRequestTimout(timeval_a& now)
{
	//BS_XLOG(XLOG_DEBUG,"COsapManager::%s...\n",__FUNCTION__);
	while(!m_mapRegistRequestTimer.empty())
	{
		if((m_mapRegistRequestTimer.begin()->first)<now)
		{
			map<unsigned int, UserInfo>::iterator iterReg = 
				m_mapRegistRequest.find(m_mapRegistRequestTimer.begin()->second);
			if (iterReg != m_mapRegistRequest.end())
			{
				SendRequestFromMap(iterReg->second.strName, ERROR_SOS_RESPONSE_TIMEOUT, NULL);
				m_mapRegistRequest.erase(iterReg);
			}
			m_mapRegistRequestTimer.erase(m_mapRegistRequestTimer.begin());
		}
		else
		{
			break;
		}
	}
}

void COsapManager::ParseKeyValue(const vector<SSapValueNode>& vecValueNode, OsapUserInfo* pUserInfo)
{
	BS_XLOG(XLOG_DEBUG,"COsapManager::%s...\n",__FUNCTION__);
	struct KeyValue
	{
		int nKey;
		char szValue[0];
	};

	for (vector<SSapValueNode>::const_iterator iter = vecValueNode.begin();
		iter != vecValueNode.end(); ++iter)
	{
		KeyValue* pRet = (KeyValue*)iter->pLoc;
		if (iter->nLen > sizeof(KeyValue))
		{
			OsapKeyValue keyValue;
			keyValue.nKey = ntohl(pRet->nKey);
			keyValue.strValue = string((const char*)pRet->szValue, iter->nLen-sizeof(KeyValue));
			pUserInfo->vecKeyValue.push_back(keyValue);
		}
	}
}

void COsapManager::ParsePassword(const vector<SSapValueNode>& vecValueNode, OsapUserInfo* pUserInfo)
{
	BS_XLOG(XLOG_DEBUG,"COsapManager::%s...\n",__FUNCTION__);
	struct Password
	{
		int nType;
		char szBeginTime[20]; // yyyy-mm-dd hh:mi:ss 
		char szEndTime[20];
		char szValue[0];
	};

	int nCount = 0;
	for (vector<SSapValueNode>::const_iterator iter = vecValueNode.begin();
		iter != vecValueNode.end(); ++iter)
	{
		Password* pRet = (Password*)iter->pLoc;
		if (nCount%3 == 0 && 
			iter->nLen > sizeof(Password) &&
			ntohl(pRet->nType) == SMS_ATTRI_MD5_PWD)
		{
			pRet->szBeginTime[19] = pRet->szEndTime[19] = 0;
			OsapPassword oPassword;
			oPassword.strKey = string((const char*)pRet->szValue, iter->nLen-sizeof(Password));
			oPassword.nBeginTime = get_time(pRet->szBeginTime);
			oPassword.nEndTime = get_time(pRet->szEndTime);
			//BS_XLOG(XLOG_DEBUG,"COsapManager::%s key[%s] [%d] [%s] [%s]\n",
			//	__FUNCTION__,oPassword.strKey.c_str(),oPassword.strKey.length(),pRet->szBeginTime,pRet->szEndTime);
			pUserInfo->vecPassword.push_back(oPassword);
		}
		++nCount;
	}
}

void COsapManager::ParsePrivilege(const string& strPrivilege, OsapUserInfo* pUserInfo)
{
	BS_XLOG(XLOG_DEBUG,"COsapManager::%s Privilege[%s]\n",__FUNCTION__,strPrivilege.c_str());
	vector<string> vecService;
	boost::algorithm::split(vecService, strPrivilege,
		boost::algorithm::is_any_of(","), boost::algorithm::token_compress_on); 
	vector<string>::const_iterator itrService;

	for(vector<string>::const_iterator itrService = vecService.begin();
		itrService != vecService.end(); ++itrService)
	{
		const string & strService= *itrService;
		char szMsg[2048]={0};
		OsapPrivilege obj;
		if(sscanf(strService.c_str(), "%u{%2047[^}]}", &(obj.nServiceId), szMsg) == 2)
		{

			vector<string> vecMsgId;
			boost::algorithm::split( vecMsgId, szMsg,
				boost::algorithm::is_any_of("/"),boost::algorithm::token_compress_on); 

			for(vector<string>::const_iterator itrMsgId = vecMsgId.begin();
				itrMsgId != vecMsgId.end();++itrMsgId)
			{
				const string & strMsgId=*itrMsgId;
				obj.vecMsgId.push_back(atoi(strMsgId.c_str()));
			}
			sort(obj.vecMsgId.begin(),obj.vecMsgId.end());
		}
		pUserInfo->vecPrivilege.push_back(obj);
	}
	sort(pUserInfo->vecPrivilege.begin(),pUserInfo->vecPrivilege.end());
}

int COsapManager::CheckAuthen(const OsapUserInfo* pUserInfo, const string& strClientIp,
				unsigned int nServiceId, unsigned int nMsgId)
{
	BS_XLOG(XLOG_DEBUG,"COsapManager::%s SvrId[%d] MsgId[%d]\n",__FUNCTION__,nServiceId,nMsgId);
	vector<OsapPrivilege>::const_iterator iter = std::find(
		pUserInfo->vecPrivilege.begin(), pUserInfo->vecPrivilege.end(), nServiceId);

	if (iter == pUserInfo->vecPrivilege.end())
	{
		return OSAP_NO_AUTHEN;
	}

	if (!std::binary_search(iter->vecMsgId.begin(), iter->vecMsgId.end(), nMsgId ))
	{
		return OSAP_NO_AUTHEN;
	}

	if (pUserInfo->vecIpList.size() > 0)
	{
		if (!std::binary_search(pUserInfo->vecIpList.begin(), pUserInfo->vecIpList.end(), strClientIp))
		{
			BS_XLOG(XLOG_DEBUG,"COsapManager::%s ip no permission[%s] %d\n",__FUNCTION__,strClientIp.c_str());
			return OSAP_IP_FORBIDEN;
		}
	}

	return 0;
}

int COsapManager::CheckSignature(const OsapUserInfo* pUserInfo,
				   const string& strSigntureSrc, const string& strSignature)
{
	BS_XLOG(XLOG_DEBUG,"COsapManager::%s signature[%s]\n",__FUNCTION__,strSignature.c_str());
	timeval_a now;
	gettimeofday_a(&now,NULL);
	bool bCheck = false;
	for (vector<OsapPassword>::const_iterator iter = pUserInfo->vecPassword.begin();
		iter != pUserInfo->vecPassword.end(); ++iter)
	{
		if (now.tv_sec >= iter->nBeginTime && now.tv_sec <= iter->nEndTime)
		{
			bCheck = true;
			unsigned char szMd5[32] = {0};
			unsigned char szMd5Hex[64] = {0};
			string strSrc = strSigntureSrc + iter->strKey;
			sdo::common::CCipher::Md5((unsigned char*)strSrc.c_str(), strSrc.length(), szMd5);
			encode_hex_str(szMd5, 16, szMd5Hex);
			if (strncasecmp(strSignature.c_str(), (char*)szMd5Hex, 32) == 0)
			{
				return 0; // ok
			}
		}
	}
	BS_XLOG(XLOG_DEBUG,"COsapManager::%s signatureSrc[%s] [%d]\n",__FUNCTION__,strSigntureSrc.c_str(), bCheck);
	return (bCheck ? OSAP_ERROR_SIG : OSAP_NO_PASSWORD);
}

void COsapManager::UpdataUserInfo(const string& strUserName, OsapUserInfo* pUserInfo)
{
	BS_XLOG(XLOG_DEBUG,"COsapManager::%s...\n",__FUNCTION__);
	boost::unordered_map<string, OsapUserInfo*>::iterator
		iter = m_mapUserInfoCatch.find(strUserName);
	if (iter != m_mapUserInfoCatch.end())
	{
		delete iter->second;
		m_mapUserInfoCatch.erase(iter);
	}
	m_mapUserInfoCatch.insert(make_pair(strUserName, pUserInfo));
}

void COsapManager::UpdataUserReqeustTime(const string& strUserName, const timeval_a& sTime)
{
	BS_XLOG(XLOG_DEBUG,"COsapManager::%s...\n",__FUNCTION__);
	boost::unordered_map<string, OsapUserInfo*>::iterator
		iter = m_mapUserInfoCatch.find(strUserName);
	if (iter != m_mapUserInfoCatch.end())
	{
		iter->second->tmUpdate = sTime;
	}
}

void COsapManager::ClearCatch(const string& strMerchant)
{
	BS_XLOG(XLOG_DEBUG,"COsapManager::%s...%s\n",__FUNCTION__,strMerchant.c_str());
	if (strMerchant.empty())
	{
		for (boost::unordered_map<string, OsapUserInfo*>::iterator
			iter = m_mapUserInfoCatch.begin(); iter != m_mapUserInfoCatch.end(); ++iter)
		{
			delete iter->second;
		}
		m_mapUserInfoCatch.clear();
	}
	else
	{
		boost::unordered_map<string, OsapUserInfo*>::iterator iter = m_mapUserInfoCatch.find(strMerchant);
		if (iter != m_mapUserInfoCatch.end())
		{
			delete iter->second;
			m_mapUserInfoCatch.erase(iter);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// for test

string COsapManager::OnCalcMd5(const string& strAttris, const string& strKey)
{
	bool bFromApipool = true;
	string::size_type posUsername = strAttris.find("merchant_name=");
	if (posUsername == string::npos)
	{
		posUsername = strAttris.find("osap_user=");
		if (posUsername == string::npos)
		{
			posUsername = strAttris.find("client_id=");
		}
	}
	if (posUsername != string::npos)
	{
		bFromApipool = false;
	}

	vector<string> vecAttri;
	boost::algorithm::split(vecAttri, strAttris,
		boost::algorithm::is_any_of("&"), boost::algorithm::token_compress_on);

	for (vector<string>::iterator iter = vecAttri.begin();
		iter != vecAttri.end(); ++iter)
	{
		*iter = URLDecoder::decode(iter->c_str());
	}

	if (bFromApipool)
		sort(vecAttri.begin(), vecAttri.end(), cmpstr_apipool);
	else
		sort(vecAttri.begin(), vecAttri.end(), cmpstr);

	string strSignatureSrc;
	for (vector<string>::iterator iter = vecAttri.begin();
		iter != vecAttri.end(); ++iter)
	{
		if (strncmp(iter->c_str(), "signature=", strlen("signature=")) != 0)
		{
			strSignatureSrc += *iter;
		}
	}
	unsigned char szMd5[32] = {0};
	unsigned char szMd5Hex[64] = {0};
	string strSrc = strSignatureSrc + URLDecoder::decode(strKey.c_str());
	BS_XLOG(XLOG_DEBUG,"COsapManager::%s %s\n",__FUNCTION__, strSrc.c_str());
	sdo::common::CCipher::Md5((unsigned char*)strSrc.c_str(), strSrc.length(), szMd5);
	encode_hex_str(szMd5, 16, szMd5Hex);
	return strSrc + "<br/>signature=" + string((char*)szMd5Hex, 32);
}
