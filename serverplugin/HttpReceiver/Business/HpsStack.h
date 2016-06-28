#ifndef _SDO_SAP_STACK_H_
#define _SDO_SAP_STACK_H_
#include "SessionManager.h"
#include <boost/thread/mutex.hpp>
#include <vector>
#include <string>

using std::string;
using std::vector;

class CHttpRecVirtualClient;

class CHpsStack
{
public:
    static CHpsStack *Instance();
    void Start(CHttpRecVirtualClient *pVirtualClient);
    void Stop();

    CHpsSessionManager * GetThread();	
    CHpsSessionManager * GetThread(int n);
    CHpsSessionManager * GetAcceptThread();
public:
	int OnLoadTransferObjs(const string &strH2ADir, const string &strH2AFilter, const string &strA2HDir, const string &strA2HFilter);
	void OnLoadSosList(const vector<string> & vecServer);
	void OnUnLoadSharedLibs(const string &strDirPath,const vector<string> &vecSo);
	void OnReLoadTransferObjs(const string &strDirPath, const vector<string> &vecSoName);
	int OnLoadRouteConfig(const vector<string> &vecRouteBalance);
	void OnUpdateIpLimit(const map<string, string> & mapIpLimits);

	void OnUpdateRouteConfig(unsigned int dwServiceId, unsigned int dwMsgId, int nErrCode);

	void GetThreadStatus(int &nThreads, int &nDeadThreads);
        void GetSelfCheckState(unsigned int &dwAliveNum, unsigned int &dwAllNum);

	string GetPluginSoInfo();

	void UpdataUserInfo(const CHpsSessionManager* pManager,
		const string& strUserName, OsapUserInfo* pUserInfo);
	void UpdataOsapUserTime(const CHpsSessionManager* pManager,
		const string& strUserName, const timeval_a& sTime);
	void ClearOsapCatch(const string& strMerchant);

	void GetUrlMapping(vector<string> &vecUrlMapping);
	int OnUpdateConfigFromOsap(bool bWait = true);
	void UpdateFinished(int nUpdateStat);
	SUrlInfo GetSoNameByUrl(const string& strUrl);

	void Dump();

private:
    CHpsStack():ppThreads(NULL),m_nThread(0),m_dwIndex(0),m_bUpdate(false),m_pVirtualClient(NULL){}
    static CHpsStack * sm_instance;
    CHpsSessionManager ** ppThreads;
    int m_nThread;
    unsigned int m_dwIndex;
    bool m_bUpdate;
    CHttpRecVirtualClient *m_pVirtualClient;

public:
	static int nOsapOption;
};
#endif

