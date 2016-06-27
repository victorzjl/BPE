#ifndef _SDO_SAP_STACK_H_
#define _SDO_SAP_STACK_H_
#include <vector>
#include <string>
#include <map>
#include "SapRecord.h"
#include "SapServiceInfo.h"

using std::string;
using std::vector;
using std::map;
class CSessionManager;
class CSapStack
{
public:
	static CSapStack *Instance();
	void Start(int nThread=3);
	void Stop();
	
	int LoadConfig(const char* pConfig="config.xml");
	int LoadServiceConfig();
	int LoadVirtualService();
    int LoadAsyncVirtualService();
	int LoadAsyncVirtualClientService();
	int InstallVirtualService(const string& strName);
	int UnInstallVirtualService(const string& strName);
	CSessionManager * GetThread();	
	void HandleRequest(void *pBuffer, int nLen);
	void GetThreadStatus(int &nThreads,int &nDeadThread);
	void GetConnStatus(SConnectData &oData);
	const string GetPluginInfo();
    const string GetConfigInfo();
    void SetAllConfig(const void* buffer, const int nLen);
	void Dump();
    bool IsClosed();
private:
	void Init(int visAuthSoc,int visVerifySocSignature,int visSocEnc,const string &vstrLocalKey);
	void LoadSos(const vector<SSosStruct> & vecSos);
	void LoadSuperSoc(const vector<string> & vecSoc,const vector<string> & strPrivelege);
    int  LoadDataServerConfig(map<SComposeKey, vector<unsigned> >& mapDataServerId, const char* pConfig = "DataServer.xml");
private:
	CSapStack();
	static CSapStack * sm_instance;
	CSessionManager ** ppThreads;
	int m_nThread;
	unsigned int m_dwIndex;
    unsigned int m_minDataServiceId;

public:
	static int isAsc;
	static int isLocalPrivilege;
	static int isAuthSoc;
	static int isVerifySocSignature;
	static int isSocEnc;
	static string strLocalKey;
    static int nLogType;
};
#endif

