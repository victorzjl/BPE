#ifndef _COH_SERVER_IMP_H_
#define _COH_SERVER_IMP_H_
#include "CohStack.h"
#include "CohServer.h"
#include "CohRequestMsg.h"


#include <boost/thread.hpp>
#include <boost/unordered_map.hpp>
#include <string>
#include <map>
using std::map;
using std::string;
using std::make_pair;

namespace sdo{
namespace coh{

class CohServerImp:public ICohServer
{
	
public:
	static CohServerImp* GetInstance();
	int Start(unsigned int dwPort);
	virtual void OnReceiveRequest(void * identifier,const string &request,const string &remote_ip,unsigned int remote_port);

private:
	CohServerImp();
	void OnHelp(void * identifier);
	void OnUnloadSharedLibs(void * identifier,const vector<string> &vecSo);
	void OnReloadSharedLibs(void * identifier,const vector<string> &vecSo);
	void OnReloadSosList(void * identifier);
	void OnQuerySelfCheck(void * identifier);
	void OnGetPluginSoInfo(void *identifier);
	void OnClearCache(void * identifier, const string& strRequest);
	void OnGetCurrentNoHttpResponseNum(void * identifier);
	void OnReLoadRouteConfig(void * identifier);
	void OnCalcMd5(void * identifier, const string& strRequest);
	void OnGetUrlMapping(void *identifier);

	void OnGetAllService(void * identifier);
	void OnGetAllURLs(void * identifier);
	void OnGetAllServiceMsg(void *identifier);
	void OnGetAllServers(void * identifier);
	void OnUpdateConfigFromOsap(void *identifier);
	void OnShowParam(void *identifier, const string& strUrl);

private:
	static CohServerImp* sm_pInstance;
	map<string, string> m_mapCommand;
	unsigned int m_dwPort;

};
}
}
#endif

