#ifndef OSAP_MANAGER_H
#define OSAP_MANAGER_H

#include <boost/unordered_map.hpp>
#include <map>
#include <string>
#include <algorithm>
#include "HpsCommonInner.h"
#include "OsapConfig.h"
#include "SapMessage.h"

using namespace std;

class CHpsSessionManager;
struct OsapUserInfo;

class COsapManager
{
public:
	COsapManager();
	~COsapManager();

	void SetSessionManager(CHpsSessionManager* pSessionManager);

	int LoadConfig();

	int GetUserInfo(SRequestInfo& sReq, OsapUserInfo** ppUserInfo);
	int GetUserInfo(SAvenueRequestInfo& sReq,
		const void* pBuffer, unsigned nLen, OsapUserInfo** ppUserInfo);
	int CheckAuthenAndSignature(SRequestInfo& sReq, OsapUserInfo* pUserInfo);
	int OnOsapResponse(const void* pBuffer, unsigned int nLen);

	void SendRequestFromMap(const string& strUsername, int nOsapCode, OsapUserInfo* pUserInfo);

	void DetectOsapRequestTimout(timeval_a& now);

	void UpdataUserInfo(const string& strUserName, OsapUserInfo* pUserInfo);
	void UpdataUserReqeustTime(const string& strUserName, const timeval_a& sTime);
	void ClearCatch(const string& strMerchant="");
	bool IsUrlInNoSaagList(const string& strUrl);

	static string OnCalcMd5(const string& strAttris, const string& strKey);

private:
	int DoSendOsapRequest(SRequestInfo &sReq);
	int DecodeOsapResponse(const string& strUserName,
		const void* pBuffer, unsigned int nLen, OsapUserInfo** ppUserInfo);
	int FillOsapRequest(const string& strUserName, const string& strClientIp,
		unsigned int nPort, CSapEncoder& osapRequest);
	void ParseKeyValue(const vector<SSapValueNode>& vecValueNode, OsapUserInfo* pUserInfo);
	void ParsePassword(const vector<SSapValueNode>& vecValueNode, OsapUserInfo* pUserInfo);
	void ParsePrivilege(const string& strPrivilege, OsapUserInfo* pUserInfo);

	int CheckAuthen(const OsapUserInfo* pUserInfo, const string& strClientIp,
		unsigned int nServiceId, unsigned int nMsgId);
	int CheckSignature(const OsapUserInfo* pUserInfo,
		const string& strSigntureSrc, const string& strSignature);

private:
	CHpsSessionManager* m_pSessionManager;
	COsapConfig m_config;
	unsigned int m_nSequence;
	boost::unordered_map<string, OsapUserInfo*> m_mapUserInfoCatch;

	map<string, vector<SRequestInfo> > m_mapHttpRequest;
	struct AvenueRequest
	{
		SAvenueRequestInfo avenueRequest;
		void* pBuffer;
		unsigned nLen;
		AvenueRequest(const SAvenueRequestInfo& reqest, const void* _pBuffer, unsigned _nLen)
			:avenueRequest(reqest)
		{
			pBuffer = malloc(_nLen);
			memcpy(pBuffer, _pBuffer, _nLen);
			nLen = _nLen;
		}
		~AvenueRequest(){free(pBuffer);}
	};
	map<string, vector<AvenueRequest*> > m_mapAvenueRequest;
	struct UserInfo
	{
		string strName;
		timeval_a oTimeout;
		UserInfo(const string& _strName, const timeval_a& _timeout)
			: strName(_strName), oTimeout(_timeout){}
			UserInfo(){}
	};
	map<unsigned int, UserInfo> m_mapRegistRequest;
	multimap<timeval_a, unsigned int> m_mapRegistRequestTimer;
};

struct OsapKeyValue
{
	unsigned int nKey;
	string strValue;
};

struct OsapPassword
{
	string strKey;
	int nBeginTime;
	int nEndTime;
};

struct OsapPrivilege
{
	unsigned int nServiceId;
	vector<unsigned int> vecMsgId;
	int operator<(const OsapPrivilege &p)const
	{
		return (nServiceId < p.nServiceId);
	}
	bool operator==(unsigned int nKey)const
	{
		return (nServiceId == nKey);
	}
	OsapPrivilege(unsigned int _nServiceId = 0):nServiceId(_nServiceId){}
};

struct OsapUserInfo
{
	int nResult;
	timeval_a tmUpdate;
	int nAppid;
	int nAreaId;
	int nGroupId;
	int nHostid;
	int nSpid;
	vector<OsapKeyValue> vecKeyValue;
	vector<OsapPrivilege> vecPrivilege;
	vector<OsapPassword> vecPassword;
	vector<string> vecIpList;
	OsapUserInfo():nResult(0),nAppid(-1),nAreaId(-1),
		nGroupId(-1),nHostid(-1),nSpid(-1){}
};

#endif // OSAP_MANAGER_H
